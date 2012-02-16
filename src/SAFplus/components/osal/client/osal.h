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
 * File        : osal.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This module implements OS abstraction layer                            
 **************************************************************************/

#ifndef __COS_H
#define __COS_H

#ifdef __cplusplus
extern "C" {
#endif

#define COS_MIN_PRIORITY    1
#define COS_MAX_PRIORITY    160 
   
/* Semaphore defines*/
#define CL_SEM_MAX_VALUE    32767

#define NUMBER_OF_BUCKETS 100
#define WORD_SIZE sizeof(ClUint32T)
#define MEM_POOL_SIZE 4096
#define DESC_POOL_KEY 52583 

struct osalFunction_t;

/**************************************************************************/
typedef ClRcT
(*fpCosTaskCreateAttached)(const ClCharT* taskName,
 ClOsalSchedulePolicyT schedulePolicy,
 ClUint32T priority,
 ClUint32T stackSize,
 void* (*fpTaskFunction)(void*),
 void* pTaskFuncArgument,
 ClOsalTaskIdT* pTaskId);
/**************************************************************************/
typedef ClRcT
(*fpCosTaskCreateDetached)(const ClCharT* taskName,
 ClOsalSchedulePolicyT schedulePolicy,
 ClUint32T priority,
 ClUint32T stackSize,
 void* (*fpTaskFunction)(void*),
 void* pTaskFuncArgument);
/**************************************************************************/
typedef ClRcT
(*fpCosFileOpen)(ClCharT *file, ClInt32T flags, ClHandleT *fd);
/**************************************************************************/
typedef ClRcT
(*fpCosFileClose)(ClFdT *fd);
/**************************************************************************/
typedef ClRcT
(*fpCosMmap)(ClPtrT start, ClUint32T length, ClInt32T prot, ClInt32T flags,
                  ClHandleT fd, ClHandleT offset, ClPtrT *mmapped);
/**************************************************************************/
typedef ClRcT
(*fpCosMunmap)(ClPtrT start, ClUint32T length);
/**************************************************************************/
typedef ClRcT
(*fpCosMsync)(ClPtrT start, ClUint32T length, ClInt32T flags);
/**************************************************************************/
typedef ClRcT
(*fpCosShmOpen)(const ClCharT *name, ClInt32T oflag, ClUint32T mode, ClFdT *fd);
/**************************************************************************/
typedef ClRcT
(*fpCosShmUnlink)(const ClCharT *name);
/**************************************************************************/
typedef ClRcT
(*fpCosFtruncate)(ClFdT fd, off_t length);
/**************************************************************************/
typedef ClRcT
(*fpCosTaskDelete)(ClOsalTaskIdT taskId);
/**************************************************************************/
typedef ClRcT
(*fpCosTaskKill)(ClOsalTaskIdT taskId, ClInt32T sig);
/**************************************************************************/
typedef ClRcT
(*fpCosTask)(ClOsalTaskIdT taskId);
/**************************************************************************/
typedef ClRcT
(*fpCosSelfTaskIdGet)(ClOsalTaskIdT* pTaskId);
/**************************************************************************/
typedef ClRcT
(*fpCosTaskNameGet)(ClOsalTaskIdT taskId, ClUint8T** ppTaskName);
/**************************************************************************/
typedef ClRcT
(*fpCosTaskPriorityGet)(ClOsalTaskIdT taskId, ClUint32T* pTaskPriority);
/**************************************************************************/
typedef ClRcT
(*fpCosTaskPrioritySet)(ClOsalTaskIdT taskId, ClUint32T taskPriority);
/**************************************************************************/
typedef ClRcT
(*fpCosTaskDelay)(ClTimerTimeOutT time);
/**************************************************************************/
typedef ClTimeT 
(*fpCosStopWatchTimeGet)(void);
/**************************************************************************/
typedef ClRcT 
(*fpCosTimeOfDayGet)(ClTimerTimeOutT* pTime);
/**************************************************************************/
typedef ClRcT 
(*fpCosNanoTimeGet)(ClNanoTimeT* pTime);
/**************************************************************************/
typedef ClRcT 
(*fpCosMutexAttrInit)(ClOsalMutexAttrT* pAttr);
/**************************************************************************/
typedef ClRcT 
(*fpCosMutexAttrDestroy)(ClOsalMutexAttrT* pAttr);
/**************************************************************************/
typedef ClRcT 
(*fpCosMutexAttrPSharedSet)(ClOsalMutexAttrT* pAttr, ClOsalSharedTypeT type);
/**************************************************************************/
typedef ClRcT
(*fpCosMutexInit)(ClOsalMutexT* pMutex);
/**************************************************************************/
typedef ClRcT
(*fpCosMutexErrorCheckInit)(ClOsalMutexT* pMutex);
/**************************************************************************/
typedef ClRcT
(*fpCosSharedMutexInit)(ClOsalMutexT* pMutex, ClOsalSharedMutexFlagsT flags, ClUint8T *pKey, ClUint32T keyLen, ClInt32T value);
/**************************************************************************/
typedef ClRcT
(*fpCosMutexInitEx)(ClOsalMutexT* pMutex, ClOsalMutexAttrT *pAttr);
/**************************************************************************/
typedef ClRcT
(*fpCosMutexCreate)(ClOsalMutexIdT* pMutexId);
/**************************************************************************/
typedef ClRcT
(*fpCosMutexErrorCheckCreate)(ClOsalMutexIdT* pMutexId);
/**************************************************************************/
typedef ClRcT
(*fpCosSharedMutexCreate)(ClOsalMutexIdT* pMutexId, ClOsalSharedMutexFlagsT flags, ClUint8T *pKey, ClUint32T keyLen, ClInt32T value);
/**************************************************************************/
typedef ClRcT
(*fpCosMutexValueSet)(ClOsalMutexIdT mutexId, ClInt32T value);
/**************************************************************************/
typedef ClRcT
(*fpCosMutexValueGet)(ClOsalMutexIdT mutexId, ClInt32T *pValue);
/**************************************************************************/
typedef ClRcT
(*fpCosMutexLock)(ClOsalMutexIdT mutexId);
/**************************************************************************/
typedef ClRcT
(*fpCosMutexLockSilent)(ClOsalMutexIdT mutexId);
/**************************************************************************/
typedef ClRcT
(*fpCosMutexUnlockSilent)(ClOsalMutexIdT mutexId);
/**************************************************************************/
typedef ClRcT
(*fpCosMutexTryLock)(ClOsalMutexIdT mutexId);
/**************************************************************************/
typedef ClRcT
(*fpCosMutexUnlock)(ClOsalMutexIdT mutexId);
/**************************************************************************/
typedef ClRcT
(*fpCosMutexDelete)(ClOsalMutexIdT mutexId);
/**************************************************************************/
typedef ClRcT
(*fpCosMutexDestroy)(ClOsalMutexT *pMutex);
#ifdef CL_OSAL_DEBUG
/**************************************************************************/
typedef ClRcT
(*fpCosMutexInitDebug)(ClOsalMutexT* pMutex, const ClCharT *file, ClInt32T line);
/**************************************************************************/
typedef ClRcT
(*fpCosMutexInitExDebug)(ClOsalMutexT* pMutex, ClOsalMutexAttrT *pAttr, const ClCharT *file, ClInt32T line);
/**************************************************************************/
typedef ClRcT
(*fpCosMutexCreateDebug)(ClOsalMutexIdT* pMutexId, const ClCharT *file, ClInt32T line);
/**************************************************************************/
typedef ClRcT
(*fpCosMutexLockDebug)(ClOsalMutexIdT mutexId, const ClCharT *file, ClInt32T line);
/**************************************************************************/
typedef ClRcT
(*fpCosMutexUnlockDebug)(ClOsalMutexIdT mutexId, const ClCharT *file, ClInt32T line);
/**************************************************************************/
typedef ClRcT
(*fpCosMutexDeleteDebug)(ClOsalMutexIdT mutexId, const ClCharT *file, ClInt32T line);
/**************************************************************************/
typedef ClRcT
(*fpCosMutexDestroyDebug)(ClOsalMutexT *pMutex, const ClCharT *file, ClInt32T line);
/**************************************************************************/
#endif

typedef ClRcT
(*fpCosCondCreate)(ClOsalCondIdT *pConditionId);
/**************************************************************************/
typedef ClRcT 
(*fpCosCondAttrInit)(ClOsalCondAttrT* pAttr);
/**************************************************************************/
typedef ClRcT 
(*fpCosCondAttrDestroy)(ClOsalCondAttrT* pAttr);
/**************************************************************************/
typedef ClRcT 
(*fpCosCondAttrPSharedSet)(ClOsalCondAttrT* pAttr, ClOsalSharedTypeT type);
/**************************************************************************/
typedef ClRcT
(*fpCosCondInit)(ClOsalCondT *pCond);
/**************************************************************************/
typedef ClRcT
(*fpCosCondInitEx)(ClOsalCondT* pCond, ClOsalCondAttrT *pAttr);
/**************************************************************************/
typedef ClRcT
(*fpCosCondDelete)(ClOsalCondIdT conditionId);
/**************************************************************************/
typedef ClRcT
(*fpCosCondDestroy)(ClOsalCondT *pCond);
/**************************************************************************/
typedef ClRcT
(*fpCosCondWait)(ClOsalCondIdT conditionId, ClOsalMutexIdT mutexId, ClTimerTimeOutT time);
/**************************************************************************/
#ifdef CL_OSAL_DEBUG
typedef ClRcT
(*fpCosCondWaitDebug)(ClOsalCondIdT conditionId, ClOsalMutexIdT mutexId, ClTimerTimeOutT time,
                      const ClCharT *file, ClInt32T line);
#endif
/**************************************************************************/
typedef ClRcT
(*fpCosCondBroadcast)(ClOsalCondIdT conditionId);
/**************************************************************************/
typedef ClRcT
(*fpCosCondSignal)(ClOsalCondIdT conditionId);
/**************************************************************************/
typedef ClRcT
(*fpCosTaskDataSet)(ClUint32T key, ClOsalTaskDataT threadData);
/**************************************************************************/
typedef ClRcT
(*fpCosTaskDataGet)(ClUint32T key, ClOsalTaskDataT* pThreadData);
/**************************************************************************/
typedef ClRcT
(*fpCosTaskKeyCreate)(ClUint32T* pKey,  ClOsalTaskKeyDeleteCallBackT pCallbackFunc);
/**************************************************************************/
typedef ClRcT
(*fpCosTaskKeyDelete)(ClUint32T key);
/**************************************************************************/
typedef ClRcT
(*fpCosCleanup)(struct osalFunction_t*);
/**************************************************************************/
typedef ClRcT 
(*fpCosSemCreate)(ClUint8T* pName, ClUint32T value, ClOsalSemIdT* pSemId);
/**************************************************************************/
typedef ClRcT 
(*fpCosSemIdGet)(ClUint8T* pName, ClOsalSemIdT* pSemId);
/**************************************************************************/
typedef ClRcT 
(*fpCosSemLock)(ClOsalSemIdT semId);
/**************************************************************************/
typedef ClRcT 
(*fpCosSemTryLock)(ClOsalSemIdT semId);
/**************************************************************************/
typedef ClRcT 
(*fpCosSemUnlock)(ClOsalSemIdT semId);
/**************************************************************************/
typedef ClRcT 
(*fpCosSemValueGet)(ClOsalSemIdT semId, ClUint32T* pSemValue);
/**************************************************************************/
typedef ClRcT 
(*fpCosSemDelete)(ClOsalSemIdT semId);
/**************************************************************************/
typedef ClRcT 
(*fpCosProcessCreate)(ClOsalProcessFuncT fpFunction, 
                      void* processArg, 
                      ClOsalProcessFlagT creationFlags,
                      ClOsalPidT* pProcessId);
/**************************************************************************/
typedef ClRcT 
(*fpCosProcessDelete)(ClOsalPidT processId);
/**************************************************************************/
typedef ClRcT 
(*fpCosProcessWait)(ClOsalPidT processId);
/**************************************************************************/
typedef ClRcT 
(*fpCosProcessSelfIdGet)(ClOsalPidT* pProcessId);
/**************************************************************************/
typedef ClRcT 
(*fpCosShmCreate)(ClUint8T* pName, ClUint32T size, ClOsalShmIdT* pShmId);
/**************************************************************************/
typedef ClRcT 
(*fpCosShmIdGet)(ClUint8T* pName, ClOsalShmIdT* pShmId);
/**************************************************************************/
typedef ClRcT 
(*fpCosShmDelete)(ClOsalShmIdT shmId);
/**************************************************************************/
typedef ClRcT 
(*fpCosShmAttach)(ClOsalShmIdT shmId, void* pInMem, void** ppOutMem);
/**************************************************************************/
typedef ClRcT 
(*fpCosShmDetach)(void* pMem);
/**************************************************************************/
typedef ClRcT 
(*fpCosShmSecurityModeSet)(ClOsalShmIdT shmId, ClUint32T mode);
/**************************************************************************/
typedef ClRcT 
(*fpCosShmSecurityModeGet)(ClOsalShmIdT shmId, ClUint32T* pMode);
/**************************************************************************/
typedef ClRcT 
(*fpCosShmSizeGet)(ClOsalShmIdT shmId, ClUint32T* pSize);
/**************************************************************************/
typedef ClRcT 
(*fpCosMaxPathGet)(const ClCharT* path, ClInt32T* pLength);
/**************************************************************************/
typedef ClRcT 
(*fpCosPageSizeGet)(ClInt32T* pSize);
/**************************************************************************/

typedef struct osalFunction_t
{
	fpCosFileOpen		    fpFunctionFileOpen;
	fpCosFileClose		    fpFunctionFileClose;
	fpCosMmap		    fpFunctionMmap;
	fpCosMunmap		    fpFunctionMunmap;
	fpCosMsync		    fpFunctionMsync;
	fpCosShmOpen		    fpFunctionShmOpen;
        fpCosShmUnlink              fpShmUnlink;
	fpCosFtruncate	            fpFunctionFtruncate;
	fpCosTaskCreateDetached	    fpFunctionTaskCreateDetached;
	fpCosTaskCreateAttached	    fpFunctionTaskCreateAttached;
	fpCosTaskDelete		    fpFunctionTaskDelete;
	fpCosTask		    fpFunctionTaskJoin;
	fpCosTask		    fpFunctionTaskDetach;
	fpCosSelfTaskIdGet	    fpFunctionSelfTaskIdGet;
	fpCosTaskNameGet	    fpFunctionTaskNameGet;
	fpCosTaskPriorityGet	    fpFunctionTaskPriorityGet;
	fpCosTaskPrioritySet	    fpFunctionTaskPrioritySet;
	fpCosTaskDelay		    fpFunctionTaskDelay;
        fpCosTimeOfDayGet           fpFunctionTimeOfDayGet;
    fpCosStopWatchTimeGet       fpFunctionStopWatchTimeGet;
        fpCosNanoTimeGet        fpNanoTimeGet;
        fpCosMutexAttrInit      fpMutexAttrInit;
        fpCosMutexAttrDestroy   fpMutexAttrDestroy;
        fpCosMutexAttrPSharedSet fpMutexAttrPSharedSet;
	fpCosMutexInit              fpFunctionMutexInit;

	fpCosMutexInit              fpFunctionRecursiveMutexInit;

	fpCosSharedMutexInit              fpFunctionProcessSharedMutexInit;
  /*	fpCosMutexInitEx            fpFunctionMutexInitEx; */
	fpCosMutexCreate	    fpFunctionMutexCreate;

	fpCosSharedMutexCreate	    fpFunctionSharedMutexCreate;
    fpCosMutexValueSet      fpFunctionMutexValueSet;
    fpCosMutexValueGet      fpFunctionMutexValueGet;
	fpCosMutexLock		    fpFunctionMutexLock;
	fpCosMutexUnlock	    fpFunctionMutexUnlock;
	fpCosMutexDelete	    fpFunctionMutexDelete;
	fpCosMutexDestroy	    fpFunctionMutexDestroy;

#ifdef CL_OSAL_DEBUG
	fpCosMutexInitDebug     fpFunctionMutexInitDebug;
	fpCosMutexInitDebug     fpFunctionRecursiveMutexInitDebug;
	fpCosMutexCreateDebug	fpFunctionMutexCreateDebug;
	fpCosMutexLockDebug		fpFunctionMutexLockDebug;
	fpCosMutexUnlockDebug	fpFunctionMutexUnlockDebug;
	fpCosMutexDeleteDebug	fpFunctionMutexDeleteDebug;
	fpCosMutexDestroyDebug	fpFunctionMutexDestroyDebug;
#endif

    fpCosCondAttrInit      fpCondAttrInit;
    fpCosCondAttrDestroy   fpCondAttrDestroy;
    fpCosCondAttrPSharedSet fpCondAttrPSharedSet;
    fpCosCondInit               fpFunctionCondInit;
    fpCosCondInit               fpFunctionProcessSharedCondInit;
	fpCosCondInitEx             fpFunctionCondInitEx;
	fpCosCondCreate		    fpFunctionCondCreate;
	fpCosCondDelete		    fpFunctionCondDelete;
	fpCosCondDestroy	    fpFunctionCondDestroy;
	fpCosCondWait		    fpFunctionCondWait;
#ifdef CL_OSAL_DEBUG
	fpCosCondWaitDebug		fpFunctionCondWaitDebug;
#endif
	fpCosCondBroadcast	    fpFunctionCondBroadcast;
	fpCosCondSignal		    fpFunctionCondSignal;
    fpCosTaskDataSet        fpFunctionTaskDataSet;
    fpCosTaskDataGet        fpFunctionTaskDataGet;
    fpCosTaskKeyCreate      fpFunctionTaskKeyCreate;
    fpCosTaskKeyDelete      fpFunctionTaskKeyDelete;
	fpCosCleanup		    fpFunctionCleanup;
    fpCosSemCreate          fpFunctionSemCreate;
    fpCosSemIdGet           fpFunctionSemIdGet;
    fpCosSemLock            fpFunctionSemLock;
    fpCosSemTryLock         fpFunctionSemTryLock;
    fpCosSemUnlock          fpFunctionSemUnlock;
    fpCosSemValueGet        fpFunctionSemValueGet;
    fpCosSemDelete          fpFunctionSemDelete;
    fpCosProcessCreate      fpFunctionProcessCreate;
    fpCosProcessDelete      fpFunctionProcessDelete;
    fpCosProcessWait        fpFunctionProcessWait;
    fpCosProcessSelfIdGet   fpFunctionProcessSelfIdGet;
    fpCosShmCreate          fpFunctionShmCreate;
    fpCosShmIdGet           fpFunctionShmIdGet;
    fpCosShmDelete          fpFunctionShmDelete;
    fpCosShmAttach          fpFunctionShmAttach;
    fpCosShmDetach          fpFunctionShmDetach;
    fpCosShmSecurityModeSet fpFunctionShmSecurityModeSet;
    fpCosShmSecurityModeGet fpFunctionShmSecurityModeGet;
    fpCosShmSizeGet         fpFunctionShmSizeGet;
    fpCosMaxPathGet         fpMaxPathGet;
    fpCosPageSizeGet        fpPageSizeGet;
	fpCosTaskKill 		    fpFunctionTaskKill;
    fpCosMutexTryLock       fpFunctionMutexTryLock;
    fpCosMutexErrorCheckInit       fpFunctionMutexErrorCheckInit;
    fpCosMutexErrorCheckCreate     fpFunctionMutexErrorCheckCreate;
	fpCosMutexLockSilent		   fpFunctionMutexLockSilent;
	fpCosMutexUnlockSilent	       fpFunctionMutexUnlockSilent;
} osalFunction_t;

/* Task structure needed for task wrapper function */
typedef struct CosTaskBlock_s
{
    void* (*fpTaskFunction)(void*);
    void* pArgument;
    int schedPolicy;
    int schedPriority;
    ClCharT taskName[CL_OSAL_NAME_MAX];
} CosTaskBlock_t;

typedef struct CosTaskControl_s
{
    ClUint32T isCosInitialized;
    ClUint32T taskNameKey;	// This now a key fot the pthread specif area
} CosTaskControl_t;

/* This union needs to be defined. */
   
typedef union CosSemCtl_u
{
    int val;
    struct semid_ds *buf;
    unsigned short *array;
}CosSemCtl_t;

typedef struct cosCompCfgInit_s
{
    ClUint32T cosTaskMinStackSize;
}cosCompCfgInit_t;

/**************************************************************************/
/* The global function pointer structure*/
extern osalFunction_t gOsalFunction;
/**************************************************************************/
#ifdef __cplusplus
}
#endif

#endif /* __INC_COS_H */
