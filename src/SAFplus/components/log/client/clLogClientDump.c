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
#include <string.h>

#include <clDispatchApi.h>
#include <clHandleApi.h>
#include <ipi/clHandleIpi.h>

#include <clLogClient.h>
#include <clLogClientHandle.h>
#include <clLogClientStream.h>
#include <clLogClientEo.h>
#include <clLogDump.h>

/* Dump functions for Client handle data structures */

ClRcT
clLogInitDataDump(ClHandleDatabaseHandleT  hDb,
                  ClLogHandleT             hLog,
                  ClPtrT                   pCookie)
{
    ClRcT                 rc            = CL_OK;
    ClUint32T             bitmapLen     = 0;
    ClUint32T             i             = 0;
    ClLogClntEoDataT      *pClntEoEntry = NULL;
    ClLogInitHandleDataT  *pData        = NULL;
    ClCharT               ret[MAX_DEBUG_DISPLAY];
    ClCharT               p[MAX_DEBUG_DISPLAY];
    ClCharT               **ppRet       = (ClCharT**) pCookie;
    
    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogClntEoEntryGet(&pClntEoEntry);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogClntEoEntryGet(): rc[0x %x]", rc));
        return rc;
    }

    rc = clHandleCheckout(pClntEoEntry->hClntHandleDB, hLog, (void **) (&pData));
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clHandleCheckout(): rc[0x %x]", rc));
        return rc;
    }

    memset(ret, '\0', MAX_DEBUG_DISPLAY);
    memset(p, '\0', MAX_DEBUG_DISPLAY);
    
    if( CL_LOG_INIT_HANDLE == pData->type )
    {
        snprintf(ret, MAX_DEBUG_DISPLAY, "Init handle: %#llX\nCallbacks: %p\nBitmap:%p\n",
                hLog, (void *) pData->pCallbacks,
                (void *) pData->hStreamBitmap);

        bitmapLen = clBitmapLen(pData->hStreamBitmap);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clBitmapLen(): rc[0x %x]", rc));
            return rc;
        }

        snprintf(p, MAX_DEBUG_DISPLAY, "hStreamBitmap:\n");
        strncat(ret, p, CL_MIN(strlen(p), (MAX_DEBUG_DISPLAY-strlen(ret)-1)));
        
        for( i = 0; i < bitmapLen; i++ )
        {
            if( clBitmapIsBitSet(pData->hStreamBitmap, i, &rc) )
            {
                snprintf(p, MAX_DEBUG_DISPLAY, "%d ", 1);
                strncat(ret, p, CL_MIN(strlen(p), (MAX_DEBUG_DISPLAY-strlen(ret)-1)));
            }
            else
            {
                snprintf(p, MAX_DEBUG_DISPLAY, "%d ", 0);
                strncat(ret, p, CL_MIN(strlen(p), (MAX_DEBUG_DISPLAY-strlen(ret)-1)));
            }
        }

        bitmapLen = clBitmapLen(pData->hFileBitmap);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clBitmapLen(): rc[0x %x]", rc));
            return rc;
        }

        snprintf(p, MAX_DEBUG_DISPLAY, "\nhFileBitmap:\n");
        strncat(ret, p, CL_MIN(strlen(p), (MAX_DEBUG_DISPLAY-strlen(ret)-1)));

        for( i = 0; i < bitmapLen; i++ )
        {
            if( clBitmapIsBitSet(pData->hFileBitmap, i, &rc) )
            {
                snprintf(p, MAX_DEBUG_DISPLAY, "%d ", 1);
                strncat(ret, p, CL_MIN(strlen(p), (MAX_DEBUG_DISPLAY-strlen(ret)-1)));
            }
            else
            {
                snprintf(p, MAX_DEBUG_DISPLAY, "%d ", 0);
                strncat(ret, p, CL_MIN(strlen(p), (MAX_DEBUG_DISPLAY-strlen(ret)-1)));
            }
        }
    }

    rc = clHandleCheckin(pClntEoEntry->hClntHandleDB, hLog);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clHandleCheckin(): rc[0x %x]", rc));
        return rc;
    }

    *ppRet = (ClCharT*) clHeapAllocate(strlen(ret) + 1);
    if( NULL == *ppRet )
    {
        CL_LOG_DEBUG_ERROR(("clHeapAllocate(): rc[0x %x]", rc));
        return rc;
    }

    snprintf(*ppRet, strlen(ret)+1, ret);
    
    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogInitHandleDataDisplay(ClCharT  **ppRet)
{
    ClRcT             rc            = CL_OK;
    ClLogClntEoDataT  *pClntEoEntry = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogClntEoEntryGet(&pClntEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = clHandleWalk(pClntEoEntry->hClntHandleDB, clLogInitDataDump, (void *)ppRet);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clHandleWalk(): rc[0x %x]", rc));
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogStreamDataDump(ClHandleDatabaseHandleT  hDb,
                    ClLogStreamHandleT       hStream,
                    ClPtrT                   pCookie)
{
    ClRcT                   rc            = CL_OK;
    ClLogStreamHandleDataT  *pData        = NULL;
    ClLogClntStreamDataT    *pStreamInfo  = NULL;
    ClLogClntEoDataT        *pClntEoEntry = NULL;
    ClCharT                 **ppRet       = (ClCharT**) pCookie;
    ClCharT                 ret[MAX_DEBUG_DISPLAY];
    ClCharT                 p[MAX_DEBUG_DISPLAY];


    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogClntEoEntryGet(&pClntEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = clHandleCheckout(hDb, hStream, (void **) (&pData));
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clHandleCheckout(): rc[0x %x]", rc));
        return rc;
    }

    memset(ret, '\0', MAX_DEBUG_DISPLAY);
    memset(p, '\0', MAX_DEBUG_DISPLAY);
    
    if( CL_LOG_STREAM_HANDLE == pData->type )
    {
        snprintf(ret, MAX_DEBUG_DISPLAY, "Stream handle: %#llX\nStreamNode: %p\n",
                hStream, (ClPtrT) pData->hClntStreamNode);

        
        rc = clCntNodeUserDataGet(pClntEoEntry->hClntStreamTable,
                                  pData->hClntStreamNode,
                                  (ClCntDataHandleT *) &pStreamInfo);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clCntNodeUserDataGet(): rc[0x %x]", rc));
            return rc;
        }

        snprintf(p,MAX_DEBUG_DISPLAY, "Stream Header: %p\n",
                (ClCharT *) pStreamInfo->pStreamHeader);
        strncat(ret, p, CL_MIN(strlen(p), (MAX_DEBUG_DISPLAY-strlen(ret)-1)));

        snprintf(p,MAX_DEBUG_DISPLAY, "Shm Name: %.*s", pStreamInfo->shmName.length,
                pStreamInfo->shmName.pValue);
        strncat(ret, p, CL_MIN(strlen(p), (MAX_DEBUG_DISPLAY-strlen(ret)-1)));
        
        rc = clHandleCheckin(hDb, hStream);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clHandleCheckin(): rc[0x %x]", rc));
            return rc;
        }

        *ppRet = (ClCharT*) clHeapAllocate(strlen(ret) + 1);
        if( NULL == *ppRet )
        {
            CL_LOG_DEBUG_ERROR(("clHeapAllocate(): rc[0x %x]", rc));
            return rc;
        }

        snprintf(*ppRet, strlen(ret)+1, ret);
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogStreamHandleDataDisplay(ClCharT  **ppRet)
{
    ClRcT             rc            = CL_OK;
    ClLogClntEoDataT  *pClntEoEntry = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogClntEoEntryGet(&pClntEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }
    
    rc = clHandleWalk(pClntEoEntry->hClntHandleDB, clLogStreamDataDump, (void *)ppRet);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clHandleWalk(): rc[0x %x]", rc));
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

#if 0
ClRcT
clLogHandlerDataDump(ClHandleDatabaseHandleT hDb, ClStreamHandleT hHandler,
                    void *pCookie)
{
    ClRcT                   rc            = CL_OK;
    ClLogHandlerHandleDataT *pData        = NULL;
    ClLogClntHandlerNodeT   *pHandlerInfo = NULL;
    ClLogClntEoDataT        *pClntEoEntry = NULL;
    ClUint32T               bitmapLen     = 0;
    ClCharT                 s[256];

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogClntEoEntryGet(&pClntEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = clHandleCheckout(hDb, hHandler, (void **) (&pData));
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clHandleCheckout(): rc[0x %x]", rc));
        return rc;
    }

    if( CL_LOG_HANDLER_HANDLE == pData->type )
    {
        CL_LOG_DEBUG_TRACE(("Handler handle: %d\nInit handle: %d"
                            " Handler Flag%d\n" , hStream,
                            pData->hLog, pData->handlerFlags));


        rc = clCntNodeUserDataGet(pClntEoEntry->hStreamHandlerTable,
                                  pData->hClntHandlerNode,
                                  (ClCntDataHandleT *) &pHandlerInfo);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clCntNodeUserDataGet(): rc[0x %x]", rc));
            return rc;
        }

        sprintf(s, pHandlerInfo->streamName.length, "%s",
                pHandlerInfo->streamName.value);
        CL_LOG_DEBUG_TRACE(("Stream Name: %s\n", s));

        sprintf(s, pHandlerInfo->nodeName.length, "%s",
                pHandlerInfo->nodeName.value);
        CL_LOG_DEBUG_TRACE(("Node Name: %s\n", s));

        bitmapLen = clBitmapLen(pStreamInfo->hStreamBitmap);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clBitmapLen(): rc[0x %x]", rc));
            return rc;
        }

        CL_LOG_DEBUG_TRACE(("hStreamBitmap:\n"));

        for( i = 0; i < bitmapLen; i++ )
        {
            if( clBitmapIsBitSet(pData->hStreamBitmap, i, &rc) )
            {
                CL_LOG_DEBUG_TRACE(("%d ", 1));
            }
            else
            {
                CL_LOG_DEBUG_TRACE(("%d ", 0));
            }
        }

    }

    rc = clHandleCheckin(hDb, hStream);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clHandleCheckin(): rc[0x %x]", rc));
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogHandlerHandleDataDisplay()
{
    ClRcT             rc            = CL_OK;
    ClLogClntEoDataT  *pClntEoEntry = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogClntEoEntryGet(&pClntEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = clHandleWalk(pClntEoEntry->hClntHandleDB, clLogHandlerDataDump, NULL);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clHandleWalk(): rc[0x %x]", rc));
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}
#endif
