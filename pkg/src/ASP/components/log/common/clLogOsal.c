/*
 * Copyright (C) 2002-2009 by OpenClovis Inc. All  Rights Reserved.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office.
 * 
 * This program is  free software; you can redistribute it and / or
 * modify  it under  the  terms  of  the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * This program is distributed in the  hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied  warranty  of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 * 
 * You  should  have  received  a  copy of  the  GNU General Public
 * License along  with  this program. If  not,  write  to  the 
 * Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * Build: 4.2.0
 */
#include <sys/time.h>
#include <string.h>

#include <clLogErrors.h>
#include <clLogDebug.h>
#include <clLogOsal.h>

ClRcT
clOsalNanoTimeGet_L(ClTimeT  *pTime)
{
    ClRcT        rc   = CL_OK;
    ClNanoTimeT  time = {0}; 

    rc = clOsalNanoTimeGet(&time);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalNanoTimeGet(): rc[0x %x]", rc));
        return rc;
    }
    *pTime  = time.tv_sec;
    *pTime *= (1000L * 1000L * 1000L);
    *pTime += time.tv_nsec;

    CL_LOG_DEBUG_VERBOSE(("Time: %lld", *pTime));
    return CL_OK;
}

#if 0
ClRcT
clOsalMutexAttrInit_L(ClOsalMutexAttr_LT  *pAttr)
{
    CL_LOG_DEBUG_VERBOSE(("Enter: %p", (void *) pAttr));

    if( 0 != pthread_mutexattr_init(pAttr) )
    {
        perror("pthread_mutexattr_init() failure");
        return CL_LOG_RC(CL_ERR_MUTEX_ERROR);
    }

    CL_LOG_DEBUG_VERBOSE(("Exit"));
    return CL_OK;
}

ClRcT
clOsalMutexAttrPSharedSet_L(ClOsalMutexAttr_LT   *pAttr,
                            ClOsalSharedType_LT  type)
{
    CL_LOG_DEBUG_VERBOSE(("Enter: %p %d", (void *) pAttr, type));

    if( 0 !=
        pthread_mutexattr_setpshared(pAttr, (CL_OSAL_PROCESS_SHARED_L == type)
                                             ? PTHREAD_PROCESS_SHARED
                                             : PTHREAD_PROCESS_PRIVATE) )
    {
        perror("pthread_mutexattr_setpshared()");
        return CL_LOG_RC(CL_ERR_MUTEX_ERROR);
    }

    CL_LOG_DEBUG_VERBOSE(("Exit"));
    return CL_OK;
}

ClRcT
clOsalMutexAttrDestroy_L(ClOsalMutexAttr_LT  *pAttr)
{
    CL_LOG_DEBUG_VERBOSE(("Enter: %p", (void *) pAttr));

    if( 0 != pthread_mutexattr_destroy(pAttr) )
    {
        perror("pthread_mutexattr_destroy() failure");
        return CL_LOG_RC(CL_ERR_MUTEX_ERROR);
    }

    CL_LOG_DEBUG_VERBOSE(("Exit"));
    return CL_OK;
}

ClRcT
clOsalMutexInitEx_L(ClOsalMutexId_LT    *pMutex,
                    ClOsalMutexAttr_LT  *pAttr)
{
    CL_LOG_DEBUG_VERBOSE(("Enter: %p", (void *) pMutex));
    //fprintf(stderr, "clOsalMutexInitEx_L(): %p\n", (void *) pMutex);
#if 0
    *ppMutex = clHeapCalloc(1, sizeof(pthread_mutex_t));
    if( NULL = *pMutex )
    {
        return CL_LOG_RC(CL_ERR_NO_RESOURCE);
    }
#endif

    if( pthread_mutex_init(pMutex, pAttr) )
    {
        perror("pthread_mutex_init() failure");
        return CL_LOG_RC(CL_ERR_MUTEX_ERROR);
    }

    CL_LOG_DEBUG_VERBOSE(("Exit"));
    return CL_OK;
}

ClRcT
clOsalMutexLock_L(ClOsalMutexId_LT  *pMutex)
{
    CL_LOG_DEBUG_VERBOSE(("Enter: %p", (void *) pMutex));
//    fprintf(stderr, "clOsalMutexLock_L(): %p threadid: %lu\n", (void *) pMutex, pthread_self());
    if( 0 != pthread_mutex_lock(pMutex) )
    {
        perror("pthread_mutex_lock()");
        return CL_LOG_RC(CL_ERR_MUTEX_ERROR);
    }

    CL_LOG_DEBUG_VERBOSE(("Exit"));
    return CL_OK;
}

ClRcT
clOsalMutexTryLock_L(ClOsalMutexId_LT  *pMutex)
{
    CL_LOG_DEBUG_VERBOSE(("Enter: %p", (void *) pMutex));
    //fprintf(stderr, "clOsalMutexTryLock_L(): %p\n", (void *) pMutex);
    if( 0 != pthread_mutex_trylock(pMutex) )
    {
        perror("pthread_mutex_trylock()");
        return CL_LOG_RC(CL_ERR_MUTEX_ERROR);
    }

    CL_LOG_DEBUG_VERBOSE(("Exit"));
    return CL_OK;
}

ClRcT
clOsalMutexUnlock_L(ClOsalMutexId_LT  *pMutex)
{
    CL_LOG_DEBUG_VERBOSE(("Enter: %p", (void *) pMutex));
    //fprintf(stderr, "clOsalMutexUnlock_L(): %p threadid %lu\n", (void *) pMutex, pthread_self());
    if( 0 != pthread_mutex_unlock(pMutex) )
    {
        perror("pthread_mutex_unlock()");
        return CL_LOG_RC(CL_ERR_MUTEX_ERROR);
    }

    CL_LOG_DEBUG_VERBOSE(("Exit"));
    return CL_OK;
}

ClRcT
clOsalMutexDestroy_L(ClOsalMutexId_LT  *pMutex)
{
    int rc = 0;

    CL_LOG_DEBUG_VERBOSE(("Enter: %p", (void *) pMutex));
    //fprintf(stderr, "clOsalMutexDestroy_L(): %p\n", (void *) pMutex);
    errno = 0;
    rc = pthread_mutex_destroy(pMutex);
    if( 0 != rc )
    {
        perror("pthread_mutex_destroy()");
        CL_LOG_DEBUG_ERROR(("clOsalMutexDestroy_L()"));
        if( EBUSY == rc )
        {
            return CL_LOG_RC(CL_ERR_TRY_AGAIN);
        }
        return CL_LOG_RC(CL_ERR_MUTEX_ERROR);
    }

    CL_LOG_DEBUG_VERBOSE(("Exit"));
    return CL_OK;
}

ClRcT
clOsalCondAttrInit_L(ClOsalCondAttr_LT  *pAttr)
{
    CL_LOG_DEBUG_VERBOSE(("Enter: %p", (void *) pAttr));

    if( 0 != pthread_condattr_init(pAttr) )
    {
        perror("pthread_condattr_init() failure");
        return CL_LOG_RC(CL_ERR_MUTEX_ERROR);
    }

    CL_LOG_DEBUG_VERBOSE(("Exit"));
    return CL_OK;
}

ClRcT
clOsalCondAttrPSharedSet_L(ClOsalCondAttr_LT    *pAttr,
                           ClOsalSharedType_LT  type)
{
    CL_LOG_DEBUG_VERBOSE(("Enter: %p %d", (void *) pAttr, type));

    if( 0 !=
        pthread_condattr_setpshared(pAttr, (CL_OSAL_PROCESS_SHARED_L == type)
                                            ? PTHREAD_PROCESS_SHARED
                                            : PTHREAD_PROCESS_PRIVATE) )
    {
        perror("pthread_condattr_setpshared()");
        return CL_LOG_RC(CL_ERR_MUTEX_ERROR);
    }

    CL_LOG_DEBUG_VERBOSE(("Exit"));
    return CL_OK;
}

ClRcT
clOsalCondAttrDestroy_L(ClOsalCondAttr_LT  *pAttr)
{
    CL_LOG_DEBUG_VERBOSE(("Enter: %p", (void *) pAttr));

    if( 0 != pthread_condattr_destroy(pAttr) )
    {
        perror("pthread_condattr_destroy() failure");
        return CL_LOG_RC(CL_ERR_MUTEX_ERROR);
    }

    CL_LOG_DEBUG_VERBOSE(("Exit"));
    return CL_OK;
}

ClRcT
clOsalCondInitEx_L(ClOsalCondId_LT    *pCond,
                   ClOsalCondAttr_LT  *pAttr)
{
    CL_LOG_DEBUG_VERBOSE(("Enter: %p", (void *) pCond));
    //fprintf(stderr, "clOsalCondInitEx_L():Cond: %p\n", (void *) pCond);
#if 0
    *ppCond = clHeapCalloc(1, sizeof(pthread_cond_t));
    if( NULL = *ppCond )
    {
        return CL_LOG_RC(CL_ERR_NO_RESOURCE);
    }
#endif

    if( pthread_cond_init(pCond, pAttr) )
    {
        perror("pthread_cond_init() failure");
        return CL_LOG_RC(CL_ERR_MUTEX_ERROR);
    }

    CL_LOG_DEBUG_VERBOSE(("Exit"));
    return CL_OK;
}

ClRcT
clOsalCondWait_L(ClOsalCondId_LT   *pCond,
                 ClOsalMutexId_LT  *pMutex,
                 ClTimerTimeOutT   time)
{
    struct timeval  currTime = {0, 0};
    struct timespec timeout  = {0, 0};
    int             rc       = 0;

    CL_LOG_DEBUG_VERBOSE(("Enter: %p", (void *) pMutex));
    //fprintf(stderr, "clOsalCondWati_L(): Cond: %p Mutex: %p\n",
            //(void *) pCond, (void *) pMutex);
    if( (0 == time.tsSec ) && (0 == time.tsMilliSec) )
    {
        if( 0 != pthread_cond_wait(pCond, pMutex) )
        {
            perror("pthread_mutex_wait()");
            return CL_LOG_RC(CL_ERR_MUTEX_ERROR);
        }
        return CL_OK;
    }

    if( 0 != gettimeofday (&currTime, NULL) )
    {
        perror("pthread_mutex_wait()");
        return CL_LOG_RC(CL_ERR_MUTEX_ERROR);
    }
    currTime.tv_sec  += time.tsSec + (time.tsMilliSec / 1000L);
    currTime.tv_usec += (time.tsMilliSec % 1000L) * 1000L;

    timeout.tv_sec  = currTime.tv_sec + (currTime.tv_usec / (1000L * 1000L));
    timeout.tv_nsec = (currTime.tv_usec % (1000L * 1000L)) * 1000L;

    rc = pthread_cond_timedwait (pCond, pMutex, &timeout);
    if( 0 != rc )
    {
        if( ETIMEDOUT == rc)
        {
            CL_LOG_DEBUG_TRACE(("Cond timed out..."));
            return CL_LOG_RC(CL_ERR_TIMEOUT);
        }
        if( EBUSY == rc )
        {
            return CL_LOG_RC(CL_ERR_TIMEOUT);
        }
        perror("pthread_cond_timedwait()");
        return CL_LOG_RC(CL_ERR_MUTEX_ERROR);
    }

    CL_LOG_DEBUG_VERBOSE(("Exit"));
    return CL_OK;
}

ClRcT
clOsalCondBroadcast_L(ClOsalCondId_LT  *pCond)
{
    CL_LOG_DEBUG_VERBOSE(("Enter: %p", (void *) pCond));
    //fprintf(stderr, "clOsalCondBroadcast_L():Cond: %p\n", (void *) pCond);
    if( 0 != pthread_cond_broadcast(pCond) )
    {
        perror("pthread_cond_broadcast()");
        return CL_LOG_RC(CL_ERR_MUTEX_ERROR);
    }

    CL_LOG_DEBUG_VERBOSE(("Exit"));
    return CL_OK;
}

ClRcT
clOsalCondSignal_L(ClOsalCondId_LT  *pCond)
{
    CL_LOG_DEBUG_VERBOSE(("Enter: %p", (void *) pCond));
    //fprintf(stderr, "clOsalCondSignal_L():Cond: %p\n", (void *) pCond);
    if( 0 != pthread_cond_signal(pCond) )
    {
        perror("pthread_cond_signal()");
        return CL_LOG_RC(CL_ERR_MUTEX_ERROR);
    }

    CL_LOG_DEBUG_VERBOSE(("Exit"));
    return CL_OK;
}

ClRcT
clOsalCondDestroy_L(ClOsalCondId_LT  *pCond)
{
    int rc = 0;

    CL_LOG_DEBUG_VERBOSE(("Enter: %p", (void *) pCond));
    //fprintf(stderr, "clOsalCondDestroy_L():Cond: %p\n", (void *) pCond);
    rc = pthread_cond_destroy(pCond);
    if( 0 != rc )
    {
        perror("pthread_cond_destroy()");
        if( EBUSY == rc )
        {
            return CL_LOG_RC(CL_ERR_TRY_AGAIN);
        }
        return CL_LOG_RC(CL_ERR_MUTEX_ERROR);
    }

    CL_LOG_DEBUG_VERBOSE(("Exit"));
    return CL_OK;
}

ClRcT
clOsalFtruncate_L(ClInt32T  fd,
                  ClInt32T  length)
{
    CL_LOG_DEBUG_VERBOSE(("Enter: %d", length));
    //fprintf(stderr, "clOsalFtruncate_L(): fd: %d length: %d\n", fd, length);
    if( 0 != ftruncate(fd, length) )
    {
        perror("ftruncate()");
        return CL_LOG_RC(CL_ERR_BAD_OPERATION);
    }

    CL_LOG_DEBUG_VERBOSE(("Exit"));
    return CL_OK;
}



ClRcT
clOsalShmOpen_L(const ClCharT  *name,
                ClInt32T       oflag,
                ClUint32T      mode,
                ClInt32T       *fd)
{
    CL_LOG_DEBUG_VERBOSE(("Enter: %s", name));
    //fprintf(stderr, "clOsalShmOpen_L(): ShmName: %s\n", name);
    *fd = shm_open(name, oflag, mode);
    if( 0 > *fd )
    {
        if( EEXIST == errno  )
        {
            CL_LOG_DEBUG_VERBOSE(("%s already exists", name));
            //fprintf(stderr, "clOsalShmOpen_L(): ShmName: %s already exists\n",
      //              name);
            return CL_ERR_ALREADY_EXIST;
        }
        perror("shm_open");
        return CL_LOG_RC(CL_ERR_BAD_OPERATION);
    }

    //fprintf(stderr, "clOsalShmOpen_L(): ShmName: %s %d\n", name, *fd);
    CL_LOG_DEBUG_VERBOSE(("Exit"));
    return CL_OK;
}

ClRcT
clOsalShmClose_L(ClInt32T shmFd)
{
    CL_LOG_DEBUG_VERBOSE(("Enter:"));
    //fprintf(stderr, "clOsalShmClose_L(): ShmFd: %d\n", shmFd);
    close(shmFd);

    CL_LOG_DEBUG_VERBOSE(("Exit"));
    return CL_OK;
}

ClRcT
clOsalShmUnlink_L(const ClCharT *name)
{
    CL_LOG_DEBUG_VERBOSE(("Enter: %s", name));
    //fprintf(stderr, "clOsalShmUnlink_L(): ShmName: %s\n", name);
    if( 0 != shm_unlink(name) )
    {
        perror("unlink()");
        return CL_LOG_RC(CL_ERR_BAD_OPERATION);
    }

    CL_LOG_DEBUG_VERBOSE(("Exit"));
    return CL_OK;
}


ClRcT
clOsalMmap_L(ClPtrT     start,
             ClUint32T  length,
             ClInt32T   prot,
             ClInt32T   flags,
             ClInt32T   fd,
             ClInt32T   offset,
             ClPtrT     *mmapped)
{
    CL_LOG_DEBUG_VERBOSE(("Enter: %u", length));
    //fprintf(stderr, "clOsalMmap_L(): fd: %d length: %u\n", fd, length);
    *mmapped = mmap(start, length, prot, flags, fd, offset);
    if( MAP_FAILED == mmapped )
    {
        perror("mmap()");
        return CL_LOG_RC(CL_ERR_BAD_OPERATION);
    }

    //fprintf(stderr, "clOsalMmap_L(): mmaped area%p\n", *mmapped);
    CL_LOG_DEBUG_VERBOSE(("Exit"));
    return CL_OK;
}

ClRcT
clOsalMunmap_L(ClPtrT     start,
               ClUint32T  length)
{
    CL_LOG_DEBUG_VERBOSE(("Enter: %u", length));
    //fprintf(stderr, "clOsalMunmap_L(): ptr: %p length: %u\n", start, length);
    if( 0 != munmap(start, length) )
    {
        perror("munmap()");
        return CL_LOG_RC(CL_ERR_BAD_OPERATION);
    }

    CL_LOG_DEBUG_VERBOSE(("Exit"));
    return CL_OK;
}

ClRcT
clOsalMsync_L(ClPtrT     start,
              ClUint32T  length,
              ClInt32T   flags)
{
    CL_LOG_DEBUG_VERBOSE(("Enter: %u", length));

    if( 0 != msync(start, length, flags) )
    {
        perror("msync()");
        return CL_LOG_RC(CL_ERR_BAD_OPERATION);
    }

    CL_LOG_DEBUG_VERBOSE(("Exit"));
    return CL_OK;
}

ClUint32T
clOsalPageSizeGet_L()
{
    long pgSize = 0;

    pgSize = sysconf(_SC_PAGESIZE);
    if( -1 == pgSize )
        return 4096;

    CL_LOG_DEBUG_TRACE(("PageSize: %ld", pgSize));
    return (ClUint32T) pgSize;
}
#endif
