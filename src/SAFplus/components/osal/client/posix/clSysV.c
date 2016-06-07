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
/*******************************************************************************
 * ModuleName  : osal
 * File        : posix.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This module implements OS abstraction layer                            
 **************************************************************************/
/* INCLUDES */
#include <pthread.h>
#ifndef SOLARIS_BUILD
#include <bits/local_lim.h>
#endif
#include <errno.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <clCommon.h>
#include <clCommonErrors.h>
#include <clTimerApi.h>
#include <clOsalApi.h>
#include <clHeapApi.h>
#include <clOsalErrors.h>
#include "../osal.h"
#include <clCksmApi.h>
#include <clDebugApi.h>
#include <clLogUtilApi.h>
#include <clDbg.h>
#include <clBitApi.h>
#include <clLogApi.h>
#include "clOsalCommon.h"
#include "clSysV.h"

#ifndef SOLARIS_BUILD
#include <execinfo.h>
#endif

#ifdef _POSIX_PRIORITY_SCHEDULING
#include <sched.h>
#endif
#include <clEoApi.h>


static pthread_mutex_t gClSemAccessLock = PTHREAD_MUTEX_INITIALIZER;


/**************************************************************************/

ClRcT cosSysvProcessSharedSemInit(ClOsalMutexT *pMutex, ClUint8T *pKey, ClUint32T keyLen, ClInt32T value)
{
    ClInt32T semId = 0;
    ClUint32T semKey = 0;
    ClRcT rc = CL_OK;
    ClUint32T flags = 0666;
    ClInt32T err = 0;

    nullChkRet(pKey);
    if(keyLen == 0)
    {
        clDbgCodeError(CL_ERR_INVALID_PARAMETER,("Invalid keylen [%d]\n",keyLen));
        return CL_OSAL_RC(CL_ERR_INVALID_PARAMETER);
    }

    rc = clCrc32bitCompute(pKey, keyLen, &semKey, NULL);
    CL_ASSERT(rc == CL_OK && semKey );

    pthread_mutex_lock(&gClSemAccessLock);
    retry:
    semId = semget(semKey, 1, flags);
    if(semId < 0 )
    {
        if(errno == EINTR)
            goto retry;
        if(errno == ENOENT)
        {
            flags |= IPC_CREAT;
            goto retry;
        }
        pthread_mutex_unlock(&gClSemAccessLock);
        rc = CL_OSAL_RC(CL_ERR_LIBRARY);
        goto out;
    }
    pthread_mutex_unlock(&gClSemAccessLock);
    if( (flags & IPC_CREAT) )
    {
        CosSemCtl_t arg = {0};
        arg.val = value;
        retry1:
        err = semctl(semId,0,SETVAL,arg);
        if(err < 0 )
        {
            if(errno == EINTR)
                goto retry1;
            rc = CL_OSAL_RC(CL_ERR_LIBRARY);
            goto out;
        }
    }
    pMutex->shared_lock.sem.semId = semId;
    pMutex->shared_lock.sem.numSems = 1;
    rc = CL_OK;

    out:
    return rc;
}


ClRcT
cosSysvMutexValueSet(ClOsalMutexIdT mutexId, ClInt32T value)
{
    ClOsalMutexT *pMutex = (ClOsalMutexT *) mutexId;
    ClRcT rc = CL_OK;
    ClInt32T semId = pMutex->shared_lock.sem.semId;
    ClInt32T err = 0;
    CosSemCtl_t arg = {0};
    
    CL_FUNC_ENTER();

    if(value >= SEMVMX)
    {
        clLogError(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Error in setting sem value to [%d]\n", value);
        rc = CL_OSAL_RC(CL_ERR_INVALID_PARAMETER);
        goto out;
    }
    arg.val = value;
    err = semctl(semId, 0, SETVAL, arg);
    if(err < 0 )
    {
        clLogError(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Mutex value set- semctl error: [%s]\n", strerror(errno));
        rc = CL_OSAL_RC(CL_ERR_LIBRARY);
        goto out;
    }

    CL_FUNC_EXIT();
    out:
    return rc;
}

ClRcT
cosSysvMutexValueGet(ClOsalMutexIdT mutexId, ClInt32T *pValue)
{
    ClOsalMutexT *pMutex = (ClOsalMutexT*) mutexId;
    ClRcT rc = CL_OK;
    ClInt32T val = 0;
    ClInt32T semId = pMutex->shared_lock.sem.semId;
    
    CL_FUNC_ENTER();

    val = semctl(semId, 0, GETVAL);
    if(val < 0 )
    {
        clLogError(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Mutex value get- semctl failed with [%s]\n", strerror(errno));
        rc = CL_OSAL_RC(CL_ERR_LIBRARY);
        goto out;
    }
    *pValue = val;

    CL_FUNC_EXIT();

    out:
    return rc;
}

/**************************************************************************/

ClRcT 
__cosSysvMutexLock (ClOsalMutexIdT mutexId, ClBoolT verbose)
{
    ClRcT rc = CL_OK;
    ClOsalMutexT *pMutex = (ClOsalMutexT*)mutexId;
    static struct sembuf sembuf = {0,-1,SEM_UNDO};
    ClInt32T err=0;

    CL_FUNC_ENTER();   
retry:
    err = semop(pMutex->shared_lock.sem.semId,&sembuf,1);
    if(err < 0 )
    {
        if(errno == EINTR)
            goto retry;
        rc = CL_OSAL_RC(CL_ERR_LIBRARY);
        if(verbose)
        {
            clDbgCodeError(rc,("semop returned [%s]\n",strerror(errno)));
        }
    }

    CL_FUNC_EXIT();
    return (rc);
}

ClRcT 
cosSysvMutexLock (ClOsalMutexIdT mutexId)
{
    return __cosSysvMutexLock(mutexId, CL_TRUE);
}

/**************************************************************************/

ClRcT 
__cosSysvMutexUnlock (ClOsalMutexIdT mutexId, ClBoolT verbose)
{
    ClRcT rc = CL_OK;
    ClOsalMutexT* pMutex = (ClOsalMutexT*) mutexId;
    static struct sembuf sembuf = {0, 1, SEM_UNDO };
    ClInt32T err = 0;
   
    CL_FUNC_ENTER();
    
retry:
    err = semop(pMutex->shared_lock.sem.semId,&sembuf,1);
    if(err < 0)
    {
        if(errno == EINTR)
        {
            goto retry;
        }
        rc = CL_OSAL_RC(CL_ERR_LIBRARY);
        if(verbose)
        {
            clDbgCodeError(rc,("semop unlock returned [%s]\n",strerror(errno)));
        }
    }

    CL_FUNC_EXIT();
    return (rc);
}

ClRcT 
cosSysvMutexUnlock (ClOsalMutexIdT mutexId)
{
    return __cosSysvMutexUnlock(mutexId, CL_TRUE);
}

ClRcT 
cosSysvMutexDestroy (ClOsalMutexT *pMutex)
{
    ClRcT rc = CL_OK;
    ClInt32T err;
    CL_FUNC_ENTER();

    err = semctl(pMutex->shared_lock.sem.semId,0,IPC_RMID,0);
    if(err < 0 )
    {
        rc = CL_OSAL_RC(CL_ERR_LIBRARY);
        clDbgCodeError(CL_ERR_LIBRARY,("semctl returned [%s]\n",strerror(errno)));
    }

    CL_FUNC_EXIT();
    return (rc);
}

/*******************************************************************/
ClRcT
cosSysvSemCreate (ClUint8T* pName, ClUint32T count, ClOsalSemIdT* pSemId)
{
    ClInt32T retCode = CL_OK;
    CosSemCtl_t semArg = {0};
    ClUint32T len = 0;
    ClUint32T key = 0;
    ClInt32T semId = -1;

    nullChkRet(pSemId);
    nullChkRet(pName);

    CL_FUNC_ENTER();
    if ((count == 0) || count > CL_SEM_MAX_VALUE)
    {
        retCode = CL_OSAL_RC(CL_ERR_INVALID_PARAMETER);
        clDbgCodeError(retCode, ("Number of semaphores to create (count) [%d] must be between [1] and [%d]", count,  CL_SEM_MAX_VALUE));
        CL_FUNC_EXIT();
        return(retCode);
    }

    len = (ClUint32T)strlen ((ClCharT*)pName);

#if 0 /* Stone: why this limitation? */
    if(len > 20)
    if(len > 256)
    {
        clLogInfo(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Sanity check, semaphore name length is suspiciously long");
        retCode = CL_OSAL_RC(CL_OSAL_ERR_NAME_TOO_LONG);
        CL_FUNC_EXIT();
        return(retCode);
    }
#endif

    if(len > 256)
    {
        clLogInfo(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Sanity check, semaphore name [%s] is suspiciously long",pName);
    }

    retCode = (ClInt32T)clCrc32bitCompute (pName, len, &key, NULL);
    CL_ASSERT(retCode == CL_OK); /* There is no possible error except for pName == NULL, which I've already checked, so don't check the retCode */


    sysErrnoChkRet(semId = semget ((key_t)key, (int)count, IPC_CREAT|0666));
      
    semArg.val = (int)count;

    /* Initialize all the semaphores to 0.  This should never fail, because I just created the semaphores */
    sysErrnoChkRet(semctl (semId, 0, SETVAL, semArg));
    
    *pSemId = (ClOsalSemIdT)semId;

    CL_FUNC_EXIT();
    return (CL_OK);
}

/**************************************************************************/
ClRcT
cosSysvSemIdGet(ClUint8T* pName, ClOsalSemIdT* pSemId)
{
    ClUint32T key = 0;
    ClInt32T semId = -1;
    ClUint32T len = 0;
    ClUint32T count = 0;
    ClUint32T retCode = CL_OK;
    int       err;

    nullChkRet(pSemId);
    nullChkRet(pName);

    CL_FUNC_ENTER();

    len = (ClUint32T)strlen ((ClCharT*)pName);

    if(len > 256)
    {
        clLogInfo(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Sanity check, semaphore name [%s] is suspiciously long",pName);
    }

    retCode = clCrc32bitCompute (pName, len, &key, NULL);
    CL_ASSERT(retCode == CL_OK); /* There is no possible error except for pName == NULL, which I've already checked, so don't check the retCode */

    semId = semget ((key_t)key, (int)count, 0660);
    err   = errno;
    
    if(semId == -1)
    {
      if (err == ENOENT) 
        {
          clLogInfo(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"Semaphore [%s], id [%u] accessed but not created.  Creating it now",pName,key);
          semId = semget ((key_t)key, (int)count, IPC_CREAT|0666);
        }
    }

    sysErrnoChkRet(semId);

    *pSemId = (ClOsalSemIdT)semId;

    CL_FUNC_EXIT();
    return (CL_OK);
}

/**************************************************************************/

ClRcT
cosSysvSemLock (ClOsalSemIdT semId)
{
    int retCode = 0;
   
    CL_FUNC_ENTER();
    struct sembuf semLock[] = {{0, -1, SEM_UNDO}};

    /* IPC_NOWAIT is not specified so that we can wait until the resource
     * is available.
     */

    do
    {
        retCode = semop ((int)semId, semLock, 1);

    /* If the semop system calls fails with errno=4, then it it because of
     * the timers interrupt. Call semop again and again.
     */
    }    while((-1 == retCode) && (EINTR == errno));

    sysErrnoChkRet(retCode);

    CL_FUNC_EXIT();
    return (CL_OK);
}

/**************************************************************************/
  
ClRcT
cosSysvSemTryLock (ClOsalSemIdT semId)
{
    ClInt32T retCode = CL_OK;
    struct sembuf semLock[] = {{0, -1, SEM_UNDO | IPC_NOWAIT}};
 
    CL_FUNC_ENTER();
    retCode = semop ((int)semId, semLock, 1);
    
    if (-1 == retCode)
    {
      int err = errno;
      if (err == EAGAIN) return(CL_OSAL_RC(CL_OSAL_ERR_SEM_LOCK));  /* Taking the lock failed because the lock is taken by someone else */
      else
        {
          sysRetErrChkRet(err);  /* Some unexpected error occurred */
        }
    }
 
    CL_FUNC_EXIT();
    return (CL_OK);  /* It worked, lock was taken */
}

/**************************************************************************/
ClRcT
cosSysvSemUnlock (ClOsalSemIdT semId)
{
    struct sembuf semUnlock[] = {{0, 1, SEM_UNDO}};
  
    CL_FUNC_ENTER();
    sysErrnoChkRet(semop ((int)semId, semUnlock, 1));
    CL_FUNC_EXIT();

    return (CL_OK);
}

/**************************************************************************/
ClRcT
cosSysvSemValueGet(ClOsalSemIdT semId, ClUint32T* pSemValue)
{
    ClInt32T count = -1;
   
    nullChkRet(pSemValue);

    CL_FUNC_ENTER();
    
    sysErrnoChkRet(count = semctl ((int)semId, 0, GETVAL, 0));

    *pSemValue = (ClUint32T)count;

    CL_FUNC_EXIT();
    return (CL_OK);
}

/**************************************************************************/
ClRcT
cosSysvSemDelete(ClOsalSemIdT semId)
{ 
    CL_FUNC_ENTER();
    sysErrnoChkRet(semctl ((int)semId, 0, IPC_RMID, 0));
    CL_FUNC_EXIT();

    return (CL_OK);
}

/**************************************************************************/
ClRcT
cosSysvShmCreate(ClUint8T* pName, ClUint32T size, ClOsalShmIdT* pShmId)
{
    struct shmid_ds shmPerm ;
    ClInt32T retCode = CL_OK;
    ClUint32T len = 0;
    ClUint32T key = 0;
    ClInt32T shmId = -1;
   
    CL_FUNC_ENTER();

    if(NULL == pShmId)
    {
        clLogInfo(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nShared Memory Create: FAILED");
        retCode = CL_OSAL_RC(CL_ERR_NULL_POINTER);
        CL_FUNC_EXIT();
        return(retCode);
    }
    
    if(NULL == pName)
    {
        clLogInfo(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nShared Memory Create: FAILED");
        retCode = CL_OSAL_RC(CL_ERR_NULL_POINTER);
        CL_FUNC_EXIT();
        return(retCode);
    }
        
    len = (ClUint32T)strlen ((ClCharT*) pName);

    retCode = (ClInt32T)clCrc32bitCompute (pName, len, &key, NULL);

    if(CL_OK != retCode)
    {
        clLogInfo(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nShared Memory Create: FAILED");
        retCode = CL_OSAL_RC(CL_OSAL_ERR_SHM_CREATE);
        CL_FUNC_EXIT();
        return(retCode);
    }

    shmId = shmget ((key_t)key, size, (0666 | IPC_CREAT));

    if(shmId < 0)
    {
        clLogInfo(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nShared Memory Create: FAILED");
        retCode = CL_OSAL_RC(CL_OSAL_ERR_SHM_CREATE);
        CL_FUNC_EXIT();
        return(retCode);
    }
   
    
    retCode = shmctl (shmId, IPC_STAT, &shmPerm);

    if(0 != retCode)
    {
        retCode = shmctl (shmId, IPC_RMID, NULL);

        if(0 != retCode)
        {
            /*debug messages if any */
        }

        clLogInfo(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nShared Memory Create: FAILED");
        retCode = CL_OSAL_RC(CL_OSAL_ERR_SHM_CREATE);
        CL_FUNC_EXIT();
        return(retCode);
    }
        
    shmPerm.shm_perm.uid = getuid();
    shmPerm.shm_perm.gid = getgid();

    retCode = shmctl (shmId, IPC_SET, &shmPerm);

    if(0 != retCode)
    {
        retCode = shmctl (shmId, IPC_RMID, NULL);

        if(0 != retCode)
        {
            /*debug messages if any */
        }

        clLogInfo(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nShared Memory Create: FAILED");
        retCode = CL_OSAL_RC(CL_OSAL_ERR_SHM_CREATE);
        CL_FUNC_EXIT();
        return(retCode);
    }

    *pShmId = (ClOsalSemIdT)shmId;
    
    clLogTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nShared Memory Create: DONE");
    CL_FUNC_EXIT();
    return (CL_OK);
}

/**************************************************************************/
ClRcT
cosSysvShmIdGet(ClUint8T* pName, ClOsalShmIdT* pShmId)
{
    ClUint32T key = 0;
    ClUint32T len = 0;
    ClUint32T size = 0;
    ClInt32T shmId = 0;
    ClRcT retCode = CL_OK;
   
    CL_FUNC_ENTER();
    if(NULL == pShmId)
    {
        clLogInfo(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nShared Memory ID Get: FAILED");
        retCode = CL_OSAL_RC(CL_ERR_NULL_POINTER);
        CL_FUNC_EXIT();
        return(retCode);
    }
    
    if(NULL == pName)
    {
        clLogInfo(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nShared Memory ID Get: FAILED");
        retCode = CL_OSAL_RC(CL_ERR_NULL_POINTER);
        CL_FUNC_EXIT();
        return(retCode);
    }

    len = (ClUint32T)strlen ((ClCharT*)pName);

    retCode = clCrc32bitCompute (pName, len, &key, NULL);

    if(CL_OK != retCode)
    {
        clLogInfo(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nShared Memory ID Get: FAILED");
        retCode = CL_OSAL_RC(CL_OSAL_ERR_SHM_ID_GET);
        CL_FUNC_EXIT();
        return(retCode);
    }

    shmId = shmget ((key_t)key, size, (0666 | IPC_CREAT));

    if(shmId < 0)
    {
        clLogInfo(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nShared Memory ID Get: FAILED");
        retCode = CL_OSAL_RC(CL_OSAL_ERR_SHM_ID_GET);
        CL_FUNC_EXIT();
        return(retCode);
    }

    *pShmId = (ClOsalShmIdT)shmId;

    clLogTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nShared Memory ID Get: DONE");
    CL_FUNC_EXIT();
    return (CL_OK);
}

/**************************************************************************/

ClRcT
cosSysvShmDelete(ClOsalShmIdT shmId)
{
    ClUint32T retCode = CL_OK;

    CL_FUNC_ENTER();
    retCode = (ClUint32T)(shmctl ((int)shmId, IPC_RMID, NULL));

    if(0 != retCode)
    {
        clLogInfo(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nShared Memory Delete: FAILED");
        retCode = CL_OSAL_RC(CL_OSAL_ERR_SHM_DELETE);
        CL_FUNC_EXIT();
        return(retCode);
    }

    clLogTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nShared Memory Delete: DONE");
    CL_FUNC_EXIT();
    return (CL_OK);
}

/**************************************************************************/
ClRcT
cosSysvShmAttach(ClOsalShmIdT shmId,void* pInMem, void** ppOutMem)
{
    ClRcT retCode = 0;
    void* pShared = NULL;
        
    CL_FUNC_ENTER();
    if(NULL == ppOutMem)
    {
        clLogInfo(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nShared Memory Attach: FAILED");
        retCode = CL_OSAL_RC(CL_ERR_NULL_POINTER);
        CL_FUNC_EXIT();
        return(retCode);
    }
    
    pShared = shmat ((int)shmId, pInMem, 0);

    if((void*)-1 == pShared)
    {
        clLogInfo(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nShared Memory Attach: FAILED");
        retCode = CL_OSAL_RC(CL_OSAL_ERR_SHM_ATTACH);
        CL_FUNC_EXIT();
        return(retCode);
    }
    else
    {
        *ppOutMem = pShared;
    }
    
    clLogTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nShared Memory Attach: DONE");
    CL_FUNC_EXIT();
    return (CL_OK);
}

/**************************************************************************/
ClRcT
cosSysvShmDetach(void* pMem)
{
    ClInt32T retCode =  CL_OK;
   
    CL_FUNC_ENTER();
    if(NULL == pMem)
    {
        clLogTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nShared Memory Detach: FAILED");
        retCode = CL_OSAL_RC(CL_ERR_NULL_POINTER);
        CL_FUNC_EXIT();
        return(retCode);
    }

    retCode = shmdt (pMem);

    if(0 != retCode)
    {
        clLogTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nShared Memory Detach: FAILED");
        retCode = CL_OSAL_RC(CL_OSAL_ERR_SHM_DETACH);
        CL_FUNC_EXIT();
        return(retCode);
    }
    
    clLogTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nShared Memory Detach: DONE");
    CL_FUNC_EXIT();
    return (CL_OK);
}

/**************************************************************************/
ClRcT
cosSysvShmSecurityModeSet(ClOsalShmIdT shmId,ClUint32T mode)
{
    struct shmid_ds shmPerm ;
    ClInt32T retCode = CL_OK;

    CL_FUNC_ENTER();

    /* Basically we dont want to allow RWX on User,Group and others (Nothing
     * greater than 111 111 111 (0777). Eight is for calculating the number of
     * bytes.9 is the number of bits that will used for RWX RWX RWX for 
     * User Group Others.
     */

    if(mode & (((~(1 << ((sizeof(int) *8)- 1))))<< 9))
    {
        clLogTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nShared Memory SecurityModeSet: FAILED");
        retCode = CL_OSAL_RC(CL_OSAL_ERR_SHM_MODE_SET);
        CL_FUNC_EXIT();
        return(retCode); 
    }

    /* Get the current values set and modify it */
    retCode = shmctl ((int)shmId, IPC_STAT, &shmPerm);

    if(0 != retCode)
    {
        clLogTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nShared Memory SecurityModeSet: FAILED");
        retCode = CL_OSAL_RC(CL_OSAL_ERR_SHM_MODE_SET);
        CL_FUNC_EXIT();
        return(retCode);
    }
  
    shmPerm.shm_perm.mode = (unsigned short int)mode;

    retCode = shmctl ((int)shmId, IPC_SET, &shmPerm);

    if(0 != retCode)
    {
        clLogTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nShared Memory SecurityModeSet: FAILED");
        retCode = CL_OSAL_RC(CL_OSAL_ERR_SHM_MODE_SET);
        CL_FUNC_EXIT();
        return(retCode);
    }
 
    clLogTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nShared Memory SecurityModeSet: DONE");
    CL_FUNC_EXIT();
    return (CL_OK);
}

/**************************************************************************/
ClRcT
cosSysvShmSecurityModeGet(ClOsalShmIdT shmId,ClUint32T* pMode)
{
    struct shmid_ds shmPerm ;
    ClInt32T retCode = CL_OK;

    CL_FUNC_ENTER();
    if(NULL == pMode)
    {
        clLogTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nShared Memory SecurityModeGet: FAILED");
        retCode = CL_OSAL_RC(CL_ERR_NULL_POINTER);
        CL_FUNC_EXIT();
        return(retCode);
    }

    /* Get the current values set and modify it */
    retCode = shmctl ((int)shmId, IPC_STAT, &shmPerm);

    if(0 != retCode)
    {
        clLogTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nShared Memory SecurityModeGet: FAILED");
        retCode = CL_OSAL_RC(CL_OSAL_ERR_SHM_MODE_GET);
        CL_FUNC_EXIT();
        return(retCode);
    }
       
    *pMode = shmPerm.shm_perm.mode;
    
    clLogTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nShared Memory SecurityModeGet: DONE");
    CL_FUNC_EXIT();
    return (CL_OK);
}

/**************************************************************************/
ClRcT
cosSysvShmSizeGet(ClOsalShmIdT shmId,ClUint32T* pSize)
{
    struct shmid_ds shmSize ;
    ClInt32T retCode = CL_OK;

    CL_FUNC_ENTER();
    if(NULL == pSize)
    {
        clLogTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nShared Memory SecurityModeGet: FAILED");
        retCode = CL_OSAL_RC(CL_ERR_NULL_POINTER);
        CL_FUNC_EXIT();
        return(retCode);
    }

    /* Get the current values set and modifiy it */
    retCode = shmctl ((int)shmId, IPC_STAT, &shmSize);

    if(0 != retCode)
    {
        clLogTrace(CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,"\nShared Memory SecurityModeGet: FAILED");
        retCode = CL_OSAL_RC(CL_OSAL_ERR_SHM_SIZE);
        CL_FUNC_EXIT();
        return(retCode);
    }

    *pSize = (ClUint32T)shmSize.shm_segsz;

    CL_FUNC_EXIT();
    return (CL_OK);
}
