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
#include <clLogSvrEvt.h>
#include <clLogServer.h>
#include <clLogSvrEo.h>

ClRcT
clLogSvrCompdownStateUpdate(ClUint32T  compId)
{
    ClRcT                  rc                 = CL_OK;
    ClLogSvrEoDataT        *pSvrEoEntry       = NULL;
    ClUint32T              size               = 0;
    ClLogSvrStreamDataT    *pSvrStreamData    = NULL;
    ClCntNodeHandleT       hSvrStreamNode     = CL_HANDLE_INVALID_VALUE;
    ClCntNodeHandleT       hNextNode          = CL_HANDLE_INVALID_VALUE;
    ClLogSvrCommonEoDataT  *pSvrCommonEoEntry = NULL;
    ClLogSvrCompKeyT       compKey            = {0};

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogSvrEoEntryGet(&pSvrEoEntry, &pSvrCommonEoEntry);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogSvrEoEntryGet(): rc[0x %x]\n", rc));
        return rc;
    }
    rc = clOsalMutexLock_L(&pSvrEoEntry->svrStreamTableLock);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("CL_LOG_LOCK(): rc[0x %x]\n", rc));
        return rc;
    }
    compKey.componentId = compId; 
    compKey.hash        = compKey.componentId % pSvrCommonEoEntry->maxComponents;

    rc = clCntFirstNodeGet(pSvrEoEntry->hSvrStreamTable, &hSvrStreamNode);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_TRACE(("clCntFirstNodeGet(); rc[0x %x]", rc));
        CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pSvrEoEntry->svrStreamTableLock),
                    CL_OK);
        return rc;
    }
    while( CL_HANDLE_INVALID_VALUE != hSvrStreamNode )
    {
        hNextNode = CL_HANDLE_INVALID_VALUE;

        rc = clCntNodeUserDataGet(pSvrEoEntry->hSvrStreamTable, hSvrStreamNode,
                                    (ClCntDataHandleT *) &pSvrStreamData);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_TRACE(("clCntNodeUserDataGet(); rc[0x %x]", rc));
            CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pSvrEoEntry->svrStreamTableLock),
                        CL_OK);
            return rc;
        }
        clCntNextNodeGet(pSvrEoEntry->hSvrStreamTable, hSvrStreamNode, &hNextNode);

        clCntAllNodesForKeyDelete(pSvrStreamData->hComponentTable,
                                  (ClCntKeyHandleT) &compKey); 
        rc = clCntSizeGet(pSvrStreamData->hComponentTable, &size);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clCntSizeGet(): rc[0x %x]\n", rc));
            CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pSvrEoEntry->svrStreamTableLock),
                        CL_OK);
            return rc;
        }
        if( 0 == size )
        {
            rc = clCntNodeDelete(pSvrEoEntry->hSvrStreamTable, hSvrStreamNode);
            if( CL_OK != rc )
            {
                CL_LOG_DEBUG_ERROR(("clCntNodeDelete(): rc[0x %x]\n", rc));
                CL_LOG_CLEANUP(clOsalMutexUnlock_L(&pSvrEoEntry->svrStreamTableLock),
                            CL_OK);
                return rc;
                
            }
        }
        hSvrStreamNode = hNextNode;
    }

    rc = clOsalMutexUnlock_L(&pSvrEoEntry->svrStreamTableLock);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("CL_LOG_UNLOCK(): rc[0x %x]\n", rc));
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}
