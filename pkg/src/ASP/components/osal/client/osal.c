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
 * File        : osal.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This module implements OS abstraction layer                            
 **************************************************************************/

/* INCLUDES */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#include <clCommon.h>
#include <clCommonErrors.h>
#include <clTimerApi.h>
#include <clOsalApi.h>
#include <clOsalErrors.h>
#include <clDebugApi.h>
#include <clCntApi.h>
#include <clLogApi.h>
#include "osal.h"
#include <clDbg.h>
#ifdef CL_OSAL_DEBUG
#include "clPosixDebug.h"
#endif

#define CL_OSAL_DEPRECATED()  do {                              \
    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Usage of %s is deprecated." \
                                   "Please use heap APIs\n",    \
                                   __FUNCTION__));              \
}while(0)

#define CHK_INIT_RET(fn) do {  \
    if(NULL == gOsalFunction.fn)  \
    { \
      clLogWarning(CL_OSAL_AREA, "INI", "Function " #fn " is not available.  OS adaption layer is probably not initialized.  Delaying this thread..."); \
      clDbgCodeError(CL_OSAL_RC(CL_OSAL_ERR_OS_ERROR), ("Function " #fn " is not available. OSAL not inited")); \
      sleep(2); \
      if(NULL == gOsalFunction.fn) \
        { \
          clLogCritical(CL_OSAL_AREA,"INI", "Function " #fn " is not available.  OS adaption layer is probably not initialized.  Failing call"); \
          return(CL_OSAL_RC(CL_OSAL_ERR_OS_ERROR)); \
        } \
    } \
} while(0)


ClOsalShmAreaDefT *gpClShmArea = NULL;
ClOsalShmIdT gClCompUniqueShmId = 0;

/*FIXME: #include <cosCompId.h>*/
/**************************************************************************/

ClRcT
clOsalFinalize ()
{
    CHK_INIT_RET(fpFunctionCleanup);
    ClRcT ret = gOsalFunction.fpFunctionCleanup(&gOsalFunction);
    return (ret);
}

/**************************************************************************/

ClRcT 
clOsalTaskCreateDetached (const ClCharT* taskName, ClOsalSchedulePolicyT schedulePolicy, ClUint32T priority, ClUint32T stackSize,
		      void* (*fpTaskFunction)(void*),
		      void* pTaskFuncArgument)
{
  CHK_INIT_RET(fpFunctionTaskCreateDetached);
  ClRcT ret = gOsalFunction.fpFunctionTaskCreateDetached(taskName,schedulePolicy, priority, stackSize, fpTaskFunction, pTaskFuncArgument);
  if (clDbgReverseTiming)   /* Allows the child thread to run BEFORE this parent, which will discover race conditions */
    {
      ClTimerTimeOutT timer = { 0, clDbgReverseTiming };
      clOsalTaskDelay(timer);
    }
  return ret;
}

ClRcT 
clOsalTaskCreateAttached(const ClCharT* taskName, ClOsalSchedulePolicyT schedulePolicy, ClUint32T priority, ClUint32T stackSize,
		      void* (*fpTaskFunction)(void*),
		      void* pTaskFuncArgument,ClOsalTaskIdT* pTaskId)
{
  CHK_INIT_RET(fpFunctionTaskCreateAttached);
  ClRcT ret = gOsalFunction.fpFunctionTaskCreateAttached(taskName,schedulePolicy, priority, stackSize, fpTaskFunction, pTaskFuncArgument, pTaskId);
  if (clDbgReverseTiming)  /* Allows the child thread to run BEFORE this parent, which will discover race conditions */
    {
      ClTimerTimeOutT timer = { 0, clDbgReverseTiming };
      clOsalTaskDelay(timer);
    }
  return ret;
}



/**************************************************************************/

ClRcT 
clOsalTaskDelete (ClOsalTaskIdT taskId)
{
    CHK_INIT_RET(fpFunctionTaskDelete);
    return (gOsalFunction.fpFunctionTaskDelete(taskId));
}

ClRcT 
clOsalTaskKill (ClOsalTaskIdT taskId, ClInt32T sig)
{
    CHK_INIT_RET(fpFunctionTaskKill);
    return (gOsalFunction.fpFunctionTaskKill(taskId, sig));
}

ClRcT 
clOsalTaskDetach (ClOsalTaskIdT taskId)
{
    CHK_INIT_RET(fpFunctionTaskDetach);
    return (gOsalFunction.fpFunctionTaskDetach(taskId));
}

ClRcT clOsalTaskJoin (ClOsalTaskIdT taskId)
{
    CHK_INIT_RET(fpFunctionTaskJoin);
    return (gOsalFunction.fpFunctionTaskJoin(taskId));
}


#if 0 /* Deprecated R4.0.  Use the standard C library routines */
/**************************************************************************/

ClRcT
clOsalFileOpen (ClCharT *file, ClInt32T flags, ClHandleT *fd)
{
    CHK_INIT_RET(fpFunctionFileOpen);
    return (gOsalFunction.fpFunctionFileOpen(file, flags, fd));
}

/**************************************************************************/

ClRcT
clOsalFileClose (ClFdT *fd)
{
    CHK_INIT_RET(fpFunctionFileClose);
    return (gOsalFunction.fpFunctionFileClose(fd));
}
#endif

ClRcT
clOsalShmClose (ClFdT *fd)
{
    CHK_INIT_RET(fpFunctionFileClose);
    return (gOsalFunction.fpFunctionFileClose(fd));
}
/**************************************************************************/

ClRcT
clOsalMmap(ClPtrT start, ClUint32T length, ClInt32T prot, ClInt32T flags,
       ClHandleT fd, ClHandleT offset, ClPtrT *mmapped)
{
    CHK_INIT_RET(fpFunctionMmap);
    return (gOsalFunction.fpFunctionMmap(start, length, prot, flags, fd, offset, mmapped));
}

/**************************************************************************/

ClRcT
clOsalMunmap(ClPtrT start, ClUint32T length)
{
    CHK_INIT_RET(fpFunctionMunmap);
    return (gOsalFunction.fpFunctionMunmap(start, length));
}

/**************************************************************************/

ClRcT
clOsalMsync (ClPtrT start, ClUint32T length, ClInt32T flags)
{
    CHK_INIT_RET(fpFunctionMsync);
    return (gOsalFunction.fpFunctionMsync(start, length, flags));
}

/**************************************************************************/

ClRcT
clOsalShmOpen (const ClCharT *name, ClInt32T oflag, ClUint32T mode, ClFdT *fd)
{
    CHK_INIT_RET(fpFunctionShmOpen);
    return (gOsalFunction.fpFunctionShmOpen(name, oflag, mode, fd));
}

/**************************************************************************/

ClRcT clOsalShmUnlink (const ClCharT *name)
{
    CHK_INIT_RET(fpShmUnlink);
    return (gOsalFunction.fpShmUnlink(name));
}

/**************************************************************************/

ClRcT
clOsalFtruncate (ClFdT fd, off_t length)
{
    CHK_INIT_RET(fpFunctionFtruncate);
    return (gOsalFunction.fpFunctionFtruncate(fd, length));
}

/**************************************************************************/

ClRcT 
clOsalSelfTaskIdGet (ClOsalTaskIdT* pTaskId)
{
    CHK_INIT_RET(fpFunctionSelfTaskIdGet);
    return (gOsalFunction.fpFunctionSelfTaskIdGet(pTaskId));
}

/**************************************************************************/

ClRcT 
clOsalTaskNameGet (ClOsalTaskIdT taskId, ClCharT** ppTaskName)
{
    CHK_INIT_RET(fpFunctionTaskNameGet);
    return (gOsalFunction.fpFunctionTaskNameGet(taskId,(ClUint8T**) ppTaskName));
}
/**************************************************************************/

ClRcT 
clOsalTaskPriorityGet (ClOsalTaskIdT taskId,ClUint32T* pTaskPriority)
{
    CHK_INIT_RET(fpFunctionTaskPriorityGet);
    return (gOsalFunction.fpFunctionTaskPriorityGet(taskId, pTaskPriority));
}

/**************************************************************************/

ClRcT 
clOsalTaskPrioritySet (ClOsalTaskIdT taskId,ClUint32T taskPriority)
{
    CHK_INIT_RET(fpFunctionTaskPrioritySet);
    return (gOsalFunction.fpFunctionTaskPrioritySet(taskId,taskPriority));
}

/**************************************************************************/

ClRcT 
clOsalTaskDelay (ClTimerTimeOutT timer)
{
    CHK_INIT_RET(fpFunctionTaskDelay);
    return (gOsalFunction.fpFunctionTaskDelay(timer));
}

/**************************************************************************/

ClTimeT 
clOsalStopWatchTimeGet(void)
{
    CHK_INIT_RET(fpFunctionStopWatchTimeGet);
    return (gOsalFunction.fpFunctionStopWatchTimeGet());
}

/**************************************************************************/

ClRcT 
clOsalTimeOfDayGet (ClTimerTimeOutT* pTime)
{
    CHK_INIT_RET(fpFunctionTimeOfDayGet);
    return (gOsalFunction.fpFunctionTimeOfDayGet(pTime));
}

ClRcT 
clOsalNanoTimeGet (ClNanoTimeT* pTime)
{
    CHK_INIT_RET(fpNanoTimeGet);
    return (gOsalFunction.fpNanoTimeGet(pTime));
}

/**************************************************************************/

ClRcT
clOsalMutexAttrInit(ClOsalMutexAttrT *pAttr)
{
    CL_OSAL_DEPRECATED();
    CHK_INIT_RET(fpMutexAttrInit);
    return (gOsalFunction.fpMutexAttrInit(pAttr));
}

ClRcT
clOsalMutexAttrDestroy(ClOsalMutexAttrT *pAttr)
{
    CL_OSAL_DEPRECATED();
    CHK_INIT_RET(fpMutexAttrDestroy);
    return (gOsalFunction.fpMutexAttrDestroy(pAttr));
}

ClRcT
clOsalMutexAttrPSharedSet(ClOsalMutexAttrT *pAttr, 
        ClOsalSharedTypeT type)
{
    CL_OSAL_DEPRECATED();
    CHK_INIT_RET(fpMutexAttrPSharedSet);
    return (gOsalFunction.fpMutexAttrPSharedSet(pAttr, type));
}

#ifndef CL_OSAL_DEBUG
ClRcT 
clOsalMutexInit (ClOsalMutexT* pMutex)
{
    CHK_INIT_RET(fpFunctionMutexInit);
    return (gOsalFunction.fpFunctionMutexInit(pMutex));
}
#else
ClRcT 
clOsalMutexInitDebug (ClOsalMutexT* pMutex, const ClCharT *file, ClInt32T line)
{
    CHK_INIT_RET(fpFunctionMutexInitDebug);
    return (gOsalFunction.fpFunctionMutexInitDebug(pMutex, file, line));
}
#endif

ClRcT 
clOsalProcessSharedMutexInit (ClOsalMutexT* pMutex, ClOsalSharedMutexFlagsT flags, ClUint8T *pKey, ClUint32T keyLen, ClInt32T value)
{
    CHK_INIT_RET(fpFunctionProcessSharedMutexInit);
    return (gOsalFunction.fpFunctionProcessSharedMutexInit(pMutex, flags, pKey, keyLen, value));
}


ClRcT
clOsalMutexInitEx(ClOsalMutexT *pMutex, ClOsalMutexAttrT *pAttr)
{
    CHK_INIT_RET(fpFunctionMutexInit);
    CL_OSAL_DEPRECATED();

    /* GAS BUG, the Attr parameter is lost! */
    return (gOsalFunction.fpFunctionMutexInit(pMutex));
}

ClRcT
clOsalMutexErrorCheckInit(ClOsalMutexT *pMutex)
{
    CHK_INIT_RET(fpFunctionMutexErrorCheckInit);
    /* GAS BUG, the Attr parameter is lost! */
    return (gOsalFunction.fpFunctionMutexErrorCheckInit(pMutex));
}


#ifndef CL_OSAL_DEBUG
ClRcT
clOsalRecursiveMutexInit (ClOsalMutexT* pMutex)
{
    CHK_INIT_RET(fpFunctionRecursiveMutexInit);
    return (gOsalFunction.fpFunctionRecursiveMutexInit(pMutex));
}

ClRcT 
clOsalMutexCreate (ClOsalMutexIdT* pMutexId)
{
    CHK_INIT_RET(fpFunctionMutexCreate);
    return (gOsalFunction.fpFunctionMutexCreate(pMutexId));
}

#else

ClRcT
clOsalRecursiveMutexInitDebug(ClOsalMutexT* pMutex, const ClCharT *file, ClInt32T line)
{
    CHK_INIT_RET(fpFunctionRecursiveMutexInitDebug);
    return (gOsalFunction.fpFunctionRecursiveMutexInitDebug(pMutex, file, line));
}

ClRcT 
clOsalMutexCreateDebug (ClOsalMutexIdT* pMutexId, const ClCharT *file, ClInt32T line)
{
    CHK_INIT_RET(fpFunctionMutexCreateDebug);
    return (gOsalFunction.fpFunctionMutexCreateDebug(pMutexId, file, line));
}

#endif

ClRcT 
clOsalMutexErrorCheckCreate (ClOsalMutexIdT* pMutexId)
{
    CHK_INIT_RET(fpFunctionMutexErrorCheckCreate);
    return (gOsalFunction.fpFunctionMutexErrorCheckCreate(pMutexId));
}

ClRcT 
clOsalSharedMutexCreate (ClOsalMutexIdT* pMutexId, ClOsalSharedMutexFlagsT flags, ClUint8T *pKey, ClUint32T keyLen, ClInt32T value)
{
    CHK_INIT_RET(fpFunctionSharedMutexCreate);
    return (gOsalFunction.fpFunctionSharedMutexCreate(pMutexId, flags, pKey, keyLen, value));
}

/*******************************************************************/

#ifndef CL_OSAL_DEBUG
ClRcT 
clOsalMutexCreateAndLock (ClOsalMutexIdT* pMutexId)
{
    ClRcT returnCode = CL_OK;

    returnCode = clOsalMutexCreate(pMutexId);
    if(CL_OK != returnCode) {
        return(returnCode);
    }

    returnCode = clOsalMutexLock(*pMutexId);
    if(CL_OK != returnCode) {
        return(returnCode);
    }

    return(CL_OK);

}

#else

ClRcT 
clOsalMutexCreateAndLockDebug (ClOsalMutexIdT* pMutexId, const ClCharT *file, ClInt32T line)
{
    ClRcT returnCode = CL_OK;

    returnCode = clOsalMutexCreateDebug(pMutexId, file, line);
    if(CL_OK != returnCode) {
        return(returnCode);
    }

    returnCode = clOsalMutexLockDebug(*pMutexId, file, line);
    if(CL_OK != returnCode) {
        return(returnCode);
    }

    return(CL_OK);

}
#endif

/*******************************************************************/
#ifndef CL_OSAL_DEBUG
ClRcT 
clOsalMutexDelete (ClOsalMutexIdT mutexId)
{
    CHK_INIT_RET(fpFunctionMutexDelete);
    return (gOsalFunction.fpFunctionMutexDelete(mutexId));
}

ClRcT 
clOsalMutexDestroy (ClOsalMutexT *pMutex)
{
    CHK_INIT_RET(fpFunctionMutexDestroy);
    return (gOsalFunction.fpFunctionMutexDestroy(pMutex));
}

#else

ClRcT 
clOsalMutexDeleteDebug (ClOsalMutexIdT mutexId, const ClCharT *file, ClInt32T line)
{
    CHK_INIT_RET(fpFunctionMutexDeleteDebug);
    return (gOsalFunction.fpFunctionMutexDeleteDebug(mutexId, file, line));
}

ClRcT 
clOsalMutexDestroyDebug (ClOsalMutexT *pMutex, const ClCharT *file, ClInt32T line)
{
    CHK_INIT_RET(fpFunctionMutexDestroyDebug);
    return (gOsalFunction.fpFunctionMutexDestroyDebug(pMutex, file, line));
}

#endif

/**************************************************************************/
ClRcT 
clOsalMutexValueGet (ClOsalMutexIdT mutexId, ClInt32T *pValue)
{
    CHK_INIT_RET(fpFunctionMutexValueGet);
    return (gOsalFunction.fpFunctionMutexValueGet(mutexId, pValue));
}

/**************************************************************************/
ClRcT 
clOsalMutexValueSet (ClOsalMutexIdT mutexId, ClInt32T value)
{
    CHK_INIT_RET(fpFunctionMutexValueSet);
    return (gOsalFunction.fpFunctionMutexValueSet(mutexId, value));
}

ClRcT
clOsalMutexLockSilent(ClOsalMutexIdT mutexId)
{
    if(!gOsalFunction.fpFunctionMutexLockSilent)
        return CL_OSAL_RC(CL_OSAL_ERR_OS_ERROR);
    return gOsalFunction.fpFunctionMutexLockSilent(mutexId);
}

ClRcT
clOsalMutexUnlockSilent(ClOsalMutexIdT mutexId)
{
    if(!gOsalFunction.fpFunctionMutexUnlockSilent)
        return CL_OSAL_RC(CL_OSAL_ERR_OS_ERROR);
    return gOsalFunction.fpFunctionMutexUnlockSilent(mutexId);
}

/**************************************************************************/
#ifndef CL_OSAL_DEBUG

ClRcT 
clOsalMutexLock (ClOsalMutexIdT mutexId)
{
    CHK_INIT_RET(fpFunctionMutexLock);
    return (gOsalFunction.fpFunctionMutexLock(mutexId));
}

/**************************************************************************/
ClRcT 
clOsalMutexUnlock (ClOsalMutexIdT mutexId)
{
    CHK_INIT_RET(fpFunctionMutexUnlock);
    return (gOsalFunction.fpFunctionMutexUnlock(mutexId));
}

#else

ClRcT 
clOsalMutexLockDebug (ClOsalMutexIdT mutexId, const ClCharT *file, ClInt32T line)
{
    CHK_INIT_RET(fpFunctionMutexLockDebug);
    return (gOsalFunction.fpFunctionMutexLockDebug(mutexId, file, line));
}

/**************************************************************************/
ClRcT 
clOsalMutexUnlockDebug (ClOsalMutexIdT mutexId, const ClCharT *file, ClInt32T line)
{
    CHK_INIT_RET(fpFunctionMutexUnlockDebug);
    return (gOsalFunction.fpFunctionMutexUnlockDebug(mutexId, file, line));
}

ClRcT 
clOsalBustLocks(void)
{
    cosPosixMutexPoolBustLocks();
    return CL_OK;
}
#endif

/*
 * Backdoor to avoid osal mutex debugging.
 */
ClRcT 
clOsalMutexUnlockNonDebug(ClOsalMutexIdT mutexId)
{
    CHK_INIT_RET(fpFunctionMutexUnlock);
    return (gOsalFunction.fpFunctionMutexUnlock(mutexId));
}

ClRcT
clOsalMutexTryLock(ClOsalMutexIdT mutexId)
{
    if(!gOsalFunction.fpFunctionMutexTryLock)
    {
        sleep(2);
        if(!gOsalFunction.fpFunctionMutexTryLock)
            return CL_OSAL_RC(CL_OSAL_ERR_OS_ERROR);
    }
    return gOsalFunction.fpFunctionMutexTryLock(mutexId);
}

ClRcT
clOsalCondAttrInit(ClOsalCondAttrT *pAttr)
{
    CL_OSAL_DEPRECATED();
    CHK_INIT_RET(fpCondAttrInit);
    return (gOsalFunction.fpCondAttrInit(pAttr));
}

ClRcT
clOsalCondAttrDestroy(ClOsalCondAttrT *pAttr)
{
    CL_OSAL_DEPRECATED();
    CHK_INIT_RET(fpCondAttrDestroy);
    return (gOsalFunction.fpCondAttrDestroy(pAttr));
}

ClRcT
clOsalCondAttrPSharedSet(ClOsalCondAttrT *pAttr, 
        ClOsalSharedTypeT type)
{
    CL_OSAL_DEPRECATED();
    CHK_INIT_RET(fpCondAttrPSharedSet);
    return (gOsalFunction.fpCondAttrPSharedSet(pAttr, type));
}

ClRcT 
clOsalCondInit (ClOsalCondT* pCond)
{
    CHK_INIT_RET(fpFunctionCondInit);
    return (gOsalFunction.fpFunctionCondInit(pCond));
}

ClRcT 
clOsalProcessSharedCondInit (ClOsalCondT* pCond)
{
    CHK_INIT_RET(fpFunctionProcessSharedCondInit);
    return (gOsalFunction.fpFunctionProcessSharedCondInit(pCond));
}



/**************************************************************************/

ClRcT
clOsalCondInitEx(ClOsalCondT *pCond, ClOsalCondAttrT *pAttr)
{
    CL_OSAL_DEPRECATED();
    CHK_INIT_RET(fpFunctionCondInitEx);
    return (gOsalFunction.fpFunctionCondInitEx(pCond, pAttr));
}

ClRcT 
clOsalCondCreate (ClOsalCondIdT* pConditionId)
{
    CHK_INIT_RET(fpFunctionCondCreate);
    return (gOsalFunction.fpFunctionCondCreate(pConditionId));
}

/**************************************************************************/

ClRcT 
clOsalCondDelete (ClOsalCondIdT conditionId)
{
    CHK_INIT_RET(fpFunctionCondDelete);
    return (gOsalFunction.fpFunctionCondDelete(conditionId));
}

/**************************************************************************/

ClRcT 
clOsalCondDestroy (ClOsalCondT *pCond)
{
    CHK_INIT_RET(fpFunctionCondDestroy);
    return (gOsalFunction.fpFunctionCondDestroy(pCond));
}

/**************************************************************************/
#ifndef CL_OSAL_DEBUG

ClRcT 
clOsalCondWait (ClOsalCondIdT conditionId, ClOsalMutexIdT mutexId, ClTimerTimeOutT timer)
{
    CHK_INIT_RET(fpFunctionCondWait);
    return (gOsalFunction.fpFunctionCondWait(conditionId,mutexId,timer));
}

#else

ClRcT 
clOsalCondWaitDebug (ClOsalCondIdT conditionId, ClOsalMutexIdT mutexId, ClTimerTimeOutT timer,
                     const ClCharT *file, ClInt32T line)
{
    CHK_INIT_RET(fpFunctionCondWaitDebug);
    return (gOsalFunction.fpFunctionCondWaitDebug(conditionId, mutexId, timer, file, line));
}


#endif

/**************************************************************************/

ClRcT 
clOsalCondBroadcast (ClOsalCondIdT conditionId)
{
    CHK_INIT_RET(fpFunctionCondBroadcast);
    return (gOsalFunction.fpFunctionCondBroadcast(conditionId));
}

/**************************************************************************/

ClRcT 
clOsalCondSignal (ClOsalCondIdT conditionId)
{
    CHK_INIT_RET(fpFunctionCondSignal);
    return (gOsalFunction.fpFunctionCondSignal(conditionId));
}

/**************************************************************************/

ClRcT 
clOsalTaskDataSet(ClUint32T key, ClOsalTaskDataT threadData)
{
    CHK_INIT_RET(fpFunctionTaskDataSet);
    return (gOsalFunction.fpFunctionTaskDataSet(key, threadData));
}

/**************************************************************************/

ClRcT 
clOsalTaskDataGet(ClUint32T key, ClOsalTaskDataT* pThreadData)
{
    CHK_INIT_RET(fpFunctionTaskDataGet);
    return (gOsalFunction.fpFunctionTaskDataGet(key, pThreadData));
}

/**************************************************************************/

ClRcT
clOsalPrintf(const ClCharT* fmt, ...)
{
    ClRcT retCode;
    ClInt32T result;
    va_list arguments;
    va_start(arguments, fmt);
    result = vfprintf(stdout, fmt, arguments);
    va_end(arguments);
    
    if(0 > result)
    {
        retCode = CL_OSAL_RC(CL_OSAL_ERR_OS_ERROR);
        return(retCode); 
    }
    else
    {
        return(CL_OK);
    }
}

/**************************************************************************/

ClRcT 
clOsalTaskKeyCreate(ClUint32T* pKey,  ClOsalTaskKeyDeleteCallBackT pCallbackFunc)
{
    CHK_INIT_RET(fpFunctionTaskKeyCreate);
    return (gOsalFunction.fpFunctionTaskKeyCreate(pKey, pCallbackFunc));
}

/**************************************************************************/

ClRcT 
clOsalTaskKeyDelete(ClUint32T key)
{
    CHK_INIT_RET(fpFunctionTaskKeyDelete);
    return (gOsalFunction.fpFunctionTaskKeyDelete(key));
}

/**************************************************************************/

ClRcT 
clOsalSemCreate(ClUint8T* pName, ClUint32T value, ClOsalSemIdT* pSemId)
{
    CHK_INIT_RET(fpFunctionSemCreate);
    return (gOsalFunction.fpFunctionSemCreate(pName, value, pSemId));
}

/**************************************************************************/
ClRcT 
clOsalSemIdGet(ClUint8T* pName, ClOsalSemIdT* pSemId)
{
    CHK_INIT_RET(fpFunctionSemIdGet);
    return (gOsalFunction.fpFunctionSemIdGet(pName, pSemId));
}

/**************************************************************************/

ClRcT 
clOsalSemLock(ClOsalSemIdT semId)
{
    CHK_INIT_RET(fpFunctionSemLock);
    return (gOsalFunction.fpFunctionSemLock(semId));
}

/**************************************************************************/

ClRcT 
clOsalSemTryLock(ClOsalSemIdT semId)
{
    CHK_INIT_RET(fpFunctionSemTryLock);
    return (gOsalFunction.fpFunctionSemTryLock(semId));
}

/**************************************************************************/

ClRcT 
clOsalSemUnlock(ClOsalSemIdT semId)
{
    CHK_INIT_RET(fpFunctionSemUnlock);
    return (gOsalFunction.fpFunctionSemUnlock(semId));
}

/**************************************************************************/

ClRcT
clOsalSemValueGet(ClOsalSemIdT semId, ClUint32T* pSemValue)
{
    CHK_INIT_RET(fpFunctionSemValueGet);
    return (gOsalFunction.fpFunctionSemValueGet(semId, pSemValue));
}

/**************************************************************************/

ClRcT 
clOsalSemDelete (ClOsalSemIdT semId)
{
    CHK_INIT_RET(fpFunctionSemDelete);
    return (gOsalFunction.fpFunctionSemDelete(semId));
}

/**************************************************************************/

ClRcT 
clOsalProcessCreate(ClOsalProcessFuncT fpFunction, 
                 void* processArg, 
                 ClOsalProcessFlagT creationFlags,
                 ClOsalPidT* pProcessId)
{
    CHK_INIT_RET(fpFunctionProcessCreate);
    return (gOsalFunction.fpFunctionProcessCreate(fpFunction, 
                                                  processArg, 
                                                  creationFlags,
                                                  pProcessId));
}

/**************************************************************************/

ClRcT 
clOsalProcessSelfIdGet(ClOsalPidT* pProcessId)
{
    CHK_INIT_RET(fpFunctionProcessSelfIdGet);
    return (gOsalFunction.fpFunctionProcessSelfIdGet(pProcessId));
}

/**************************************************************************/

ClRcT 
clOsalProcessDelete(ClOsalPidT processId)
{
    CHK_INIT_RET(fpFunctionProcessDelete);
    return (gOsalFunction.fpFunctionProcessDelete(processId));
}

/**************************************************************************/

ClRcT 
clOsalProcessWait(ClOsalPidT processId)
{
    CHK_INIT_RET(fpFunctionProcessWait);
    return (gOsalFunction.fpFunctionProcessWait(processId));
}

/**************************************************************************/

#ifndef POSIX_BUILD
ClRcT 
clOsalShmCreate(ClUint8T* pName, ClUint32T size, ClOsalShmIdT* pShmId)
{
    CHK_INIT_RET(fpFunctionShmCreate);
    return (gOsalFunction.fpFunctionShmCreate(pName, size, pShmId));
}

/**************************************************************************/
ClRcT 
clOsalShmIdGet(ClUint8T* pName, ClOsalShmIdT* pShmId)
{
    CHK_INIT_RET(fpFunctionShmIdGet);
    return (gOsalFunction.fpFunctionShmIdGet(pName, pShmId));
}

/**************************************************************************/

ClRcT 
clOsalShmDelete(ClOsalShmIdT shmId)
{
    CHK_INIT_RET(fpFunctionShmDelete);
    return (gOsalFunction.fpFunctionShmDelete(shmId));
}

/**************************************************************************/

ClRcT 
clOsalShmAttach(ClOsalShmIdT shmId, void* pInMem, void** ppOutMem)
{
    CHK_INIT_RET(fpFunctionShmAttach);
    return (gOsalFunction.fpFunctionShmAttach(shmId,pInMem, ppOutMem));
}

/**************************************************************************/

ClRcT 
clOsalShmDetach(void* pMem)
{
    CHK_INIT_RET(fpFunctionShmDetach);
    return (gOsalFunction.fpFunctionShmDetach(pMem));
}

/**************************************************************************/

ClRcT 
clOsalShmSecurityModeSet(ClOsalShmIdT shmId, ClUint32T mode)
{
    CHK_INIT_RET(fpFunctionShmSecurityModeSet);
    return (gOsalFunction.fpFunctionShmSecurityModeSet(shmId,mode));
}

/**************************************************************************/
ClRcT 
clOsalShmSecurityModeGet(ClOsalShmIdT shmId, ClUint32T* pMode)
{
    CHK_INIT_RET(fpFunctionShmSecurityModeGet);
    return (gOsalFunction.fpFunctionShmSecurityModeGet(shmId,pMode));
}

#endif
/**************************************************************************/

ClRcT 
clOsalShmSizeGet(ClOsalShmIdT shmId,ClUint32T* pSize)
{
    CHK_INIT_RET(fpFunctionShmSizeGet);
    return (gOsalFunction.fpFunctionShmSizeGet(shmId,pSize));
}

ClPtrT clOsalMalloc(ClUint32T size)
{
    CL_OSAL_DEPRECATED();
    return clHeapAllocate(size);
}

ClPtrT clOsalCalloc(ClUint32T size)
{
    CL_OSAL_DEPRECATED();
    return clHeapCalloc(1,size);
}

void clOsalFree(ClPtrT pAddress)
{
    CL_OSAL_DEPRECATED();
    clHeapFree(pAddress);
}

/**************************************************************************/

ClRcT
clOsalMaxPathGet(const ClCharT* path, ClInt32T* pLength)
{
    CHK_INIT_RET(fpMaxPathGet);
    return gOsalFunction.fpMaxPathGet(path, pLength);
}

ClRcT
clOsalPageSizeGet(ClInt32T* pSize)
{
    CHK_INIT_RET(fpPageSizeGet);
    return gOsalFunction.fpPageSizeGet(pSize);
}

