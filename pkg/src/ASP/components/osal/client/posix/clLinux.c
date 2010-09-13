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
#include <pthread.h>
#ifndef SOLARIS_BUILD
#include <bits/local_lim.h>
#endif
#include <sched.h>
#include <semaphore.h>
#include <sys/ipc.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/sem.h>
#include <sys/shm.h>
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
/*#include <clLogApi.h>*/
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
#include "clCommonCos.h"
#include "clSysV.h"
#include "clPosix.h"

#ifdef CL_OSAL_DEBUG
#include "clPosixDebug.h"
#endif

#ifdef _POSIX_PRIORITY_SCHEDULING
#include <sched.h>
#endif
#include <clEoApi.h>

#include "backtrace_arch.h"

char logBuffer[LOG_BUFFER_SIZE];
struct sigaction oldact[_NSIG];
osalFunction_t gOsalFunction = {0};
CosTaskControl_t gTaskControl;
cosCompCfgInit_t sCosConfig={CL_OSAL_MIN_STACK_SIZE};

/**************************************************************************/

static ClRcT cosPosixInit (void);


ClRcT clOsalInitialize(const ClPtrT pConfig)
{
    #ifdef MORE_CODE_COVERAGE
        clCodeCovInit();
    #endif
   /* Determine Byte Endiannes*/
    clBitBlByteEndianGet();
    return cosPosixInit();
}

/**************************************************************************/

static ClRcT
cosProcessCreate(ClOsalProcessFuncT fpFunction, 
                      void* processArg, 
                      ClOsalProcessFlagT creationFlags, 
                      ClOsalPidT* pProcessId)
{
ClInt32T pid = -1;
ClInt32T sid = -1;
ClRcT retCode = 0;

CL_FUNC_ENTER();
nullChkRet(fpFunction);
nullChkRet(pProcessId);

pid = fork();

if(pid < 0)
{
    CL_DEBUG_PRINT (CL_DEBUG_INFO,("\nProcess Create: FAILED"));
    retCode = CL_OSAL_RC(CL_OSAL_ERR_PROCESS_CREATE);
    CL_FUNC_EXIT();
    return(retCode);
}

if(0 == pid)
{
    if(creationFlags & CL_OSAL_PROCESS_WITH_NEW_SESSION)
    {
        sid = setsid ();

        if(-1 == sid)
        {
            /*
             * Process creation failed though fork was successful.
             * Exit out of the process 
             */

            exit(0);
        }
    }

    if(creationFlags & CL_OSAL_PROCESS_WITH_NEW_GROUP)
    {
        pid = getpid();

        /* Set the process group id to its own pid */
        setpgid (pid, 0);
    }
    CL_DEBUG_PRINT (CL_DEBUG_INFO,("\nProcess Create: Function invoked"));
    fpFunction(processArg);
    exit(0);
}
else
{
    *pProcessId = (ClOsalPidT)pid;
}

CL_FUNC_EXIT();
return (CL_OK);
}

/**************************************************************************/
static ClRcT 
cosProcessSharedMutexInit (ClOsalMutexT* pMutex, ClOsalSharedMutexFlagsT flags, ClUint8T *pKey, ClUint32T keyLen, ClInt32T value)
{
    ClRcT rc = CL_OK;

    nullChkRet(pMutex);
    CL_FUNC_ENTER();  

    pMutex->flags = flags;

    if( (flags & CL_OSAL_SHARED_SYSV_SEM) ) 
    {
        rc = cosSysvProcessSharedSemInit(pMutex, pKey, keyLen, value);
    }
    else if( (flags & CL_OSAL_SHARED_POSIX_SEM) )
    {
        rc = cosPosixProcessSharedSemInit(pMutex, pKey, keyLen, value);
    }
    else if( (flags & CL_OSAL_SHARED_NORMAL) )
    {
        rc = cosPosixProcessSharedFutexInit(pMutex);
    }


    if( rc != CL_OK)
    {
        clDbgCodeError(rc,("Process shared mutex init failed with [%#x]\n",rc));
    }

    CL_FUNC_EXIT();
    return (rc);
}


/**************************************************************************/

static ClRcT 
cosSharedMutexCreate (ClOsalMutexIdT* pMutexId, ClOsalSharedMutexFlagsT flags, ClUint8T *pKey, ClUint32T keyLen, ClInt32T value)
{
    ClUint32T retCode = 0;
    ClOsalMutexT *pMutex = NULL;
   
    nullChkRet(pMutexId);
    CL_FUNC_ENTER();
    pMutex = (ClOsalMutexT*) clHeapAllocate((ClUint32T)sizeof(ClOsalMutexT));
    if(NULL == pMutex)
    {
        CL_DEBUG_PRINT (CL_DEBUG_ERROR, ("Mutex creation failed, out of memory."));
        retCode = CL_OSAL_RC(CL_ERR_NO_MEMORY);
        CL_FUNC_EXIT();
        return(retCode);
    }
    
    retCode =(ClUint32T) cosProcessSharedMutexInit(pMutex, flags, pKey, keyLen, value);

    if(0 != retCode)
	{
        clHeapFree(pMutex);
        CL_DEBUG_PRINT (CL_DEBUG_ERROR, ("Mutex creation failed, error [0x%x]", retCode));
        CL_FUNC_EXIT();
        return(CL_OSAL_RC(CL_OSAL_ERR_MUTEX_CREATE));
	}

    *pMutexId = ((ClOsalMutexIdT)pMutex);
    CL_FUNC_EXIT();
    return (CL_OK);
}

static ClRcT
cosMutexValueSet(ClOsalMutexIdT mutexId, ClInt32T value)
{
    ClOsalMutexT *pMutex = (ClOsalMutexT *) mutexId;
    ClRcT rc = CL_OK;
    
    nullChkRet(pMutex);
    CL_FUNC_ENTER();

    if( (pMutex->flags & CL_OSAL_SHARED_SYSV_SEM) )
    {
        rc = cosSysvMutexValueSet(mutexId, value);
    }
    else if( (pMutex->flags & CL_OSAL_SHARED_POSIX_SEM) )
    {
        rc = cosPosixMutexValueSet(mutexId, value);
    }
    else
    {
        rc = CL_OSAL_RC(CL_ERR_NOT_SUPPORTED);
        clDbgCodeError(rc, ("Normal mutex dont support setval operations\n"));
        goto out;
    }

    CL_FUNC_EXIT();
    out:
    return rc;
}

static ClRcT
cosMutexValueGet(ClOsalMutexIdT mutexId, ClInt32T *pValue)
{
    ClOsalMutexT *pMutex = (ClOsalMutexT*) mutexId;
    ClRcT rc = CL_OK;
    
    nullChkRet(pMutex);
    nullChkRet(pValue);
    CL_FUNC_ENTER();

    if ( (pMutex->flags & CL_OSAL_SHARED_SYSV_SEM) )
    {
        rc = cosSysvMutexValueGet(mutexId, pValue);
    }
    else if ( (pMutex->flags & CL_OSAL_SHARED_POSIX_SEM) )
    {
        rc = cosPosixMutexValueGet(mutexId, pValue);
    }
    else
    {
        rc = CL_OSAL_RC(CL_ERR_NOT_SUPPORTED);
        clDbgCodeError(rc, ("Normal mutex dont support getval operations\n"));
        goto out;
    }
    CL_FUNC_EXIT();

    out:
    return rc;
}

/**************************************************************************/

static ClRcT 
cosMutexLock (ClOsalMutexIdT mutexId)
{
    ClRcT rc = CL_OK;
    ClOsalMutexT *pMutex = (ClOsalMutexT*)mutexId;

    nullChkRet(pMutex);
    CL_FUNC_ENTER();   
    if ( (pMutex->flags & CL_OSAL_SHARED_SYSV_SEM) )
    {
        rc = cosSysvMutexLock(mutexId);
    }
    else if ( (pMutex->flags & CL_OSAL_SHARED_POSIX_SEM) )
    {
        rc = cosPosixMutexLock(mutexId);
    }
    else if( (pMutex->flags & CL_OSAL_SHARED_NORMAL) )
    {
        int err = pthread_mutex_lock (&pMutex->shared_lock.mutex);
        if(err == EDEADLK)
            return CL_OSAL_RC(CL_ERR_INUSE);
        else if(err)
            return CL_OSAL_RC(CL_ERR_LIBRARY);
    }

    CL_FUNC_EXIT();
    return (CL_OK);
}

static ClRcT 
cosMutexTryLock (ClOsalMutexIdT mutexId)
{
    ClRcT rc = CL_OK;
    ClOsalMutexT *pMutex = (ClOsalMutexT*)mutexId;
    if(!pMutex)
        return CL_OSAL_RC(CL_ERR_NULL_POINTER);
    CL_FUNC_ENTER();   
    if ( (pMutex->flags & CL_OSAL_SHARED_SYSV_SEM) )
    {
        return CL_OSAL_RC(CL_ERR_NOT_SUPPORTED);
    }
    else if ( (pMutex->flags & CL_OSAL_SHARED_POSIX_SEM) )
    {
        rc = cosPosixMutexTryLock(mutexId);
    }
    else if( (pMutex->flags & CL_OSAL_SHARED_NORMAL) )
    {
        int err = 0;
        if((err = pthread_mutex_trylock(&pMutex->shared_lock.mutex) == EDEADLK))
            return CL_OSAL_RC(CL_ERR_INUSE);
        else if(err == EAGAIN || err == EBUSY)
            return CL_OSAL_RC(CL_ERR_TRY_AGAIN);
        else if(err)
            return CL_OSAL_RC(CL_ERR_LIBRARY);
    }

    CL_FUNC_EXIT();
    return rc;
}

/**************************************************************************/

static ClRcT 
cosMutexUnlock (ClOsalMutexIdT mutexId)
{
    ClRcT rc = CL_OK;
    ClUint32T retCode = 0;
    ClOsalMutexT* pMutex = (ClOsalMutexT*) mutexId;
   
    CL_FUNC_ENTER();
    
    if (NULL == pMutex)
	{
        CL_DEBUG_PRINT (CL_DEBUG_ERROR, ("Mutex Unlock : FAILED, mutex is NULL (used after delete?)"));
        clDbgPause();
        retCode = CL_OSAL_RC(CL_ERR_NULL_POINTER);
        CL_FUNC_EXIT();
        return(retCode);
	}

    if ( (pMutex->flags & CL_OSAL_SHARED_SYSV_SEM) )
    {
        rc = cosSysvMutexUnlock(mutexId);
    }
    else if ( (pMutex->flags & CL_OSAL_SHARED_POSIX_SEM) )
    {
        rc = cosPosixMutexUnlock(mutexId);
    }
    else if( (pMutex->flags & CL_OSAL_SHARED_NORMAL))
    {
        sysRetErrChkRet(pthread_mutex_unlock (&pMutex->shared_lock.mutex));
    }

    /* Nobody wants to know whenever ANY mutex is locked/unlocked; now if this was a particular mutex...
       CL_DEBUG_PRINT (CL_DEBUG_TRACE, ("\nMutex Unlock : DONE")); */
    CL_FUNC_EXIT();
    return (rc);
}

static ClRcT 
cosMutexDestroy (ClOsalMutexT *pMutex)
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

    if ( (pMutex->flags & CL_OSAL_SHARED_SYSV_SEM) )
    {
        rc = cosSysvMutexDestroy(pMutex);
    }
    else if ( (pMutex->flags & CL_OSAL_SHARED_POSIX_SEM) )
    {
        rc = cosPosixMutexDestroy(pMutex);
    }
    else if( (pMutex->flags & CL_OSAL_SHARED_NORMAL))
    {
        sysRetErrChkRet(pthread_mutex_destroy(&pMutex->shared_lock.mutex));
    }

    CL_FUNC_EXIT();
    return (rc);
}

/*******************************************************************/

static ClRcT 
cosMutexDelete (ClOsalMutexIdT mutexId)
{
    ClUint32T retCode = CL_OK;
    ClOsalMutexT* pMutex = (ClOsalMutexT*) mutexId;
   
    CL_FUNC_ENTER();
    if (NULL == pMutex)
	{
        CL_DEBUG_PRINT (CL_DEBUG_ERROR, ("\nMutex Delete : FAILED, mutex is NULL (double delete?)"));
        retCode = CL_OSAL_RC(CL_ERR_INVALID_PARAMETER);
        CL_FUNC_EXIT();
        return(retCode);
	}

    retCode = (ClUint32T)cosMutexDestroy(pMutex);

    if(0 == retCode) /* If the mutex destroy worked, we want to free the memory, but if it didn't work then the mutex still exists, so don't free */
        clHeapFree(pMutex);

    CL_FUNC_EXIT();
    return (retCode);
}



#ifdef CL_OSAL_DEBUG

static ClRcT 
cosMutexLockDebug (ClOsalMutexIdT mutexId, const ClCharT *file, ClInt32T line)
{
    ClRcT rc = CL_OK;
    ClOsalMutexT *pMutex = (ClOsalMutexT*)mutexId;

    nullChkRet(pMutex);
    CL_FUNC_ENTER();   
    if ( (pMutex->flags & CL_OSAL_SHARED_SYSV_SEM) )
    {
        rc = cosSysvMutexLock(mutexId);
    }
    else if ( (pMutex->flags & CL_OSAL_SHARED_POSIX_SEM) )
    {
        rc = cosPosixMutexLock(mutexId);
    }
    else if( (pMutex->flags & CL_OSAL_SHARED_NORMAL) )
    {
        if( !(pMutex->flags & (CL_OSAL_SHARED_RECURSIVE | CL_OSAL_SHARED_PROCESS | CL_OSAL_SHARED_ERROR_CHECK)))
        {
            rc = cosPosixMutexPoolLock(pMutex, file, line);
            if(rc != CL_OK)
            {
                CL_FUNC_EXIT();
                return rc;
            }
        }
        else
        {
            int err;
            if((err = pthread_mutex_lock (&pMutex->shared_lock.mutex)))
            {
                if(err == EDEADLK)
                    return CL_OSAL_RC(CL_ERR_INUSE);
                return CL_OSAL_RC(CL_ERR_LIBRARY);
            }
        }
    }

    CL_FUNC_EXIT();
    return (CL_OK);
}

static ClRcT 
cosMutexUnlockDebug (ClOsalMutexIdT mutexId, const ClCharT *file, ClInt32T line)
{
    ClRcT rc = CL_OK;
    ClUint32T retCode = 0;
    ClOsalMutexT* pMutex = (ClOsalMutexT*) mutexId;
   
    CL_FUNC_ENTER();
    
    if (NULL == pMutex)
	{
        printf("Mutex Unlock : FAILED, mutex is NULL (used after delete?)");
        clDbgPause();
        retCode = CL_OSAL_RC(CL_ERR_NULL_POINTER);
        CL_FUNC_EXIT();
        return(retCode);
	}

    if ( (pMutex->flags & CL_OSAL_SHARED_SYSV_SEM) )
    {
        rc = cosSysvMutexUnlock(mutexId);
    }
    else if ( (pMutex->flags & CL_OSAL_SHARED_POSIX_SEM) )
    {
        rc = cosPosixMutexUnlock(mutexId);
    }
    else if( (pMutex->flags & CL_OSAL_SHARED_NORMAL))
    {
        if(! (pMutex->flags & (CL_OSAL_SHARED_RECURSIVE | CL_OSAL_SHARED_PROCESS | CL_OSAL_SHARED_ERROR_CHECK)))
            cosPosixMutexPoolUnlock(pMutex, file, line);
        sysRetErrChkRet(pthread_mutex_unlock (&pMutex->shared_lock.mutex));
    }

    /* Nobody wants to know whenever ANY mutex is locked/unlocked; now if this was a particular mutex...
       CL_DEBUG_PRINT (CL_DEBUG_TRACE, ("\nMutex Unlock : DONE")); */
    CL_FUNC_EXIT();
    return (rc);
}

static ClRcT 
cosMutexDestroyDebug (ClOsalMutexT *pMutex, const ClCharT *file, ClInt32T line)
{
    ClRcT rc = CL_OK;
    ClUint32T retCode = 0;
    CL_FUNC_ENTER();

    if (NULL == pMutex)
	{
        printf("Mutex Destroy failed, mutex is NULL (double delete?)\n");
        retCode = CL_OSAL_RC(CL_ERR_NULL_POINTER);
        CL_FUNC_EXIT();
        return(retCode);
	}

    if ( (pMutex->flags & CL_OSAL_SHARED_SYSV_SEM) )
    {
        rc = cosSysvMutexDestroy(pMutex);
    }
    else if ( (pMutex->flags & CL_OSAL_SHARED_POSIX_SEM) )
    {
        rc = cosPosixMutexDestroy(pMutex);
    }
    else if( (pMutex->flags & CL_OSAL_SHARED_NORMAL))
    {
        if(!(pMutex->flags & (CL_OSAL_SHARED_RECURSIVE | CL_OSAL_SHARED_PROCESS | CL_OSAL_SHARED_ERROR_CHECK)))
            cosPosixMutexPoolDelete(pMutex, file, line);
        sysRetErrChkRet(pthread_mutex_destroy(&pMutex->shared_lock.mutex));
    }

    CL_FUNC_EXIT();
    return (rc);
}

/*******************************************************************/

static ClRcT 
cosMutexDeleteDebug (ClOsalMutexIdT mutexId, const ClCharT *file, ClInt32T line)
{
    ClUint32T retCode = CL_OK;
    ClOsalMutexT* pMutex = (ClOsalMutexT*) mutexId;
   
    CL_FUNC_ENTER();
    if (NULL == pMutex)
	{
        printf("\nMutex Delete : FAILED, mutex is NULL (double delete?)");
        retCode = CL_OSAL_RC(CL_ERR_INVALID_PARAMETER);
        CL_FUNC_EXIT();
        return(retCode);
	}

    retCode = (ClUint32T)cosMutexDestroyDebug(pMutex, file, line);

    if(0 == retCode) /* If the mutex destroy worked, we want to free the memory, but if it didn't work then the mutex still exists, so don't free */
        free(pMutex);

    CL_FUNC_EXIT();
    return (retCode);
}

#endif


#if 0
void clOsalGetStackTrace(char *sigName, void *param)
{
    char **stackTraceInfo;

    stackTraceInfo = backtrace_symbols(trace, trace_size);
    trace_size = strlen(*stackTraceInfo);
    if(trace_size > (LOG_BUFFER_SIZE - strlen(logBuffer) - 3) )
    {
        strncpy(logBuffer, *stackTraceInfo, LOG_BUFFER_SIZE - strlen(logBuffer) - 3);
    }
    else
    {
        strcpy(logBuffer, *stackTraceInfo);
    }
    strcat(logBuffer,"\n\n");
    clLogPrint();
    /*todo : make use of the trace information when available*/
    return;
}
#endif

#if 0
static ClBoolT osalShmExistsForComp(const ClCharT *compName)
{
    ClRcT rc = CL_OK;
    
    return CL_OK == (rc = clOsalShmIdGet((ClUint8T *)compName,
                                         &gClCompUniqueShmId));
}
#endif

#ifdef __i386__
static const char *registerMap[NGREG] = { "gs", "fs", "es", "ds", "edi", "esi", "ebp", "esp",
                                          "ebx", "edx", "ecx", "eax", "trap", "err", "eip", "cs",
                                          "eflags", "uesp", "ss" };

#elif __x86_64__
static const char *registerMap[NGREG] = { "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
                                          "rdi", "rsi", "rbp", "rbx", "rdx", "rax", "rcx", "rsp",
                                          "rip", "eflags", "csgsfs", "err", "trapno", "oldmask", "cr2",
};
                      
#elif __mips__
static const char *registerMap[NGREG] = { "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
                                          "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
                                          "r16", "r17", "r18", "r19", "r20", "r21", "r22", "r23",
                                          "r24", "r25", "r26", "r27", "r28", "r29", "r30", "r31"
};

#else
static const char *registerMap[NGREG] = { NULL, };

#endif

static void registerDump(ucontext_t *ucontext, ClCharT *exceptionSegment, ClUint32T maxBytes)
{
    register int i;
    ClUint32T bytes = 0;
    bytes += snprintf(exceptionSegment + bytes, maxBytes - bytes, "\n");
    for(i = 0; i < NGREG && registerMap[i]; ++i)
    {
        if( i && !(i & 7))
            bytes += snprintf(exceptionSegment + bytes, maxBytes - bytes, "\n");
        bytes += snprintf(exceptionSegment + bytes, maxBytes-bytes, "[%s] = [%#llx]%s ", 
                          registerMap[i], (ClUint64T)ucontext->uc_mcontext.gregs[i], i + 1 < NGREG ? "," : "");
    }
    bytes += snprintf(exceptionSegment + bytes, maxBytes - bytes, "\n");
}

static void clOsalSigHandler(int signum, siginfo_t *info, void *param)
{
    char sigName[16];
    int logfd = -1;
    ClFdT fd = 0;
    void *buffer[STACK_DEPTH] = {NULL};
    void *trace[STACK_DEPTH];
    int trace_size = 0;
    ClRcT rc = CL_OK;
    ClUint32T segmentSize = getpagesize();
    ClCharT *exceptionSegment = NULL;
    ucontext_t *uc = (ucontext_t *)param;
    ClInt32T bytes = 0;
    switch(signum)
    {
    case SIGHUP:
        strcpy(sigName, "SIGHUP");
        sigaction(SIGHUP, &oldact[SIGHUP], NULL);
        break;
    case SIGINT:
        strcpy(sigName, "SIGINT");
        sigaction(SIGINT, &oldact[SIGINT], NULL);
        break;
    case SIGQUIT:
        strcpy(sigName, "SIGQUIT");
        sigaction(SIGQUIT, &oldact[SIGQUIT], NULL);
        break;
    case SIGILL:
        strcpy(sigName, "SIGILL");
        sigaction(SIGILL, &oldact[SIGILL], NULL);
        break;
    case SIGABRT:
        strcpy(sigName, "SIGABRT");
        sigaction(SIGABRT, &oldact[SIGABRT], NULL);
        break;
    case SIGFPE:
        strcpy(sigName, "SIGFPE");
        sigaction(SIGFPE, &oldact[SIGFPE], NULL);
        break;
    case SIGSEGV:
        strcpy(sigName, "SIGSEGV");
        sigaction(SIGSEGV, &oldact[SIGSEGV], NULL);
        break;
    case SIGPIPE:
        strcpy(sigName, "SIGPIPE");
        sigaction(SIGPIPE, &oldact[SIGPIPE], NULL);
        break;
    case SIGALRM:
        strcpy(sigName, "SIGALRM");
        sigaction(SIGALRM, &oldact[SIGALRM], NULL);
        break;
    case SIGTERM:
        strcpy(sigName, "SIGTERM");
        sigaction(SIGTERM, &oldact[SIGTERM], NULL);
        break;
    default:
        break;
    }
    snprintf(logBuffer,
             LOG_BUFFER_SIZE-1,
             "\nStack frame trace for component %s, process %d, signal: %s ::\n"
             "------------------------------------------\n",
             getenv("ASP_COMPNAME") ? getenv("ASP_COMPNAME") : "unknown",
             (int)getpid(),
             sigName);
    (void)uc;
    trace_size = get_backtrace(buffer, trace, 16, uc);
    
    if(clDbgLogLevel > 0)
        logfd = open("/var/log/aspdbg.log", O_APPEND | O_RDWR | O_CREAT, 0666);

    {
        ClCharT *compName = getenv("ASP_COMPNAME");
        ClCharT shmName[CL_MAX_NAME_LENGTH];
        CL_ASSERT(compName != NULL);
        snprintf(shmName, sizeof(shmName), "/CL_%s_exception_%d", compName, clIocLocalAddressGet());
        clOsalShmUnlink(shmName);
        rc = clOsalShmOpen(shmName, O_RDWR | O_CREAT | O_TRUNC, 0777, &fd);
        if(rc != CL_OK)
        {
            if(logfd >= 0)
            {
                snprintf(logBuffer, sizeof(logBuffer), "Opening shared segment [%s] returned [%#x]\n", 
                         shmName, rc);
                bytes = write(logfd, logBuffer, strlen(logBuffer));
            }
            goto out;
        }
        rc = clOsalFtruncate(fd, segmentSize);
        if(rc != CL_OK)
        {
            if(logfd >= 0)
            {
                snprintf(logBuffer, sizeof(logBuffer), "Ftruncate on shared segment [%s] returned [%#x]\n",
                         shmName, rc);
                bytes = write(logfd, logBuffer, strlen(logBuffer));
            }
            goto out;
        }
        rc = clOsalMmap(0, segmentSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0, (ClPtrT*)&exceptionSegment);
        if(rc != CL_OK)
        {
            if(logfd >= 0)
            {
                snprintf(logBuffer, sizeof(logBuffer), "Mmap on shared segment [%s] returned [%#x]\n",
                         shmName, rc);
                bytes = write(logfd, logBuffer, strlen(logBuffer));
            }
            goto out;
        }
    }

    {
        int         count = 0;
        char        **message = (char **)NULL;
        bytes = 0;
        message = backtrace_symbols(trace, trace_size);
        if(exceptionSegment != NULL)
        {
            memset(exceptionSegment, 0, segmentSize);
            bytes += snprintf(exceptionSegment + bytes, segmentSize-bytes, "%s", logBuffer);
            for(count = 0; count < trace_size; count++)
            {
                bytes += snprintf(exceptionSegment + bytes, segmentSize - bytes,
                                  "%s%s", message[count], buffer[count] ? "\t" : "\n");
                if(buffer[count])
                {
                    bytes += snprintf(exceptionSegment + bytes, segmentSize - bytes, 
                                      "(pc AT %#lx : %ld)\n",
                                      (ClWordT)buffer[count], 
                                      trace[count] ? ((ClWordT)buffer[count]-(ClWordT)trace[count]) : 0L);
                }
            }
            registerDump(uc, exceptionSegment + bytes, segmentSize - bytes);
            if(logfd > 0)
                bytes = write(logfd, exceptionSegment, strlen(exceptionSegment));
            clOsalMsync(exceptionSegment, segmentSize, MS_SYNC);
            clOsalMunmap(exceptionSegment, segmentSize);
        }
    }

    out:
    if(logfd >= 0) close(logfd);
    if(fd >= 0) close(fd);
    raise(signum);
}

void clOsalSigHandlerInitialize()
{
    struct sigaction act;
    
    act.sa_sigaction = &clOsalSigHandler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = SA_RESTART | SA_SIGINFO;
    
    if( sigaction(SIGHUP, &act, &oldact[SIGHUP])  ||
        sigaction(SIGINT, &act, &oldact[SIGINT])  ||
        sigaction(SIGQUIT, &act, &oldact[SIGQUIT]) ||
        sigaction(SIGILL, &act, &oldact[SIGILL])  
#ifndef CL_OSAL_DEBUG
        ||
        sigaction(SIGABRT, &act, &oldact[SIGABRT])
#endif
        ||
        sigaction(SIGFPE, &act, &oldact[SIGFPE])  
#ifndef CL_OSAL_DEBUG
        ||
        sigaction(SIGSEGV, &act, &oldact[SIGSEGV]) 
#endif 
        ||
        sigaction(SIGPIPE, &act, &oldact[SIGPIPE]) ||
        sigaction(SIGALRM, &act, &oldact[SIGALRM]) ||
        sigaction(SIGTERM, &act, &oldact[SIGTERM]))
    {
        CL_DEBUG_PRINT (CL_DEBUG_ERROR,("\nSIGACTION FUNCTION FAILED. Signal handling will not be available."));
    }
}

/**************************************************************************/

static ClRcT
cosPosixCleanup (osalFunction_t *pOsalFunction)
{
    ClRcT errorCode = CL_OK;

    CL_FUNC_ENTER();

    if(0 == gTaskControl.isCosInitialized)
    {
        return (CL_OK);
    }

    if(NULL == pOsalFunction)
    {
        errorCode = CL_OSAL_RC(CL_OSAL_ERR_COS_CLEANUP);
        CL_FUNC_EXIT();
        return(errorCode);
    }

    clOsalTaskKeyDelete(gTaskControl.taskNameKey);

    gTaskControl.isCosInitialized = 0;
    pOsalFunction->fpFunctionTaskCreateAttached = NULL;
    pOsalFunction->fpFunctionTaskCreateDetached = NULL;
    pOsalFunction->fpFunctionTaskDelete = NULL;
    pOsalFunction->fpFunctionTaskKill = NULL;
    pOsalFunction->fpFunctionSelfTaskIdGet = NULL;
    pOsalFunction->fpFunctionTaskPriorityGet = NULL;
    pOsalFunction->fpFunctionTaskPrioritySet = NULL;
    pOsalFunction->fpFunctionTaskNameGet = NULL;
    pOsalFunction->fpFunctionTaskDelay = NULL;
    pOsalFunction->fpFunctionTimeOfDayGet = NULL;
    pOsalFunction->fpNanoTimeGet = NULL;

    pOsalFunction->fpMutexAttrInit = NULL;
    pOsalFunction->fpMutexAttrDestroy = NULL;
    pOsalFunction->fpFunctionMutexInit = NULL;
    pOsalFunction->fpFunctionMutexCreate = NULL;
    pOsalFunction->fpFunctionSharedMutexCreate = NULL;
    pOsalFunction->fpFunctionMutexLock = NULL;
    pOsalFunction->fpFunctionMutexUnlock = NULL;
    pOsalFunction->fpFunctionMutexDelete = NULL;
    pOsalFunction->fpFunctionMutexDestroy = NULL;

    pOsalFunction->fpCondAttrInit = NULL;
    pOsalFunction->fpCondAttrDestroy = NULL;
    pOsalFunction->fpFunctionCondInit = NULL;
    pOsalFunction->fpFunctionCondCreate = NULL;
    pOsalFunction->fpFunctionCondDelete = NULL;
    pOsalFunction->fpFunctionCondDestroy = NULL;
    pOsalFunction->fpFunctionCondWait = NULL;
    pOsalFunction->fpFunctionCondBroadcast = NULL;
    pOsalFunction->fpFunctionCondSignal = NULL;

    pOsalFunction->fpFunctionTaskDataSet = NULL;
    pOsalFunction->fpFunctionTaskDataGet = NULL;
    pOsalFunction->fpFunctionTaskKeyCreate = NULL;
    pOsalFunction->fpFunctionTaskKeyDelete = NULL;
    pOsalFunction->fpFunctionSemCreate = NULL;
    pOsalFunction->fpFunctionSemLock = NULL;
    pOsalFunction->fpFunctionSemTryLock = NULL;
    pOsalFunction->fpFunctionSemUnlock = NULL;
    pOsalFunction->fpFunctionSemValueGet = NULL;
    pOsalFunction->fpFunctionSemDelete = NULL;
    pOsalFunction->fpFunctionSemIdGet = NULL;
    pOsalFunction->fpFunctionProcessCreate = NULL;
    pOsalFunction->fpFunctionProcessDelete = NULL;
    pOsalFunction->fpFunctionProcessWait = NULL;
    pOsalFunction->fpFunctionProcessSelfIdGet = NULL;
    pOsalFunction->fpFunctionShmCreate = NULL;
    pOsalFunction->fpFunctionShmIdGet = NULL;
    pOsalFunction->fpFunctionShmDelete = NULL;
    pOsalFunction->fpFunctionShmAttach = NULL;
    pOsalFunction->fpFunctionShmDetach = NULL;
    pOsalFunction->fpFunctionShmSecurityModeSet = NULL;
    pOsalFunction->fpFunctionShmSecurityModeGet = NULL;
    pOsalFunction->fpFunctionShmSizeGet = NULL;
    pOsalFunction->fpFunctionMutexValueSet = NULL;
    pOsalFunction->fpFunctionMutexValueGet = NULL;
    pOsalFunction->fpFunctionMutexTryLock = NULL;
    pOsalFunction->fpFunctionMutexErrorCheckInit = NULL;
    pOsalFunction->fpFunctionMutexErrorCheckCreate = NULL;

    pOsalFunction->fpMaxPathGet = NULL;
    pOsalFunction->fpPageSizeGet = NULL;

    CL_DEBUG_PRINT (CL_DEBUG_TRACE,("\nCOS Clean up: DONE"));
    CL_FUNC_EXIT();
    return (CL_OK);
}

/**************************************************************************/

#if 0
static ClRcT 
cosConfInit(void* pConfig)
{
    ClRcT retCode = CL_OK;
    cosCompCfgInit_t *pConfigData = NULL;
    pConfigData = (cosCompCfgInit_t *)pConfig;

    if(NULL == pConfigData)
    {
        retCode = CL_OSAL_RC(CL_ERR_NULL_POINTER);
        CL_FUNC_EXIT();
        return(retCode);
    }

    sCosConfig.cosTaskMinStackSize = pConfigData->cosTaskMinStackSize;

    retCode = cosPosixInit ();

	if(CL_OK != retCode)
	{
		return retCode;
	}
    
	return (CL_OK);
}
#endif

static ClRcT
cosPosixInit (void)
{
    ClUint32T retCode = 0;

    if(gTaskControl.isCosInitialized)
    {
        return (CL_OK);
    }    
    
    gOsalFunction.fpFunctionFileOpen = cosPosixFileOpen;
    gOsalFunction.fpFunctionFileClose = cosPosixFileClose;
    gOsalFunction.fpFunctionMmap = cosPosixMmap;
    gOsalFunction.fpFunctionMunmap = cosPosixMunmap;
    gOsalFunction.fpFunctionMsync = cosPosixMsync;
    gOsalFunction.fpFunctionShmOpen = cosPosixShmOpen;
    gOsalFunction.fpShmUnlink = cosPosixShmUnlink;
    gOsalFunction.fpFunctionFtruncate = cosPosixFtruncate;
    gOsalFunction.fpFunctionTaskCreateDetached = cosPosixTaskCreateDetached;
    gOsalFunction.fpFunctionTaskCreateAttached = cosPosixTaskCreateAttached;
    gOsalFunction.fpFunctionTaskDelete      = cosPosixTaskDelete;
    gOsalFunction.fpFunctionTaskKill        = cosPosixTaskKill;
    gOsalFunction.fpFunctionTaskJoin        = cosPosixTaskJoin;
    gOsalFunction.fpFunctionTaskDetach      = cosPosixTaskDetach;
    gOsalFunction.fpFunctionSelfTaskIdGet   = cosPosixSelfTaskIdGet;
    gOsalFunction.fpFunctionTaskNameGet     = cosPosixTaskNameGet;
    gOsalFunction.fpFunctionTaskPriorityGet = cosPosixTaskPriorityGet;
    gOsalFunction.fpFunctionTaskPrioritySet = cosPosixTaskPrioritySet;
    gOsalFunction.fpFunctionTaskDelay = cosPosixTaskDelay;
    gOsalFunction.fpFunctionTimeOfDayGet = cosPosixTimeOfDayGet;
    gOsalFunction.fpFunctionStopWatchTimeGet = cosPosixStopWatchTimeGet;
    gOsalFunction.fpNanoTimeGet = cosPosixNanoTimeGet;
    gOsalFunction.fpMutexAttrInit = cosPosixMutexAttrInit;
    gOsalFunction.fpMutexAttrDestroy = cosPosixMutexAttrDestroy;
    gOsalFunction.fpMutexAttrPSharedSet = cosPosixMutexAttrPSharedSet;
    gOsalFunction.fpFunctionMutexCreate = cosPosixMutexCreate;
    gOsalFunction.fpFunctionSharedMutexCreate = cosSharedMutexCreate;

    gOsalFunction.fpFunctionMutexInit = cosPosixMutexInit;
    gOsalFunction.fpFunctionRecursiveMutexInit = cosPosixRecursiveMutexInit;
    gOsalFunction.fpFunctionProcessSharedMutexInit = cosProcessSharedMutexInit;
    /*    gOsalFunction.fpFunctionMutexInitEx = cosPosixMutexInitEx; */
    gOsalFunction.fpFunctionMutexLock = cosMutexLock;
    gOsalFunction.fpFunctionMutexUnlock = cosMutexUnlock;
    gOsalFunction.fpFunctionMutexDelete = cosMutexDelete;
    gOsalFunction.fpFunctionMutexDestroy = cosMutexDestroy;

#ifdef CL_OSAL_DEBUG
    gOsalFunction.fpFunctionMutexInitDebug = cosPosixMutexInitDebug;
    gOsalFunction.fpFunctionRecursiveMutexInitDebug = cosPosixRecursiveMutexInitDebug;
    gOsalFunction.fpFunctionMutexCreateDebug = cosPosixMutexCreateDebug;
    gOsalFunction.fpFunctionMutexLockDebug = cosMutexLockDebug;
    gOsalFunction.fpFunctionMutexUnlockDebug = cosMutexUnlockDebug;
    gOsalFunction.fpFunctionMutexDeleteDebug = cosMutexDeleteDebug;
    gOsalFunction.fpFunctionMutexDestroyDebug = cosMutexDestroyDebug;
#endif
    
    gOsalFunction.fpCondAttrInit = cosPosixCondAttrInit;
    gOsalFunction.fpCondAttrDestroy = cosPosixCondAttrDestroy;
    gOsalFunction.fpCondAttrPSharedSet = cosPosixCondAttrPSharedSet;
    gOsalFunction.fpFunctionCondInit = cosPosixCondInit;
    gOsalFunction.fpFunctionProcessSharedCondInit = cosPosixProcessSharedCondInit;
    gOsalFunction.fpFunctionCondInitEx = cosPosixCondInitEx;
    gOsalFunction.fpFunctionCondCreate = cosPosixCondCreate;
    gOsalFunction.fpFunctionCondDelete = cosPosixCondDelete;
    gOsalFunction.fpFunctionCondDestroy = cosPosixCondDestroy;
    gOsalFunction.fpFunctionCondWait = cosPosixCondWait;
#ifdef CL_OSAL_DEBUG
    gOsalFunction.fpFunctionCondWaitDebug = cosPosixCondWaitDebug;
#endif    
    gOsalFunction.fpFunctionCondBroadcast = cosPosixCondBroadcast;
    gOsalFunction.fpFunctionCondSignal = cosPosixCondSignal;
    gOsalFunction.fpFunctionTaskDataSet = cosPosixTaskDataSet;
    gOsalFunction.fpFunctionTaskDataGet = cosPosixTaskDataGet;
    gOsalFunction.fpFunctionTaskKeyCreate = cosPosixTaskKeyCreate;
    gOsalFunction.fpFunctionTaskKeyDelete = cosPosixTaskKeyDelete;
    gOsalFunction.fpFunctionCleanup = cosPosixCleanup;
    gOsalFunction.fpFunctionSemCreate = cosSysvSemCreate;
    gOsalFunction.fpFunctionSemIdGet = cosSysvSemIdGet;
    gOsalFunction.fpFunctionSemLock = cosSysvSemLock;
    gOsalFunction.fpFunctionSemTryLock = cosSysvSemTryLock;
    gOsalFunction.fpFunctionSemUnlock = cosSysvSemUnlock;
    gOsalFunction.fpFunctionSemValueGet = cosSysvSemValueGet;
    gOsalFunction.fpFunctionSemDelete = cosSysvSemDelete;
    gOsalFunction.fpFunctionProcessCreate = cosProcessCreate;
    gOsalFunction.fpFunctionProcessDelete = cosPosixProcessDelete;
    gOsalFunction.fpFunctionProcessWait = cosPosixProcessWait;
    gOsalFunction.fpFunctionProcessSelfIdGet = cosPosixProcessSelfIdGet;
    gOsalFunction.fpFunctionShmCreate = cosSysvShmCreate;
    gOsalFunction.fpFunctionShmIdGet = cosSysvShmIdGet;
    gOsalFunction.fpFunctionShmDelete = cosSysvShmDelete;
    gOsalFunction.fpFunctionShmAttach = cosSysvShmAttach;
    gOsalFunction.fpFunctionShmDetach = cosSysvShmDetach;
    gOsalFunction.fpFunctionShmSecurityModeSet = cosSysvShmSecurityModeSet;
    gOsalFunction.fpFunctionShmSecurityModeGet = cosSysvShmSecurityModeGet;
    gOsalFunction.fpFunctionShmSizeGet = cosSysvShmSizeGet;
    gOsalFunction.fpMaxPathGet = cosPosixMaxPathGet;
    gOsalFunction.fpPageSizeGet = cosPosixPageSizeGet;
    gOsalFunction.fpFunctionMutexValueSet = cosMutexValueSet;
    gOsalFunction.fpFunctionMutexValueGet = cosMutexValueGet;
    gOsalFunction.fpFunctionMutexTryLock = cosMutexTryLock;
    gOsalFunction.fpFunctionMutexErrorCheckInit = cosPosixMutexErrorCheckInit;
    gOsalFunction.fpFunctionMutexErrorCheckCreate = cosPosixMutexErrorCheckCreate;

    /*
     * Create the key to hold the thread name in the thread specific
     * area. clOsalTaskKeyCreate() can be used as it has been just
     * initialized (above).
     */
    retCode = clOsalTaskKeyCreate(&gTaskControl.taskNameKey, NULL);
    if (CL_OK != retCode)
    {
        return retCode;
    }

    gTaskControl.isCosInitialized = 1;
    /* FIXME: Why this bootLog("cos Init Done");*/
    return (CL_OK);
}
