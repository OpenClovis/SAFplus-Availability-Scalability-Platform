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
#include <clLogSvrCommon.h>
#include <clLogMasterEo.h>
#include <clLogMaster.h>


ClRcT
clLogMasterEoDataInit(ClLogMasterEoDataT  **ppMasterEoEntry)
{
    ClRcT                  rc              = CL_OK;
    ClLogSvrCommonEoDataT  *pCommonEoEntry = NULL;
    ClEoExecutionObjT      *pEoObj         = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogMasterEoEntryGet(NULL, &pCommonEoEntry);
    if( CL_OK != rc )
    {
      return rc;
    }

    rc = clEoMyEoObjectGet(&pEoObj);
    if( CL_OK != rc )
    {
       CL_LOG_DEBUG_ERROR(("clEoMyEoObjectGet(): rc[0x %x]", rc));
       return rc;
    }

    *ppMasterEoEntry = clHeapCalloc(1, sizeof(ClLogMasterEoDataT));
    if( NULL == *ppMasterEoEntry )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
        return CL_LOG_RC(CL_ERR_NO_MEMORY);
    }

    rc = clOsalMutexInit_L(&((*ppMasterEoEntry)->masterFileTableLock));
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMutexCreate(): rc[0x %x]", rc));
        clHeapFree(*ppMasterEoEntry);
        return rc;
    }

    rc = clOsalMutexInit_L(&((*ppMasterEoEntry)->masterCompTableLock));
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMutexCreate(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(
            clOsalMutexDestroy_L(&(*ppMasterEoEntry)->masterFileTableLock), CL_OK);
        clHeapFree(*ppMasterEoEntry);
        return rc;
    }
    
    (*ppMasterEoEntry)->hMasterFileTable = CL_HANDLE_INVALID_VALUE;
    (*ppMasterEoEntry)->hAllocedAddrMap  = CL_BM_INVALID_BITMAP_HANDLE;
    (*ppMasterEoEntry)->nextStreamId     = CL_LOG_MASTER_STREAMID_START;
    /* Get these values from the config file */
    (*ppMasterEoEntry)->hCompTable       = CL_HANDLE_INVALID_VALUE;
    (*ppMasterEoEntry)->nextCompId       = CL_LOG_MASTER_COMPID_START;
    (*ppMasterEoEntry)->numComps         = 0; 
    (*ppMasterEoEntry)->startMcastAddr   =
        CL_IOC_MULTICAST_ADDRESS_FORM(1, 1);
    (*ppMasterEoEntry)->numMcastAddr     = pCommonEoEntry->maxStreams;
    (*ppMasterEoEntry)->maxFiles         = CL_LOG_MASTER_MAX_FILES;
    (*ppMasterEoEntry)->sectionSize      =
        sizeof(ClVersionT) + sizeof(ClUint32T) + CL_LOG_MASTER_MAX_SECTIONID_SIZE + sizeof(ClLogStreamAttrIDLT)
        + (pCommonEoEntry->maxStreams *
           (sizeof(ClLogMasterStreamDataT) + sizeof(ClLogStreamKeyT)));
    (*ppMasterEoEntry)->sectionIdSize    = CL_LOG_MASTER_MAX_SECTIONID_SIZE;

    rc = clEoPrivateDataSet(pEoObj, CL_LOG_MASTER_EO_ENTRY_KEY,
                            *ppMasterEoEntry);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clEoPrivateDataSet(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(
            clOsalMutexDestroy_L(&(*ppMasterEoEntry)->masterCompTableLock), CL_OK);
        CL_LOG_CLEANUP(
            clOsalMutexDestroy_L(&(*ppMasterEoEntry)->masterFileTableLock), CL_OK);
        clHeapFree(*ppMasterEoEntry);
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogMasterEoDataFinalize(ClLogMasterEoDataT *pMasterEoEntry)
{
    ClRcT  rc = CL_OK;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clOsalMutexLock_L(&pMasterEoEntry->masterCompTableLock);
    if( CL_OK != rc )
    {
        return rc;
    }
    if( CL_HANDLE_INVALID_VALUE != pMasterEoEntry->hCompTable )
    {
        CL_LOG_CLEANUP(clCntDelete(pMasterEoEntry->hCompTable),
                       CL_OK);
        pMasterEoEntry->hCompTable = CL_HANDLE_INVALID_VALUE;
    }
    CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pMasterEoEntry->masterCompTableLock),
            CL_OK);

    rc = clOsalMutexLock_L(&pMasterEoEntry->masterFileTableLock);
    if( CL_OK != rc )
    {
        return rc;
    }

    if( CL_HANDLE_INVALID_VALUE != pMasterEoEntry->hMasterFileTable )
    {
        CL_LOG_CLEANUP(clCntDelete(pMasterEoEntry->hMasterFileTable),
                       CL_OK);
        pMasterEoEntry->hMasterFileTable = CL_HANDLE_INVALID_VALUE;
    }

    if( CL_BM_INVALID_BITMAP_HANDLE != pMasterEoEntry->hAllocedAddrMap )
    {
        CL_LOG_CLEANUP(clBitmapDestroy(pMasterEoEntry->hAllocedAddrMap),
                       CL_OK);
        pMasterEoEntry->hAllocedAddrMap = CL_BM_INVALID_BITMAP_HANDLE;
    }

    CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pMasterEoEntry->masterFileTableLock),
            CL_OK);

    /* clEoPrivateDataDelete() needs to be called when implemented by EO
     * CL_CLEAN_UP(clEoPrivateDataDelete(pEoObj, CL_LOG_MASTER_EO_ENTRY_KEY,
     *             *ppMasterEoEntry), CL_OK);
    */
    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogMasterEoEntryGet(ClLogMasterEoDataT     **ppMasterEoEntry,
                      ClLogSvrCommonEoDataT  **ppCommonEoEntry)
{
    ClEoExecutionObjT  *pEoObj = NULL;
    ClRcT              rc      = CL_OK;

    CL_LOG_DEBUG_TRACE(("Enter: Master: %p Common: %p",
                        (void *) ppMasterEoEntry, (void *) ppCommonEoEntry));

    rc = clEoMyEoObjectGet(&pEoObj);
    if( CL_OK != rc)
    {
        CL_LOG_DEBUG_ERROR(("clEoMyEoObjectGet(): rc[0x %x]", rc));
        return rc;
    }

    if( NULL != ppMasterEoEntry )
    {
        rc = clEoPrivateDataGet(pEoObj, CL_LOG_MASTER_EO_ENTRY_KEY,
                                (void **) ppMasterEoEntry);
        if( CL_OK != CL_GET_ERROR_CODE(rc) )
        {
            CL_LOG_DEBUG_ERROR(("clEoPrivateDataGet(): rc[0x %x]", rc));
            return rc;
        }
    }

    if( NULL != ppCommonEoEntry )
    {
        rc = clEoPrivateDataGet(pEoObj, CL_LOG_SERVER_COMMON_EO_ENTRY_KEY,
                                (void *) ppCommonEoEntry);
        if( CL_OK != CL_GET_ERROR_CODE(rc) )
        {
            CL_LOG_DEBUG_ERROR(("clEoPrivateDataGet(): rc[0x %x]", rc));
            return rc;
        }
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogMasterEoEntrySet(ClLogMasterEoDataT  *pMasterEoEntry)
{
    ClRcT  rc = CL_OK;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clBitmapCreate(&pMasterEoEntry->hAllocedAddrMap,
                        pMasterEoEntry->numMcastAddr);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBitmapCreate(): rc[0x%x]", rc));
        return rc;
    }

    rc = clCntHashtblCreate(pMasterEoEntry->maxFiles, clLogFileKeyCompare,
                            clLogFileKeyHashFn, clLogMasterFileEntryDeleteCb,
                            clLogMasterFileEntryDeleteCb, CL_CNT_UNIQUE_KEY,
                            &(pMasterEoEntry->hMasterFileTable));
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clCntHashTblCreate(): rc[0x%x]", rc));
        CL_LOG_CLEANUP(clBitmapDestroy(pMasterEoEntry->hAllocedAddrMap), CL_OK);
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}
