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
#include <clHandleApi.h>
#include <clEoApi.h>
#include <clBitmapApi.h>

#include <clLogSvrCommon.h>
#include <clLogStreamOwnerEo.h>
#include <clLogStreamOwner.h>
#include <xdrClLogCompKeyT.h>
#include <clLogOsal.h>

#if 0
static void
clLogStreamOwnerEoDataPrint(ClLogSOEoDataT  *pSoEoEntry);
#endif

ClRcT
clLogStreamOwnerLocalEoDataInit(ClLogSOEoDataT  **ppSoEoEntry)
{
    ClRcT              rc      = CL_OK;
    ClEoExecutionObjT  *pEoObj = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clEoMyEoObjectGet(&pEoObj); 
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clEoMyEoObjectGet(): rc[0x %x]", rc));
        return rc;
    }    

    *ppSoEoEntry = clHeapCalloc(1, sizeof(ClLogSOEoDataT));
    if( NULL == *ppSoEoEntry )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
        return CL_LOG_RC(CL_ERR_NO_MEMORY);
    }    

    rc = clOsalMutexInit_L(&(*ppSoEoEntry)->lStreamTableLock);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clOsalMutexCreate_L(): rc[0x %x]", rc));
        clHeapFree(*ppSoEoEntry);
        return rc;
    }

    rc = clBitmapCreate(&(*ppSoEoEntry)->hDsIdMap, 0);
    if( CL_OK != rc )
    {
       CL_LOG_DEBUG_ERROR(("clBitmapCreate(): rc[0x %x]", rc));
       CL_LOG_CLEANUP(clOsalMutexDestroy_L(&(*ppSoEoEntry)->lStreamTableLock),
                      CL_OK); 
       clHeapFree(*ppSoEoEntry);
       return rc;
    }    
    (*ppSoEoEntry)->hLStreamOwnerTable = CL_HANDLE_INVALID_VALUE;
    (*ppSoEoEntry)->hGStreamOwnerTable = CL_HANDLE_INVALID_VALUE;
    (*ppSoEoEntry)->dsIdCnt            = 1;
    (*ppSoEoEntry)->hCkpt              = CL_HANDLE_INVALID_VALUE;
    (*ppSoEoEntry)->status             = CL_TRUE;

    rc = clEoPrivateDataSet(pEoObj, CL_LOG_STREAM_OWNER_EO_ENTRY_KEY,
                            *ppSoEoEntry); 
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clEoPrivateDataSet(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clBitmapDestroy((*ppSoEoEntry)->hDsIdMap), CL_OK);
        CL_LOG_CLEANUP(clOsalMutexDestroy_L(&(*ppSoEoEntry)->lStreamTableLock),
                       CL_OK);
        clHeapFree(*ppSoEoEntry);
    }    

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogStreamOwnerLocalEoDataFinalize(ClLogSOEoDataT  *pSoEoEntry)
{
    ClRcT  rc = CL_OK;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clOsalMutexLock_L(&pSoEoEntry->lStreamTableLock);
    if( CL_OK != rc )
    {
        return rc;
    }
    pSoEoEntry->status = CL_FALSE;
    if( CL_HANDLE_INVALID_VALUE != pSoEoEntry->hLStreamOwnerTable )
    {
        CL_LOG_CLEANUP(clCntDelete(pSoEoEntry->hLStreamOwnerTable),
                CL_OK);
        pSoEoEntry->hLStreamOwnerTable = CL_HANDLE_INVALID_VALUE;
    }
    CL_LOG_CLEANUP(clBitmapDestroy(pSoEoEntry->hDsIdMap), CL_OK);
    CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pSoEoEntry->lStreamTableLock), CL_OK);
    CL_LOG_CLEANUP(clOsalMutexDestroy_L(&pSoEoEntry->lStreamTableLock),
            CL_OK);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

