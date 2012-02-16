#ifndef __CL_COMMON_COS_H__
#define __CL_COMMON_COS_H__


ClRcT cosPosixFileOpen(ClCharT *file, ClInt32T flags, ClHandleT *fd);
ClRcT cosPosixFileClose(ClFdT *fd);
ClRcT cosPosixMmap(ClPtrT start, ClUint32T length, ClInt32T prot, ClInt32T flags, ClHandleT fd, ClHandleT offset, ClPtrT *mmapped);
ClRcT cosPosixMunmap(ClPtrT start, ClUint32T length);
ClRcT cosPosixTaskCreateDetached (
        const ClCharT* taskName,ClOsalSchedulePolicyT schedulePolicy,ClUint32T priority, 
        ClUint32T stackSize, void* (*fpTaskFunction)(void*),
        void* pTaskFuncArgument);
ClRcT cosPosixTaskCreateAttached (
        const ClCharT* taskName,ClOsalSchedulePolicyT schedulePolicy,ClUint32T priority, 
        ClUint32T stackSize, void* (*fpTaskFunction)(void*),
        void* pTaskFuncArgument,ClOsalTaskIdT* pTaskId);
ClRcT cosPosixNanoTimeGet(ClNanoTimeT *pTime);
ClRcT cosPosixProcessSharedFutexInit(ClOsalMutexT *pMutex);
ClRcT cosPosixMsync(ClPtrT start, ClUint32T length, ClInt32T flags);
ClRcT cosPosixShmOpen(const ClCharT *name, ClInt32T oflag, ClUint32T mode, ClFdT *fd);
ClRcT cosPosixShmUnlink(const ClCharT *name);
ClRcT cosPosixFtruncate(ClFdT fd, off_t length);
ClRcT cosPosixTaskCreateDetached (const ClCharT* pTaskName, ClOsalSchedulePolicyT schedulePolicy, ClUint32T priority,
                    ClUint32T stackSize, void* (*fpTaskFunction)(void*),
                    void* pTaskFuncArgument);
ClRcT cosPosixTaskCreateAttached(const ClCharT* pTaskName, ClOsalSchedulePolicyT schedulePolicy, ClUint32T priority, 
                    ClUint32T stackSize, void* (*fpTaskFunction)(void*),
                    void* pTaskFuncArgument,ClOsalTaskIdT* pTaskId);
ClRcT cosPosixTaskDelete (ClOsalTaskIdT taskId);
ClRcT cosPosixTaskKill(ClOsalTaskIdT taskId, ClInt32T sig);
ClRcT cosPosixTaskJoin (ClOsalTaskIdT taskId);
ClRcT cosPosixTaskDetach (ClOsalTaskIdT taskId);
ClRcT cosPosixSelfTaskIdGet (ClOsalTaskIdT* pTaskId);
ClRcT cosPosixTaskNameGet (ClOsalTaskIdT taskId, ClUint8T** ppTaskName);
ClRcT cosPosixTaskPriorityGet (ClOsalTaskIdT taskId,ClUint32T* pTaskPriority);
ClRcT cosPosixTaskPrioritySet (ClOsalTaskIdT taskId,ClUint32T taskPriority);
ClRcT cosPosixTaskDelay (ClTimerTimeOutT timer);
ClTimeT cosPosixStopWatchTimeGet(void);
ClRcT cosPosixTimeOfDayGet(ClTimerTimeOutT* pTime);

ClRcT cosPosixNanoTimeGet(ClNanoTimeT *pTime);
ClRcT cosPosixMutexInit (ClOsalMutexT* pMutex);
ClRcT cosPosixMutexErrorCheckInit (ClOsalMutexT* pMutex);

ClRcT cosPosixProcessSharedFutexInit(ClOsalMutexT *pMutex);
ClRcT cosPosixRecursiveMutexInit (ClOsalMutexT* pMutex);
ClRcT cosPosixMutexCreate (ClOsalMutexIdT* pMutexId);
ClRcT cosPosixMutexErrorCheckCreate (ClOsalMutexIdT* pMutexId);

#ifdef CL_OSAL_DEBUG
ClRcT cosPosixMutexInitDebug (ClOsalMutexT* pMutex, const ClCharT *file, ClInt32T line);
ClRcT cosPosixRecursiveMutexInitDebug (ClOsalMutexT* pMutex, const ClCharT *file, ClInt32T line);
ClRcT cosPosixMutexCreateDebug (ClOsalMutexIdT* pMutexId, const ClCharT *file, ClInt32T line);
#endif

ClRcT cosPosixMutexAttrInit (ClOsalMutexAttrT* pAttr);
ClRcT cosPosixMutexAttrDestroy (ClOsalMutexAttrT* pAttr);
ClRcT cosPosixMutexAttrPSharedSet (ClOsalMutexAttrT* pAttr, ClOsalSharedTypeT type);
ClRcT cosPosixCondInit(ClOsalCondT *pCond);
ClRcT cosPosixCondInitEx (ClOsalCondT* pCond, ClOsalCondAttrT *pAttr);
ClRcT cosPosixProcessSharedCondInit (ClOsalCondT* ptr);
ClRcT cosPosixCondAttrInit (ClOsalCondAttrT* pAttr);
ClRcT cosPosixCondAttrDestroy (ClOsalCondAttrT* pAttr);
ClRcT cosPosixCondAttrPSharedSet (ClOsalCondAttrT* pAttr, ClOsalSharedTypeT type);
ClRcT cosPosixCondCreate(ClOsalCondIdT *pConditionId);
ClRcT cosPosixCondDestroy(ClOsalCondT *pCond);
ClRcT cosPosixCondDelete(ClOsalCondIdT conditionId);
ClRcT cosPosixCondWait (ClOsalCondIdT conditionId, ClOsalMutexIdT mutexId, ClTimerTimeOutT timer);
#ifdef CL_OSAL_DEBUG
ClRcT cosPosixCondWaitDebug (ClOsalCondIdT conditionId, ClOsalMutexIdT mutexId, ClTimerTimeOutT timer,
                             const ClCharT *file, ClInt32T line);
#endif
ClRcT cosPosixCondBroadcast (ClOsalCondIdT conditionId);
ClRcT cosPosixCondSignal (ClOsalCondIdT conditionId);
ClRcT cosPosixTaskDataSet(ClUint32T key, ClOsalTaskDataT threadData);
ClRcT cosPosixTaskDataGet(ClUint32T key, ClOsalTaskDataT* pThreadData);
ClRcT cosPosixTaskKeyCreate(ClUint32T* pKey,  void(*ClOsalTaskKeyDeleteCallBackT)(void*));
ClRcT cosPosixTaskKeyDelete(ClUint32T key);
ClRcT cosPosixProcessDelete(ClOsalPidT processId);
ClRcT cosPosixProcessWait(ClOsalPidT processId);
ClRcT cosPosixProcessSelfIdGet(ClOsalPidT* pProcessId);
ClRcT cosPosixMaxPathGet(const ClCharT* path, ClInt32T* pLength);
ClRcT cosPosixPageSizeGet(ClInt32T* pSize);

#endif
