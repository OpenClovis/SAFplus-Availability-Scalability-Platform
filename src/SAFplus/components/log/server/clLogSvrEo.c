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
#include <clHandleApi.h>
#include <clEoApi.h>
#include <clLogSvrEo.h>

ClRcT
clLogSvrEoDataInit(ClLogSvrEoDataT        *pSvrEoEntry,
                   ClLogSvrCommonEoDataT  *pSvrCommonEoEntry)
{
    ClRcT               rc = CL_OK;

    CL_LOG_DEBUG_TRACE(("Enter"));

    pSvrEoEntry->hSvrStreamTable = CL_HANDLE_INVALID_VALUE;
    pSvrEoEntry->nextDsId        = CL_LOG_DSID_START;

    rc = clOsalMutexInit_L(&pSvrEoEntry->svrStreamTableLock);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMutexCreate(): rc[0x %x]", rc));
        return rc;
    }

    rc = clBitmapCreate(&pSvrEoEntry->hDsIdMap, pSvrCommonEoEntry->maxStreams);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBitmapCreate(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clOsalMutexDestroy_L(&pSvrEoEntry->svrStreamTableLock),
                       CL_OK);
        return rc;
    }
    rc = clHandleDatabaseCreate(NULL, &pSvrEoEntry->hFlusherDB); 
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clHandleDatabaseCreate(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clBitmapDestroy(pSvrEoEntry->hDsIdMap), CL_OK);
        CL_LOG_CLEANUP(clOsalMutexDestroy_L(&pSvrEoEntry->svrStreamTableLock),
                       CL_OK);
        return rc;
    }

    rc = clLogConfigDataGet(NULL, NULL, NULL, &pSvrEoEntry->maxFlushLimit);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogConfigDataGet(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clHandleDatabaseDestroy(pSvrEoEntry->hFlusherDB), CL_OK);
        CL_LOG_CLEANUP(clBitmapDestroy(pSvrEoEntry->hDsIdMap), CL_OK);
        CL_LOG_CLEANUP(clOsalMutexDestroy_L(&pSvrEoEntry->svrStreamTableLock),
                       CL_OK);
        return rc;
    }
    pSvrEoEntry->hCpm           = CL_HANDLE_INVALID_VALUE;    
    pSvrEoEntry->logCompId      = 0;
    pSvrEoEntry->gmsInit        = CL_FALSE; 
    pSvrEoEntry->evtInit        = CL_FALSE; 
    pSvrEoEntry->ckptInit       = CL_FALSE; 
    pSvrEoEntry->logInit        = CL_FALSE;
    pSvrEoEntry->hTimer         = CL_FALSE;

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogSvrEoDataFinalize(void)
{
    ClRcT            rc           = CL_OK;
    ClLogSvrEoDataT  *pSvrEoEntry = NULL;

	CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogSvrEoEntryGet(&pSvrEoEntry, NULL);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogSvrEoEntryGet(): rc[0x%x]", rc));
        return rc;
    }

    rc = clOsalMutexLock_L(&pSvrEoEntry->svrStreamTableLock);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMutexLock_L(): rc[0x %x]", rc));
        return rc;
    }

    if( CL_HANDLE_INVALID_VALUE != pSvrEoEntry->hSvrStreamTable )
    {
        CL_LOG_CLEANUP(clCntDelete(pSvrEoEntry->hSvrStreamTable), CL_OK);
        pSvrEoEntry->hSvrStreamTable = CL_HANDLE_INVALID_VALUE;
    }
    pSvrEoEntry->logInit = CL_FALSE;
    CL_LOG_CLEANUP(clBitmapDestroy(pSvrEoEntry->hDsIdMap), CL_OK);
    CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pSvrEoEntry->svrStreamTableLock),
            CL_OK);
    CL_LOG_CLEANUP(clOsalMutexDestroy_L(&pSvrEoEntry->svrStreamTableLock),
            CL_OK);

	CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogSvrEoEntryGet(ClLogSvrEoDataT        **ppSvrEoEntry,
                   ClLogSvrCommonEoDataT  **ppSvrCommonEoEntry)
{
    ClRcT              rc          = CL_OK;
    ClEoExecutionObjT  *pEoExecObj = NULL;

    CL_LOG_DEBUG_TRACE(("Enter: %p %p", (void *) ppSvrEoEntry, 
                       (void *) ppSvrCommonEoEntry));

    rc = clEoMyEoObjectGet(&pEoExecObj);
    if( CL_OK != rc)
    {
        CL_LOG_DEBUG_ERROR(("clEoMyEoObjectGet(): rc[0x %x]", rc));
        return rc;
    }

    if( NULL != ppSvrEoEntry )
    {
        rc = clEoPrivateDataGet(pEoExecObj, CL_LOG_SERVER_EO_ENTRY_KEY, 
                                (void **) ppSvrEoEntry);
        if( CL_OK != rc)
        {
            CL_LOG_DEBUG_ERROR(("clEoPrivateDataGet(): rc[0x %x]", rc));
            return rc;
        }
    }
    
    if( NULL != ppSvrCommonEoEntry )
    {
        rc = clEoPrivateDataGet(pEoExecObj, CL_LOG_SERVER_COMMON_EO_ENTRY_KEY, 
                                (void **) ppSvrCommonEoEntry);
        if( CL_OK != rc)
        {
            CL_LOG_DEBUG_ERROR(("clEoPrivateDataGet(): rc[0x %x]", rc));
            return rc;
        }
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}
