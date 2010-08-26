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
/*******************************************************************************
 * ModuleName  : osal
 * File        : posix.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This module implements OS abstraction layer                            
 **************************************************************************/
/* INCLUDES */

#ifndef POSIX_BUILD

#ifndef SOLARIS_BUILD
#include <bits/local_lim.h>
#include <execinfo.h>
#endif

#include <sys/sem.h>
#include <sys/shm.h>
#endif

#include <pthread.h>
#include <sched.h>
#include <semaphore.h>
#include <errno.h>
#include <sys/mman.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <clCommon.h>
#include <clCommonErrors.h>
#include <clTimerApi.h>
#include <clCntApi.h>
#include <clOsalApi.h>
#include <clHeapApi.h>
#include <clOsalErrors.h>
#include "../osal.h"
#include <clCksmApi.h>
#include <clDebugApi.h>
#include <clDbg.h>
#include <clBitApi.h>
#include <clLogApi.h>
#include "clOsalCommon.h"
#include "clPosix.h"

#ifdef _POSIX_PRIORITY_SCHEDULING
#include <sched.h>
#endif
#include <clEoApi.h>

static pthread_mutex_t gClSemAccessLock = PTHREAD_MUTEX_INITIALIZER;

/**************************************************************************/

ClRcT cosPosixProcessSharedSemInit(ClOsalMutexT *pMutex, ClUint8T *pKey, ClUint32T keyLen, ClInt32T value)
{
    ClRcT rc = CL_OK;
    int err;

    nullChkRet(pKey);
    if(keyLen == 0)
    {
        clDbgCodeError(CL_ERR_INVALID_PARAMETER,("Invalid keylen [%d]\n",keyLen));
        return CL_OSAL_RC(CL_ERR_INVALID_PARAMETER);
    }

    pthread_mutex_lock(&gClSemAccessLock);
    err = sem_init(&pMutex->shared_lock.sem.posSem,1, value);
    if(err < 0)
    {
        pthread_mutex_unlock(&gClSemAccessLock);
        rc = CL_OSAL_RC(CL_ERR_LIBRARY);
        goto out;
    }
    pthread_mutex_unlock(&gClSemAccessLock);
    
    pMutex->shared_lock.sem.numSems = 1;
    rc = CL_OK;

out:
    return rc;
}

/**************************************************************************/

ClRcT
cosPosixMutexValueSet(ClOsalMutexIdT mutexId, ClInt32T value)
{
    ClRcT rc = CL_OSAL_RC(CL_ERR_NOT_SUPPORTED);
    
    CL_FUNC_ENTER();

    clDbgCodeError(rc, ("POSIX semaphores dont support setval operations\n"));

    CL_FUNC_EXIT();
    return rc;
}

ClRcT
cosPosixMutexValueGet(ClOsalMutexIdT mutexId, ClInt32T *pValue)
{
    ClOsalMutexT *pMutex = (ClOsalMutexT*) mutexId;
    ClRcT rc = CL_OK;
    ClInt32T val = 0;
    
    nullChkRet(pMutex);
    CL_FUNC_ENTER();

    val = sem_getvalue(&pMutex->shared_lock.sem.posSem, pValue);
    if(val < 0 )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Mutex value get- sem_getvalue() failed with [%s]\n", strerror(errno)));
        rc = CL_OSAL_RC(CL_ERR_LIBRARY);
    }

    CL_FUNC_EXIT();

    return rc;
}


/**************************************************************************/

ClRcT 
cosPosixMutexLock (ClOsalMutexIdT mutexId)
{
    ClRcT rc = CL_OK;
    ClOsalMutexT *pMutex = (ClOsalMutexT*)mutexId;
    ClInt32T err=0;

    nullChkRet(pMutex);

    CL_FUNC_ENTER();   
retry:
    err = sem_wait(&pMutex->shared_lock.sem.posSem);
    if(err < 0 )
    {
        if(errno == EINTR)
            goto retry;
        rc = CL_OSAL_RC(CL_ERR_LIBRARY);
        clDbgCodeError(rc,("sem_wait returned [%s]\n",strerror(errno)));
    }

    CL_FUNC_EXIT();
    return (CL_OK);
}

ClRcT 
cosPosixMutexTryLock (ClOsalMutexIdT mutexId)
{
    ClOsalMutexT *pMutex = (ClOsalMutexT*)mutexId;
    ClInt32T err=0;

    if(!pMutex)
        return CL_OSAL_RC(CL_ERR_NULL_POINTER);

    CL_FUNC_ENTER();   
retry:
    err = sem_trywait(&pMutex->shared_lock.sem.posSem);
    if(err < 0 )
    {
        if(errno == EINTR)
            goto retry;
        if(errno == EAGAIN)
            return CL_OSAL_RC(CL_ERR_TRY_AGAIN);
        return CL_OSAL_RC(CL_ERR_LIBRARY);
    }

    CL_FUNC_EXIT();
    return (CL_OK);
}

/**************************************************************************/

ClRcT 
cosPosixMutexUnlock (ClOsalMutexIdT mutexId)
{
    ClRcT rc = CL_OK;
    ClUint32T retCode = 0;
    ClOsalMutexT* pMutex = (ClOsalMutexT*) mutexId;
    ClInt32T err = 0;
   
    CL_FUNC_ENTER();
    
    if (NULL == pMutex)
	{
        CL_DEBUG_PRINT (CL_DEBUG_ERROR, ("Mutex Unlock : FAILED, mutex is NULL (used after delete?)"));
        clDbgPause();
        retCode = CL_OSAL_RC(CL_ERR_NULL_POINTER);
        CL_FUNC_EXIT();
        return(retCode);
	}

retry:
    err = sem_post(&pMutex->shared_lock.sem.posSem);
    if(err < 0)
    {
        if(errno == EINTR)
        {
            goto retry;
        }
        rc = CL_OSAL_RC(CL_ERR_LIBRARY);
        clDbgCodeError(rc,("sem_post unlock returned [%s]\n",strerror(errno)));
    }

    /* Nobody wants to know whenever ANY mutex is locked/unlocked; now if this was a particular mutex...
       CL_DEBUG_PRINT (CL_DEBUG_TRACE, ("\nMutex Unlock : DONE")); */
    CL_FUNC_EXIT();
    return (rc);
}


ClRcT 
cosPosixMutexDestroy (ClOsalMutexT *pMutex)
{
    ClRcT rc = CL_OK;
    ClUint32T retCode = 0;
    CL_FUNC_ENTER();
    if (NULL == pMutex)
	{
        CL_DEBUG_PRINT (CL_DEBUG_ERROR, ("Mutex Destroy failed, mutex is NULL (double delete?)"));
        retCode = CL_OSAL_RC(CL_ERR_NULL_POINTER);
        CL_FUNC_EXIT();
        return(retCode);
	}

    ClInt32T err = sem_destroy(&pMutex->shared_lock.sem.posSem);
    if(err < 0 )
    {
        rc = CL_OSAL_RC(CL_ERR_LIBRARY);
        clDbgCodeError(CL_ERR_LIBRARY,("sem_destroy() returned [%s]\n",strerror(errno)));
    }

    CL_FUNC_EXIT();
    return (rc);
}

/*******************************************************************/

ClRcT
cosPosixSemCreate (ClUint8T* pName, ClUint32T count, ClOsalSemIdT* pSemId)
{
    ClInt32T rc = CL_OK;
    sem_t *pSem = NULL;
    ClInt32T semValue = 0;
    ClInt32T i = 0;
    
    nullChkRet(pSemId);
    nullChkRet(pName);

    CL_FUNC_ENTER();
    if ((count == 0) || count > CL_SEM_MAX_VALUE)
    {
        rc = CL_OSAL_RC(CL_ERR_INVALID_PARAMETER);
        clDbgCodeError(rc, ("Number of semaphores to create (count) [%d] must be between [1] and [%d]", count,  CL_SEM_MAX_VALUE));
        CL_FUNC_EXIT();
        return(rc);
    }

    pSem = sem_open((ClCharT*)pName, O_CREAT, 0777, count);
    if(pSem == SEM_FAILED)
    {
        rc = CL_OSAL_RC(CL_ERR_LIBRARY); 
        clDbgCodeError(rc, ("Failed at sem_open. system error code %d.\n", errno));
        return rc;
    }

    sem_getvalue(pSem, &semValue);

    if (semValue < count)
    {
        for (i = semValue; i < count; ++i)
        {
            sem_post(pSem);
        }
    }
    else
    {
        for (i = count; i < semValue; ++i)
        {
            sem_wait(pSem);
        }
    }

    *pSemId = *(ClOsalSemIdT*)&pSem;

    CL_FUNC_EXIT();
    return (rc);
}

/**************************************************************************/
ClRcT
cosPosixSemIdGet(ClUint8T* pName, ClOsalSemIdT* pSemId)
{
    ClUint32T count = 0;
    sem_t *pSem;

    nullChkRet(pSemId);
    nullChkRet(pName);

    CL_FUNC_ENTER();

    pSem = sem_open((ClCharT*)pName, O_CREAT, 0666, count);

    *pSemId = *(ClOsalSemIdT*)&pSem;

    CL_FUNC_EXIT();
    return (CL_OK);
}

/**************************************************************************/

ClRcT
cosPosixSemLock (ClOsalSemIdT semId)
{
    int retCode = 0;
    sem_t *pSem = *(sem_t**)&semId;
   
    CL_FUNC_ENTER();

retry:
    retCode = sem_wait(pSem);
    if(retCode == -1 && EINTR == errno)
        goto retry;

    sysErrnoChkRet(retCode);

    CL_FUNC_EXIT();
    return (CL_OK);
}

/**************************************************************************/
  
ClRcT
cosPosixSemTryLock (ClOsalSemIdT semId)
{
    ClInt32T retCode = CL_OK;
    sem_t *pSem = *(sem_t**)&semId;
 
    CL_FUNC_ENTER();
    retCode = sem_trywait(pSem);
 
    if (-1 == retCode)
    {
      int err = errno;
      if (err == EAGAIN)
          return(CL_OSAL_RC(CL_OSAL_ERR_SEM_LOCK));  /* Taking the lock failed because the lock is taken by someone else */
      else
          sysRetErrChkRet(err);  /* Some unexpected error occurred */
    }
 
    CL_FUNC_EXIT();
    return (CL_OK);  /* It worked, lock was taken */
}

/**************************************************************************/
ClRcT
cosPosixSemUnlock (ClOsalSemIdT semId)
{
    sem_t *pSem = *(sem_t**)&semId;
  
    CL_FUNC_ENTER();
    sysErrnoChkRet(sem_post(pSem));
    CL_FUNC_EXIT();

    return (CL_OK);
}

/**************************************************************************/
ClRcT
cosPosixSemValueGet(ClOsalSemIdT semId, ClUint32T* pSemValue)
{
    sem_t *pSem = *(sem_t**)&semId;
   
    nullChkRet(pSemValue);

    CL_FUNC_ENTER();
    
    sysErrnoChkRet(sem_getvalue(pSem, (ClInt32T*)pSemValue));

    CL_FUNC_EXIT();
    return (CL_OK);
}

/**************************************************************************/
ClRcT
cosPosixSemDelete(ClOsalSemIdT semId)
{ 
    sem_t *pSem = *(sem_t**)&semId;

    CL_FUNC_ENTER();
    sysErrnoChkRet(sem_close(pSem));
    CL_FUNC_EXIT();

    return (CL_OK);
}

/**************************************************************************/

ClRcT
cosPosixShmCreate(ClUint8T* pName, ClUint32T size, ClOsalShmIdT* pShmId)
{
    ClRcT rc = CL_OSAL_RC(CL_ERR_NOT_SUPPORTED);
    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nIn POSIX this operatoin is not supported. error code [0x%x].", rc));

    return rc; 
}

/**************************************************************************/
ClRcT
cosPosixShmIdGet(ClUint8T* pName, ClOsalShmIdT* pShmId)
{
    ClRcT rc = CL_OSAL_RC(CL_ERR_NOT_SUPPORTED);
    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nIn POSIX this operatoin is not supported. error code [0x%x].", rc));

    return rc; 
}

/**************************************************************************/

ClRcT
cosPosixShmDelete(ClOsalShmIdT shmId)
{
    ClRcT rc = CL_OSAL_RC(CL_ERR_NOT_SUPPORTED);
    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nIn POSIX this operatoin is not supported. error code [0x%x].", rc));

    return rc; 
}

/**************************************************************************/
ClRcT
cosPosixShmAttach(ClOsalShmIdT shmId,void* pInMem, void** ppOutMem)
{
    ClRcT rc = CL_OSAL_RC(CL_ERR_NOT_SUPPORTED);
    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nIn POSIX this operatoin is not supported. error code [0x%x].", rc));

    return rc; 
}

/**************************************************************************/
ClRcT
cosPosixShmDetach(void* pMem)
{
    ClRcT rc = CL_OSAL_RC(CL_ERR_NOT_SUPPORTED);
    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nIn POSIX this operatoin is not supported. error code [0x%x].", rc));

    return rc; 
}

/**************************************************************************/
ClRcT
cosPosixShmSecurityModeSet(ClOsalShmIdT shmId,ClUint32T mode)
{
    ClRcT rc = CL_OSAL_RC(CL_ERR_NOT_SUPPORTED);
    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nIn POSIX this operatoin is not supported. error code [0x%x].", rc));

    return rc; 
}

/**************************************************************************/
ClRcT
cosPosixShmSecurityModeGet(ClOsalShmIdT shmId,ClUint32T* pMode)
{
    ClRcT rc = CL_OSAL_RC(CL_ERR_NOT_SUPPORTED);
    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nIn POSIX this operatoin is not supported. error code [0x%x].", rc));

    return rc; 
}

/**************************************************************************/
ClRcT
cosPosixShmSizeGet(ClOsalShmIdT shmId,ClUint32T* pSize)
{
    ClRcT rc = CL_OSAL_RC(CL_ERR_NOT_SUPPORTED);
    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nIn POSIX this operatoin is not supported. error code [0x%x].", rc));

    return rc; 
}
