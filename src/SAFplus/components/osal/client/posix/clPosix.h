#ifndef __CL_POSIX_H__
#define __CL_POSIX_H__

ClRcT cosPosixProcessSharedSemInit(ClOsalMutexT *pMutex, ClUint8T *pKey, ClUint32T keyLen, ClInt32T value);

ClRcT cosPosixMutexValueSet(ClOsalMutexIdT mutexId, ClInt32T value);
ClRcT cosPosixMutexValueGet(ClOsalMutexIdT mutexId, ClInt32T *pValue);

ClRcT cosPosixMutexLock (ClOsalMutexIdT mutexId);
ClRcT __cosPosixMutexLock (ClOsalMutexIdT mutexId, ClBoolT verbose);

ClRcT cosPosixMutexTryLock (ClOsalMutexIdT mutexId);
ClRcT cosPosixMutexUnlock (ClOsalMutexIdT mutexId);
ClRcT __cosPosixMutexUnlock (ClOsalMutexIdT mutexId, ClBoolT verbose);
ClRcT cosPosixMutexDestroy (ClOsalMutexT *pMutex);

#ifdef CL_OSAL_DEBUG
ClRcT cosPosixMutexLockDebug (ClOsalMutexIdT mutexId, const ClCharT *file, ClInt32T line);
ClRcT cosPosixMutexUnlockDebug (ClOsalMutexIdT mutexId, const ClCharT *file, ClInt32T line);
ClRcT cosPosixMutexDestroyDebug (ClOsalMutexT *pMutex, const ClCharT *file, ClInt32T line);
#endif

ClRcT cosPosixSemCreate (ClUint8T* pName, ClUint32T count, ClOsalSemIdT* pSemId);
ClRcT cosPosixSemIdGet(ClUint8T* pName, ClOsalSemIdT* pSemId);
ClRcT cosPosixSemLock (ClOsalSemIdT semId);
ClRcT cosPosixSemTryLock (ClOsalSemIdT semId);
ClRcT cosPosixSemUnlock (ClOsalSemIdT semId);
ClRcT cosPosixSemDelete(ClOsalSemIdT semId);
ClRcT cosPosixSemValueGet(ClOsalSemIdT semId, ClUint32T* pSemValue);

ClRcT cosPosixShmCreate(ClUint8T* pName, ClUint32T size, ClOsalShmIdT* pShmId);
ClRcT cosPosixShmIdGet(ClUint8T* pName, ClOsalShmIdT* pShmId);
ClRcT cosPosixShmDelete(ClOsalShmIdT shmId);
ClRcT cosPosixShmAttach(ClOsalShmIdT shmId,void* pInMem, void** ppOutMem);
ClRcT cosPosixShmDetach(void* pMem);
ClRcT cosPosixShmSecurityModeSet(ClOsalShmIdT shmId,ClUint32T mode);
ClRcT cosPosixShmSecurityModeGet(ClOsalShmIdT shmId,ClUint32T* pMode);
ClRcT cosPosixShmSizeGet(ClOsalShmIdT shmId,ClUint32T* pSize);


#endif
