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
#include <clLogSvrEvt.h>
#include <clLogServer.h>
#include <clLogSvrEo.h>

ClRcT
clLogSvrCompdownStateUpdate(ClUint32T  compId)
{
    ClRcT                  rc                 = CL_OK;
    ClLogSvrEoDataT        *pSvrEoEntry       = NULL;
    ClCntNodeHandleT       hSvrStreamNode     = CL_HANDLE_INVALID_VALUE;
    ClCntNodeHandleT       hNextNode          = CL_HANDLE_INVALID_VALUE;
    ClLogSvrCommonEoDataT  *pSvrCommonEoEntry = NULL;
    ClBoolT tableEmpty = CL_FALSE;

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
        tableEmpty = CL_FALSE;

        clCntNextNodeGet(pSvrEoEntry->hSvrStreamTable, hSvrStreamNode, &hNextNode);
        clLogSvrCompRefCountDecrement(pSvrEoEntry, hSvrStreamNode, compId, &tableEmpty);

        if(tableEmpty)
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
