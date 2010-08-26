#ifndef __CL_SYSV_H__
#define __CL_SYSV_H__

ClRcT cosSysvProcessSharedSemInit(ClOsalMutexT *pMutex, ClUint8T *pKey, ClUint32T keyLen, ClInt32T value);

ClRcT cosSysvMutexValueSet(ClOsalMutexIdT mutexId, ClInt32T value);
ClRcT cosSysvMutexValueGet(ClOsalMutexIdT mutexId, ClInt32T *pValue);
ClRcT cosSysvMutexLock (ClOsalMutexIdT mutexId);
ClRcT cosSysvMutexUnlock (ClOsalMutexIdT mutexId);
ClRcT cosSysvMutexDestroy (ClOsalMutexT *pMutex);

ClRcT cosSysvSemCreate (ClUint8T* pName, ClUint32T count, ClOsalSemIdT* pSemId);
ClRcT cosSysvSemIdGet(ClUint8T* pName, ClOsalSemIdT* pSemId);
ClRcT cosSysvSemLock (ClOsalSemIdT semId);
ClRcT cosSysvSemTryLock (ClOsalSemIdT semId);
ClRcT cosSysvSemUnlock (ClOsalSemIdT semId);
ClRcT cosSysvSemDelete(ClOsalSemIdT semId);
ClRcT cosSysvSemValueGet(ClOsalSemIdT semId, ClUint32T* pSemValue);

ClRcT cosSysvShmCreate(ClUint8T* pName, ClUint32T size, ClOsalShmIdT* pShmId);
ClRcT cosSysvShmIdGet(ClUint8T* pName, ClOsalShmIdT* pShmId);
ClRcT cosSysvShmDelete(ClOsalShmIdT shmId);
ClRcT cosSysvShmAttach(ClOsalShmIdT shmId,void* pInMem, void** ppOutMem);
ClRcT cosSysvShmDetach(void* pMem);
ClRcT cosSysvShmSecurityModeSet(ClOsalShmIdT shmId,ClUint32T mode);
ClRcT cosSysvShmSecurityModeGet(ClOsalShmIdT shmId,ClUint32T* pMode);
ClRcT cosSysvShmSizeGet(ClOsalShmIdT shmId,ClUint32T* pSize);


#endif
