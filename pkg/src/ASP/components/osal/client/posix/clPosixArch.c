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
 * Build: 4.1.0
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
#include <clQnx.h>
#include "clOsalCommon.h"
#include "clCommonCos.h"
#include "clPosix.h"

#ifdef CL_OSAL_DEBUG
#include "clPosixDebug.h"
#endif

#ifdef _POSIX_PRIORITY_SCHEDULING
#include <sched.h>
#endif
#include <clEoApi.h>

char logBuffer[LOG_BUFFER_SIZE];
struct sigaction oldact;
osalFunction_t gOsalFunction = {0};
CosTaskControl_t gTaskControl;
cosCompCfgInit_t sCosConfig={CL_OSAL_MIN_STACK_SIZE};


/**************************************************************************/
static ClRcT cosPosixInit (void);

/**************************************************************************/

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
cosProcessSharedMutexInit (ClOsalMutexT* pMutex, ClOsalSharedMutexFlagsT flags, ClUint8T *pKey, ClUint32T keyLen, ClInt32T value)
{
  ClRcT rc = CL_OK;
  
  nullChkRet(pMutex);
  CL_FUNC_ENTER();  

  pMutex->flags = flags;

  if( (flags & CL_OSAL_SHARED_NORMAL) )
  {
      rc = cosPosixProcessSharedFutexInit(pMutex);
  }
  else
  {
      rc = cosPosixProcessSharedSemInit(pMutex, pKey, keyLen, value);
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
    ClRcT rc = CL_OK;
    ClOsalMutexT *pMutex = (ClOsalMutexT *) mutexId;
    
    nullChkRet(pMutex);
    CL_FUNC_ENTER();

    if( (pMutex->flags & CL_OSAL_SHARED_POSIX_SEM) )
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

    if ( (pMutex->flags & CL_OSAL_SHARED_NORMAL) )
    {
        rc = CL_OSAL_RC(CL_ERR_NOT_SUPPORTED);
        clDbgCodeError(rc, ("Normal mutex dont support getval operations\n"));
        goto out;
    }
    else
    {
        rc = cosPosixMutexValueGet(mutexId, pValue);
    }
    CL_FUNC_EXIT();

    out:
    return rc;
}

/**************************************************************************/

static ClRcT 
__cosMutexLock (ClOsalMutexIdT mutexId, ClBoolT verbose)
{
    ClRcT rc = CL_OK;
    ClOsalMutexT *pMutex = (ClOsalMutexT*)mutexId;

    if(verbose)
    {
        nullChkRet(pMutex);
    }
    else if(!pMutex)
    {
        return CL_OSAL_RC(CL_OSAL_ERR_OS_ERROR);
    }

    CL_FUNC_ENTER();   
    if( (pMutex->flags & CL_OSAL_SHARED_NORMAL) )
    {
        int err;
        if((err = pthread_mutex_lock (&pMutex->shared_lock.mutex)))
        {
            if(err == EDEADLK)
                return CL_OSAL_RC(CL_ERR_INUSE);
            return CL_OSAL_RC(CL_ERR_LIBRARY);
        }
    }
    else
    {
        rc = __cosPosixMutexLock(mutexId, verbose);
    }

    CL_FUNC_EXIT();
    return (CL_OK);
}

static ClRcT 
cosMutexLock (ClOsalMutexIdT mutexId)
{
    return __cosMutexLock(mutexId, CL_TRUE);
}

static ClRcT 
cosMutexLockSilent (ClOsalMutexIdT mutexId)
{
    return __cosMutexLock(mutexId, CL_FALSE);
}

static ClRcT 
cosMutexTryLock (ClOsalMutexIdT mutexId)
{
    ClRcT rc = CL_OK;
    ClOsalMutexT *pMutex = (ClOsalMutexT*)mutexId;
    if(!pMutex) 
        return CL_OSAL_RC(CL_ERR_NULL_POINTER);
    CL_FUNC_ENTER();   
    if( (pMutex->flags & CL_OSAL_SHARED_NORMAL) )
    {
        int err;
        if((err = pthread_mutex_trylock(&pMutex->shared_lock.mutex) == EDEADLK))
            return CL_OSAL_RC(CL_ERR_INUSE);
        else if(err == EAGAIN || err == EBUSY)
            return CL_OSAL_RC(CL_ERR_TRY_AGAIN);
        else if(err)
            return CL_OSAL_RC(CL_ERR_LIBRARY);
    }
    else
    {
        rc = cosPosixMutexTryLock(mutexId);
    }

    CL_FUNC_EXIT();
    return rc;
}


/**************************************************************************/

static ClRcT 
__cosMutexUnlock (ClOsalMutexIdT mutexId, ClBoolT verbose)
{
    ClRcT rc = CL_OK;
    ClUint32T retCode = 0;
    ClOsalMutexT* pMutex = (ClOsalMutexT*) mutexId;
   
    CL_FUNC_ENTER();
    
    if (NULL == pMutex)
	{
        retCode = CL_OSAL_RC(CL_ERR_NULL_POINTER);
        if(verbose)
        {
            CL_DEBUG_PRINT (CL_DEBUG_ERROR, ("Mutex Unlock : FAILED, mutex is NULL (used after delete?)"));
            clDbgPause();
        }
        CL_FUNC_EXIT();
        return(retCode);
	}

    if( (pMutex->flags & CL_OSAL_SHARED_NORMAL))
    {
        if(verbose)
        {
            sysRetErrChkRet(pthread_mutex_unlock (&pMutex->shared_lock.mutex));
        }
        else
        {
            if(pthread_mutex_unlock(&pMutex->shared_lock.mutex))
            {
                rc = CL_OSAL_RC(CL_ERR_LIBRARY);
            }
        }
    }
    else
    {
        rc = __cosPosixMutexUnlock(mutexId, verbose);
    }

    CL_FUNC_EXIT();
    return (rc);
}

static ClRcT 
cosMutexUnlock (ClOsalMutexIdT mutexId)
{
    return __cosMutexUnlock(mutexId, CL_TRUE);
}

static ClRcT 
cosMutexUnlockSilent(ClOsalMutexIdT mutexId)
{
    return __cosMutexUnlock(mutexId, CL_FALSE);
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

    if( (pMutex->flags & CL_OSAL_SHARED_NORMAL))
    {
        sysRetErrChkRet(pthread_mutex_destroy(&pMutex->shared_lock.mutex));
    }
    else
    {
        rc = cosPosixMutexDestroy(pMutex);
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
    
    if( (pMutex->flags & CL_OSAL_SHARED_NORMAL) )
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
    else 
    {
        rc = cosPosixMutexLock(mutexId);
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

    if( (pMutex->flags & CL_OSAL_SHARED_NORMAL))
    {
        if(! (pMutex->flags & (CL_OSAL_SHARED_RECURSIVE | CL_OSAL_SHARED_PROCESS | CL_OSAL_SHARED_ERROR_CHECK)))
            cosPosixMutexPoolUnlock(pMutex, file, line);
        sysRetErrChkRet(pthread_mutex_unlock (&pMutex->shared_lock.mutex));
    }
    else
    {
        rc = cosPosixMutexUnlock(mutexId);
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
        printf("Mutex Destroy failed, mutex is NULL (double delete?)");
        retCode = CL_OSAL_RC(CL_ERR_NULL_POINTER);
        CL_FUNC_EXIT();
        return(retCode);
	}

    if( (pMutex->flags & CL_OSAL_SHARED_NORMAL))
    {
        if(!(pMutex->flags & (CL_OSAL_SHARED_RECURSIVE | CL_OSAL_SHARED_PROCESS | CL_OSAL_SHARED_ERROR_CHECK)))
            cosPosixMutexPoolDelete(pMutex, file, line);
        sysRetErrChkRet(pthread_mutex_destroy(&pMutex->shared_lock.mutex));
    }
    else
    {
        rc = cosPosixMutexDestroy(pMutex);
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


/**************************************************************************/
static ClRcT
cosProcessCreate(ClOsalProcessFuncT fpFunction, 
                      void* processArg, 
                      ClOsalProcessFlagT creationFlags, 
                      ClOsalPidT* pProcessId)
{
    CL_FUNC_ENTER();
    nullChkRet(fpFunction);
    nullChkRet(processArg);

    fpFunction(processArg);
    
    CL_FUNC_EXIT();
    return (CL_OK);
}

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

static void clOsalSigHandler(int signum, siginfo_t *info, void *param)
{
    char sigName[16];
    
#if 0
    void *trace[STACK_DEPTH];
    int trace_size = 0;
    ClRcT rc = CL_OK;

#ifdef __i386__
    ucontext_t *uc = (ucontext_t *)param;
#endif
#endif

    
    switch(signum)
    {
        case SIGHUP:
            strcpy(sigName, "SIGHUP");
            sigaction(SIGHUP, &oldact, NULL);
            break;
        case SIGINT:
            strcpy(sigName, "SIGINT");
            sigaction(SIGINT, &oldact, NULL);
            break;
        case SIGQUIT:
            strcpy(sigName, "SIGQUIT");
            sigaction(SIGQUIT, &oldact, NULL);
            break;
        case SIGILL:
            strcpy(sigName, "SIGILL");
            sigaction(SIGILL, &oldact, NULL);
            break;
        case SIGABRT:
            strcpy(sigName, "SIGABRT");
            sigaction(SIGABRT, &oldact, NULL);
            break;
        case SIGFPE:
            strcpy(sigName, "SIGFPE");
            sigaction(SIGFPE, &oldact, NULL);
            break;
        case SIGSEGV:
            strcpy(sigName, "SIGSEGV");
            sigaction(SIGSEGV, &oldact, NULL);
            break;
        case SIGPIPE:
            strcpy(sigName, "SIGPIPE");
            sigaction(SIGPIPE, &oldact, NULL);
            break;
        case SIGALRM:
            strcpy(sigName, "SIGALRM");
            sigaction(SIGALRM, &oldact, NULL);
            break;
        case SIGTERM:
            strcpy(sigName, "SIGTERM");
            sigaction(SIGTERM, &oldact, NULL);
            break;
        default:
            break;
    }
#if 0
    sprintf(logBuffer,"\nStack frame trace for process %d, signal: %s ::\n"
                        "------------------------------------------\n", (int)getpid(), sigName);
    trace_size = backtrace(trace, 16);
#ifdef __i386__
    if(uc)
        trace[1] = (void *) uc->uc_mcontext.gregs[REG_EIP];
#endif

 {
   int fd;

    if( (fd = open("/var/log/aspdbg.log", O_APPEND | O_RDWR | O_CREAT, 00644)) != -1)
    {
    write(fd, (void *)logBuffer, strlen(logBuffer) );
    backtrace_symbols_fd(trace, trace_size, fd);
    close(fd);
    }
 }
    
 {
     ClCharT *compName = getenv("ASP_COMPNAME");
     
     CL_ASSERT(compName != NULL);
     
     if (!osalShmExistsForComp(compName))
     {
         clLog(CL_LOG_DEBUG, "OSAL", CL_LOG_CONTEXT_UNSPECIFIED,
               "Creating shared memory for [%s]...", compName);
         
         rc = clOsalShmCreate((ClUint8T *) compName,
                              sizeof(ClOsalShmAreaDefT),
                              &gClCompUniqueShmId);
         if (CL_OK != rc)
         {
             clLog(CL_LOG_CRITICAL, "OSAL", CL_LOG_CONTEXT_UNSPECIFIED,
                   "Could not create shared memory for component [%s], error [%#x]",
                   compName,
                   rc);
             clLogMultiline(CL_LOG_CRITICAL, "OSAL", CL_LOG_CONTEXT_UNSPECIFIED,
                            "- This typically indicates a component name mismatch.\n"
                            "Please compare component name in clEoConfig with "
                            "content of clAmfConfig.xml");
             return;
         }
     }
     else
     {
         clLog(CL_LOG_DEBUG, "OSAL", CL_LOG_CONTEXT_UNSPECIFIED,
               "The [%s] already has shared memory with id [%d]...",
               compName, gClCompUniqueShmId);
     }

     rc = clOsalShmAttach(gClCompUniqueShmId, 0, (void **) &gpClShmArea);
     if (CL_OK != rc)
     {
         clLog(CL_LOG_CRITICAL, CL_LOG_AREA_UNSPECIFIED, CL_LOG_CONTEXT_UNSPECIFIED,
               "Could not attach to shared memory, error [0x%x]", rc);
         return;
     }
 }

    {
        int         count = 0;
        char        **message = (char **)NULL;
        int         sizeLeft = CL_OSAL_SHM_EXCEPTION_LENGTH;
        
        message = backtrace_symbols(trace, trace_size);
        if(gpClShmArea != NULL)
        {
            memset(((ClOsalShmAreaDefT*)gpClShmArea)->exceptionInfo, 0, 
                    CL_OSAL_SHM_EXCEPTION_LENGTH);
            strcat(((ClOsalShmAreaDefT*)gpClShmArea)->exceptionInfo, logBuffer);
            sizeLeft -= strlen(logBuffer);
            for(count = 1; count < trace_size; count++)
            {
                /*Check that we have enough size left in shared memory region*/
                sizeLeft -= (strlen(message[count]) + 1);
                if(sizeLeft > 0)
                {
                    strcat(((ClOsalShmAreaDefT*)gpClShmArea)->exceptionInfo, message[count]);
                    strcat(((ClOsalShmAreaDefT*)gpClShmArea)->exceptionInfo, "\n");
                }
                else
                    break;
            }
        }
    }
#endif

    raise(signum);
}

void clOsalSigHandlerInitialize()
{
    struct sigaction act;
    
    act.sa_sigaction = &clOsalSigHandler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = SA_RESTART | SA_SIGINFO;
    
    if( sigaction(SIGHUP, &act, &oldact)  ||
        sigaction(SIGINT, &act, &oldact)  ||
        sigaction(SIGQUIT, &act, &oldact) ||
        sigaction(SIGILL, &act, &oldact)  ||
        sigaction(SIGABRT, &act, &oldact) ||
        sigaction(SIGFPE, &act, &oldact)  ||
        sigaction(SIGSEGV, &act, &oldact) ||
        sigaction(SIGPIPE, &act, &oldact) ||
        sigaction(SIGALRM, &act, &oldact) ||
        sigaction(SIGTERM, &act, &oldact))
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
    pOsalFunction->fpFunctionMutexLockSilent = NULL;
    pOsalFunction->fpFunctionMutexUnlockSilent = NULL;

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

/**************************************************************************/

static ClRcT
cosPosixInit ()
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

    gOsalFunction.fpFunctionMutexLockSilent = cosMutexLockSilent;
    gOsalFunction.fpFunctionMutexUnlockSilent = cosMutexUnlockSilent;

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
    gOsalFunction.fpFunctionSemCreate = cosPosixSemCreate;
    gOsalFunction.fpFunctionSemIdGet = cosPosixSemIdGet;
    gOsalFunction.fpFunctionSemLock = cosPosixSemLock;
    gOsalFunction.fpFunctionSemTryLock = cosPosixSemTryLock;
    gOsalFunction.fpFunctionSemUnlock = cosPosixSemUnlock;
    gOsalFunction.fpFunctionSemValueGet = cosPosixSemValueGet;
    gOsalFunction.fpFunctionSemDelete = cosPosixSemDelete;
    gOsalFunction.fpFunctionProcessCreate = cosProcessCreate;
    gOsalFunction.fpFunctionProcessDelete = cosPosixProcessDelete;
    gOsalFunction.fpFunctionProcessWait = cosPosixProcessWait;
    gOsalFunction.fpFunctionProcessSelfIdGet = cosPosixProcessSelfIdGet;
    gOsalFunction.fpFunctionShmCreate = cosPosixShmCreate;
    gOsalFunction.fpFunctionShmIdGet = cosPosixShmIdGet;
    gOsalFunction.fpFunctionShmDelete = cosPosixShmDelete;
    gOsalFunction.fpFunctionShmAttach = cosPosixShmAttach;
    gOsalFunction.fpFunctionShmDetach = cosPosixShmDetach;
    gOsalFunction.fpFunctionShmSecurityModeSet = cosPosixShmSecurityModeSet;
    gOsalFunction.fpFunctionShmSecurityModeGet = cosPosixShmSecurityModeGet;
    gOsalFunction.fpFunctionShmSizeGet = cosPosixShmSizeGet;
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
