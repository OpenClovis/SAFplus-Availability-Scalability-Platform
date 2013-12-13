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
#include <stdlib.h>
#include <string.h>
#include <clCpmApi.h>
#include <clCpmExtApi.h>
#include <clEoApi.h>
#include <clTaskPool.h>
#include <clEventApi.h>
#include <clLogSvrCommon.h>
#include <clLogServer.h>
#include <clLogFileOwner.h>
#include <clLogSvrEvt.h>
#include <clLogStreamOwner.h>
#include <clLogFileEvt.h>
#include <xdrClLogStreamInfoIDLT.h>

typedef struct ClLogFileEvent
{
    ClStringT *fileName;
    ClStringT *fileLocation;
    ClUint32T flags;
} ClLogFileEventT;

static ClTaskPoolHandleT gClLogFileEventTaskPool;
static ClBoolT gClLogEventInitialize;

static void
clLogEventChanOpenCb(ClInvocationT          invocation,
                     ClEventChannelHandleT  ClEventChannelHandle,
                     ClRcT                  error)

{
    return;
}

static void
clLogEventDeliverCb(ClEventSubscriptionIdT  subscriptionId,
                    ClEventHandleT          eventHandle,
                    ClSizeT                 eventDataSize);

static ClVersionT   gLogEvtVersion            = {'B', 0x1, 0x1 };

static ClUint32T    gStreamCreatedPattern     = 0x1;
static ClUint32T    gStreamClosedPattern      = 0x2;
static ClUint32T    gFileCreationPattern      = 0x3;
static ClUint32T    gFileClosedPattern        = 0x4;
static ClUint32T    gFileUnitFullPattern      = 0x5;
static ClUint32T    gFileHWMarkCrossedPattern = 0x6;
static ClUint32T    gCompAddPattern           = 0x7;

static ClEventPatternT gLogStreamCreatedEventPattern[] = {
    {0, sizeof(gStreamCreatedPattern), (ClUint8T *)&gStreamCreatedPattern}
};

static ClEventPatternArrayT gLogStreamCreatedPattArray =
{
    0,
    sizeof(gLogStreamCreatedEventPattern)/sizeof(ClEventPatternT),
    gLogStreamCreatedEventPattern
};
    
static ClEventPatternT gLogStreamClosedEventPattern[] = {
    {0, sizeof(gStreamClosedPattern), (ClUint8T *)&gStreamClosedPattern}
};

static ClEventPatternArrayT gLogStreamClosedPattArray =
{
    0,
    sizeof(gLogStreamClosedEventPattern)/sizeof(ClEventPatternT),
    gLogStreamClosedEventPattern
};

static ClEventPatternT gLogFileCreationEventPattern[] = {
    {0, sizeof(gFileCreationPattern), (ClUint8T *)&gFileCreationPattern}
};

static ClEventPatternArrayT gLogFileCreationPattArray =
{
    0,
    sizeof(gLogFileCreationEventPattern)/sizeof(ClEventPatternT),
    gLogFileCreationEventPattern
};

static ClEventPatternT gLogFileClosedEventPattern[] = {
    {0, sizeof(gFileClosedPattern), (ClUint8T *)&gFileClosedPattern}
};

static ClEventPatternArrayT gLogFileClosedPattArray =
{
    0,
    sizeof(gLogFileClosedEventPattern)/sizeof(ClEventPatternT),
    gLogFileClosedEventPattern

};

static ClEventPatternT gLogFileFullEventPattern[] = {
    {0, sizeof(gFileUnitFullPattern), (ClUint8T *)&gFileUnitFullPattern}
};

static ClEventPatternArrayT gLogFileFullPattArray =
{
    0,
    sizeof(gLogFileFullEventPattern)/sizeof(ClEventPatternT),
    gLogFileFullEventPattern
};

static ClEventPatternT gLogFileHWMarkCrossedEvtPattern[] = {
    {0, sizeof(gFileHWMarkCrossedPattern),
     (ClUint8T *)&gFileHWMarkCrossedPattern}
};

static ClEventPatternArrayT gLogFileHWMarkCrossedPattArray =
{
    0,
    sizeof(gLogFileHWMarkCrossedEvtPattern)/sizeof(ClEventPatternT),
    gLogFileHWMarkCrossedEvtPattern
};

static ClEventPatternT gLogCompAddEvtPattern[] = {
    {0, sizeof(gCompAddPattern),
     (ClUint8T *) &gCompAddPattern}
};

static ClEventPatternArrayT gLogCompAddPattArray =
{
    0,
    sizeof(gLogCompAddEvtPattern)/sizeof(ClEventPatternT),
    gLogCompAddEvtPattern
};

static ClEventCallbacksT gclLogEventCallbacks = { clLogEventChanOpenCb,
                                                  clLogEventDeliverCb };

static __inline__ void logEventPatternSwap(void)
{
    static ClBoolT logEventPatternSwapped = CL_FALSE;
    if(!logEventPatternSwapped)
    {
        gStreamCreatedPattern = htonl(gStreamCreatedPattern);
        gStreamClosedPattern = htonl(gStreamClosedPattern);
        gFileCreationPattern = htonl(gFileCreationPattern);
        gFileClosedPattern = htonl(gFileClosedPattern);
        gFileUnitFullPattern = htonl(gFileUnitFullPattern);
        gFileHWMarkCrossedPattern = htonl(gFileHWMarkCrossedPattern);
        gCompAddPattern = htonl(gCompAddPattern);
        logEventPatternSwapped = CL_TRUE;
    }
}

/*
 * Function will set the appropriate pattern for the particular
 * Event   
 */
static ClRcT
clLogEventPatternSet(ClLogSvrCommonEoDataT  *pSvrCommonEoEntry,
                     ClUint32T              patternType)
{

	ClRcT                 rc             = CL_OK;
    ClEventPatternArrayT  *pPatternArray = NULL;
    SaNameT               publisherName  = {10, "LOG_SERVER"};
    ClEventHandleT        eventHandle    = 0;
    /*
     * Set the attributes of the event based on the patternType passed.
     * Assign the pattern array accordingly and also set the eventHandle
     * accordingly.
     */
    switch( patternType )
    {
    case CL_LOG_FILE_CREATED:
        pPatternArray = &gLogFileCreationPattArray;
        eventHandle   = pSvrCommonEoEntry->hEvtFileCreated;
        break;
        
    case CL_LOG_FILE_CLOSED:
        pPatternArray = &gLogFileClosedPattArray;
        eventHandle   = pSvrCommonEoEntry->hEvtFileClosed;
        break;
        
    case CL_LOG_FILE_UNIT_FULL:
        pPatternArray = &gLogFileFullPattArray;
        eventHandle   = pSvrCommonEoEntry->hEvtFileUnitFull;
        break;
        
    case CL_LOG_FILE_HIGH_WATERMARK_CROSSED:
        pPatternArray = &gLogFileHWMarkCrossedPattArray;
        eventHandle   = pSvrCommonEoEntry->hEvtFileHWMarkCrossed;
        break;

    case CL_LOG_STREAM_CREATED:
        pPatternArray = &gLogStreamCreatedPattArray;
        eventHandle   = pSvrCommonEoEntry->hStreamCreationEvt;
        break;

    case CL_LOG_STREAM_CLOSED:
        pPatternArray = &gLogStreamClosedPattArray;
        eventHandle   = pSvrCommonEoEntry->hStreamCloseEvt;
        break;
    case CL_LOG_COMP_ADD:
        pPatternArray = &gLogCompAddPattArray;
        eventHandle   = pSvrCommonEoEntry->hCompAddEvt;
        break;
    }
    
    rc = clEventAttributesSet(eventHandle,
                              pPatternArray,
                              0,
                              0,
                              &publisherName);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clEventAttributesSet(): rc[0x%x]", rc));
        return rc;
    }

    return rc;
}


ClRcT
clLogEventInitialize(ClLogSvrCommonEoDataT  *pSvrCommonEoEntry)
{
    ClRcT   rc                = CL_OK;
    SaNameT logEvtChannelName = { 20, "CL_LOG_EVENT_CHANNEL"};

    /* Initialize the event library */
    rc = clEventInitialize(&pSvrCommonEoEntry->hEventInitHandle,
                           &gclLogEventCallbacks,
                           &gLogEvtVersion);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clEventInitialize(): rc[0x%x]", rc));
        return rc;
    }
    
    /* Open the channel where the file creation etc events will
     * be published
     */
    rc = clEventChannelOpen( pSvrCommonEoEntry->hEventInitHandle,
                             &logEvtChannelName,
                             CL_EVENT_CHANNEL_PUBLISHER |
                             CL_EVENT_CHANNEL_SUBSCRIBER|
                             CL_EVENT_GLOBAL_CHANNEL,
                             0,
                             &pSvrCommonEoEntry->hLogEvtChannel);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clEventChannelOpen(): rc[0x%x]", rc));
        clEventFinalize(pSvrCommonEoEntry->hEventInitHandle);
        return rc;
    }

    logEventPatternSwap();

    /*
     * Create the event task pool.
     */
    if(!gClLogFileEventTaskPool)
    {
        rc = clTaskPoolCreate(&gClLogFileEventTaskPool, 1, NULL, NULL);
        if(rc != CL_OK)
        {
            CL_LOG_DEBUG_ERROR(("Log event task pool create returned with [%#x]", rc));
            goto CLEANUP1;
        }
    }

    /* Allocate the diff events */
    
    rc = clEventAllocate(pSvrCommonEoEntry->hLogEvtChannel,
                         &pSvrCommonEoEntry->hEvtFileCreated);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clEventAllocate(): rc[0x%x]", rc));
        goto CLEANUP1;
    }
    
    rc = clLogEventPatternSet(pSvrCommonEoEntry, CL_LOG_FILE_CREATED);
    if( CL_OK != rc )
    {
        goto CLEANUP2;
    }
    
    rc = clEventAllocate( pSvrCommonEoEntry->hLogEvtChannel,
                          &pSvrCommonEoEntry->hEvtFileClosed);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clEventAllocate(): rc[0x%x]", rc));
        goto CLEANUP2;
    }
    
    /* Set the attributes of the event with the appropriate patterns */
    rc = clLogEventPatternSet(pSvrCommonEoEntry, CL_LOG_FILE_CLOSED);
    if( CL_OK != rc )
    {
        goto CLEANUP3;
    }

    rc = clEventAllocate(pSvrCommonEoEntry->hLogEvtChannel,
                         &pSvrCommonEoEntry->hEvtFileUnitFull);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clEventAllocate(): rc[0x%x]", rc));
        goto CLEANUP3;
    }
    /* Set the attributes of the event with the appropriate patterns */
    rc = clLogEventPatternSet(pSvrCommonEoEntry, CL_LOG_FILE_UNIT_FULL);
    if( CL_OK != rc )
    {
        goto CLEANUP4;
    }

    rc = clEventAllocate(pSvrCommonEoEntry->hLogEvtChannel,
                         &pSvrCommonEoEntry->hEvtFileHWMarkCrossed);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clEventAllocate(): rc[0x%x]", rc));
        goto CLEANUP4;
    }
    /* Set the attributes of the event with the appropriate patterns */
    rc = clLogEventPatternSet(pSvrCommonEoEntry,
                              CL_LOG_FILE_HIGH_WATERMARK_CROSSED);
    if( CL_OK != rc )
    {
        goto CLEANUP5;
    }

    rc = clEventAllocate(pSvrCommonEoEntry->hLogEvtChannel,
                         &pSvrCommonEoEntry->hStreamCreationEvt);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clEventAllocate(): rc[0x%x]", rc));
        goto CLEANUP5;
    }
    /* Set the attributes of the event with the appropriate patterns */
    rc = clLogEventPatternSet(pSvrCommonEoEntry, CL_LOG_STREAM_CREATED);
    if( CL_OK != rc )
    {
        goto CLEANUP6;
    }

    rc = clEventAllocate(pSvrCommonEoEntry->hLogEvtChannel,
                         &pSvrCommonEoEntry->hStreamCloseEvt);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clEventAllocate(): rc[0x%x]", rc));
        goto CLEANUP6;
    }
    
    /* Set the attributes of the event with the appropriate patterns */
    rc = clLogEventPatternSet(pSvrCommonEoEntry, CL_LOG_STREAM_CLOSED);
    if( CL_OK != rc )
    {
        goto CLEANUP7;
    }

    rc = clEventAllocate(pSvrCommonEoEntry->hLogEvtChannel,
                         &pSvrCommonEoEntry->hCompAddEvt);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clEventAllocate(): rc[0x%x]", rc));
        goto CLEANUP7;
    }

    rc = clLogEventPatternSet(pSvrCommonEoEntry, CL_LOG_COMP_ADD);
    if( CL_OK != rc )
    {
        goto CLEANUP8;
    }
    rc = clLogEvtSubscribe(pSvrCommonEoEntry); 
    if( CL_OK != rc )
    {
        goto CLEANUP8;
    }

    gClLogEventInitialize = CL_TRUE;

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
    
    CLEANUP8:
    clEventFree(pSvrCommonEoEntry->hCompAddEvt);
    CLEANUP7:
    clEventFree(pSvrCommonEoEntry->hStreamCloseEvt);
    CLEANUP6:
    clEventFree(pSvrCommonEoEntry->hStreamCreationEvt);
    CLEANUP5:
    clEventFree(pSvrCommonEoEntry->hEvtFileHWMarkCrossed);
    CLEANUP4:
    clEventFree(pSvrCommonEoEntry->hEvtFileUnitFull);
    CLEANUP3:
    clEventFree(pSvrCommonEoEntry->hEvtFileClosed);
    CLEANUP2:
    clEventFree(pSvrCommonEoEntry->hEvtFileCreated);
    CLEANUP1:
    clEventChannelClose(pSvrCommonEoEntry->hLogEvtChannel);
    clEventFinalize(pSvrCommonEoEntry->hEventInitHandle);

    CL_LOG_DEBUG_TRACE(("Exit: rc[0x %x]", rc));
    return rc;
}

ClRcT
clLogEvtFinalize(ClBoolT  terminatePath)
{
    ClRcT                  rc                 = CL_OK;
    ClLogSvrCommonEoDataT  *pSvrCommonEoEntry = NULL;
    
    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogSvrEoEntryGet(NULL, &pSvrCommonEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }

    if( pSvrCommonEoEntry->hCompAddEvt != CL_HANDLE_INVALID_VALUE )
    {
        CL_LOG_CLEANUP(clEventFree(pSvrCommonEoEntry->hCompAddEvt), CL_OK);
        pSvrCommonEoEntry->hCompAddEvt      = CL_HANDLE_INVALID_VALUE;
    }
    
    if( pSvrCommonEoEntry->hStreamCloseEvt != CL_HANDLE_INVALID_VALUE )
    {
        CL_LOG_CLEANUP(clEventFree(pSvrCommonEoEntry->hStreamCloseEvt), CL_OK);
        pSvrCommonEoEntry->hStreamCloseEvt  = CL_HANDLE_INVALID_VALUE;
    }
    
    if( pSvrCommonEoEntry->hStreamCreationEvt != CL_HANDLE_INVALID_VALUE )
    {
        CL_LOG_CLEANUP(clEventFree(pSvrCommonEoEntry->hStreamCreationEvt), CL_OK);
        pSvrCommonEoEntry->hStreamCreationEvt = CL_HANDLE_INVALID_VALUE;
    }
    
    if( pSvrCommonEoEntry->hEvtFileHWMarkCrossed != CL_HANDLE_INVALID_VALUE )
    {
        CL_LOG_CLEANUP(clEventFree(pSvrCommonEoEntry->hEvtFileHWMarkCrossed),
                               CL_OK);
        pSvrCommonEoEntry->hEvtFileHWMarkCrossed = CL_HANDLE_INVALID_VALUE;
    }
    
    if( pSvrCommonEoEntry->hEvtFileUnitFull != CL_HANDLE_INVALID_VALUE )
    {
        CL_LOG_CLEANUP(clEventFree(pSvrCommonEoEntry->hEvtFileUnitFull), CL_OK);
        pSvrCommonEoEntry->hEvtFileUnitFull = CL_HANDLE_INVALID_VALUE;
    }
    
    if( pSvrCommonEoEntry->hEvtFileClosed != CL_HANDLE_INVALID_VALUE )
    {
        CL_LOG_CLEANUP(clEventFree(pSvrCommonEoEntry->hEvtFileClosed), CL_OK);
        pSvrCommonEoEntry->hEvtFileClosed   = CL_HANDLE_INVALID_VALUE;
    }
    
    if( pSvrCommonEoEntry->hEvtFileCreated != CL_HANDLE_INVALID_VALUE )
    {
        CL_LOG_CLEANUP(clEventFree(pSvrCommonEoEntry->hEvtFileCreated), CL_OK);
        pSvrCommonEoEntry->hEvtFileCreated  = CL_HANDLE_INVALID_VALUE;
    }

    if( CL_FALSE == terminatePath )
    {
        CL_LOG_CLEANUP(clEventChannelClose(pSvrCommonEoEntry->hLogEvtChannel),
                CL_OK);
        CL_LOG_CLEANUP(clEventFinalize(pSvrCommonEoEntry->hEventInitHandle),
                CL_OK);
    }
    else
    {
        CL_LOG_DEBUG_TRACE(("Event sever is not there to finalize my data"));
    }
    /*FIXME - dependancy problem, leaks will be there */
    pSvrCommonEoEntry->hLogEvtChannel   = CL_HANDLE_INVALID_VALUE;
    pSvrCommonEoEntry->hEventInitHandle = CL_HANDLE_INVALID_VALUE;

    if(gClLogFileEventTaskPool)
        clTaskPoolStopAsync(gClLogFileEventTaskPool);

    gClLogEventInitialize = CL_FALSE;

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

static ClRcT logFileEventSet(ClLogFileEventT **ppFileEvent, ClStringT *fileName, ClStringT *fileLocation,
                             ClUint32T flags)
{
    ClLogFileEventT *pFileEvent = NULL;
    if(!ppFileEvent || !fileName || !fileLocation)
        return CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
    pFileEvent = (ClLogFileEventT*) clHeapCalloc(1, sizeof(*pFileEvent));
    CL_ASSERT(pFileEvent != NULL);
    pFileEvent->fileName = (ClStringT*) clHeapCalloc(1, sizeof(*pFileEvent->fileName));
    CL_ASSERT(pFileEvent->fileName != NULL);
    pFileEvent->fileLocation = (ClStringT*) clHeapCalloc(1, sizeof(*pFileEvent->fileLocation));
    CL_ASSERT(pFileEvent->fileLocation != NULL);
    pFileEvent->fileName->pValue = (ClCharT*) clHeapCalloc(1, fileName->length+1);
    CL_ASSERT(pFileEvent->fileName->pValue != NULL);
    pFileEvent->fileLocation->pValue = (ClCharT*) clHeapCalloc(1, fileLocation->length+1);
    CL_ASSERT(pFileEvent->fileLocation->pValue != NULL);
    pFileEvent->fileName->length = fileName->length;
    pFileEvent->fileLocation->length = fileLocation->length;
    memcpy(pFileEvent->fileName->pValue, fileName->pValue, fileName->length);
    memcpy(pFileEvent->fileLocation->pValue, fileLocation->pValue, fileLocation->length);
    pFileEvent->flags = flags;
    *ppFileEvent = pFileEvent;
    return CL_OK;
}

static ClRcT logFileEventTask(ClPtrT arg)
{
    ClLogFileEventT *pFileEvent = (ClLogFileEventT*) arg;
    ClRcT rc = CL_OK;
    if(!pFileEvent || !pFileEvent->fileName || !pFileEvent->fileLocation ||
       !pFileEvent->fileName->pValue || !pFileEvent->fileLocation->pValue) 
        return CL_LOG_RC(CL_ERR_INVALID_PARAMETER);
    rc = clLogFilePublishEvent(pFileEvent->fileName, pFileEvent->fileLocation,
                               pFileEvent->flags);
    clHeapFree(pFileEvent->fileName->pValue);
    clHeapFree(pFileEvent->fileLocation->pValue);
    clHeapFree(pFileEvent->fileName);
    clHeapFree(pFileEvent->fileLocation);
    clHeapFree(pFileEvent);
    return rc;
}

/* File Creation Event:
 *
 * Announces the creation of a log file. Also identifies the location of the
 * LogStream. File Handlers may register themselves for handlling the Log
 * records of all the log streams being persisted in this Log file.
 *
 * Channel Name: CL_LOG_EVENT_CHANNEL
 *
 * Pattern: CL_LOG_FILE_CREATED, a constant #defined to 0x3.
 *
 * Payload: 1. ClCharT *FileName, a NULL terminated string, identifying the
 *             prefix for the Log FileUnit Names.
 *
 *          2. ClCharT *fileLocation, a NULL terminated string, identifying
 *             the node name and absolute path name on that string for all
 *             the log file units.
 *       
 */

ClRcT
clLogFileCreationEvent(ClStringT * fileName,
                       ClStringT * fileLocation)
{
    ClRcT rc = CL_OK;
    ClLogFileEventT *pFileEvent = NULL;
    if(!gClLogEventInitialize || !gClLogFileEventTaskPool) return rc;
    rc = logFileEventSet(&pFileEvent, fileName, fileLocation, (ClUint32T)CL_LOG_FILE_CREATED);
    if(rc != CL_OK)
        return rc;
    return clTaskPoolRun(gClLogFileEventTaskPool, logFileEventTask, (ClPtrT)pFileEvent);
}    

/* File Closure Event
 *
 * This announces the closure of a Log File. Also it marks the deletion of the
 * last Log stream being persisted in this Log File. This notification alerts
 * the administrator and other applications in the cluster that the log file
 * is ready for final archiving.
 * File handlers may deregister themselves for handling the Log Records of all
 * the Log Streams being persisted in this file
 *
 * Channel Name: CL_LOG_EVENT_CHANNEL
 *
 * Pattern: CL_LOG_FILE_CLOSED, a constant #defined to 0x4.
 *
 * Payload: 1. ClCharT *fileName, a NULL terminated string, identifying the
 *             prefix for the log file unit names.
 *             
 *          2. ClCharT * fileLocation, a NULL terminated string, identifying
 *             the node name and absolute path name on that string for all
 *             the Log file units.
 *
 */

ClRcT
clLogFileClosureEvent(ClStringT  *fileName,
                      ClStringT  *fileLocation)
{
    ClRcT rc = CL_OK;
    ClLogFileEventT *pFileEvent = NULL;
    if(!gClLogEventInitialize || !gClLogFileEventTaskPool) return rc;
    rc = logFileEventSet(&pFileEvent, fileName, fileLocation, (ClUint32T)CL_LOG_FILE_CLOSED);
    if(rc != CL_OK)
        return rc;
    return clTaskPoolRun(gClLogFileEventTaskPool, logFileEventTask, (ClPtrT)pFileEvent);
}    

/* Watermark Event
 *
 * This event is generated when one of the Log File maintained by Log Service
 * has reached the high watermark configured for th logStreams being persisted
 * in that log file.
 *
 * Channel Name: CL_LOG_EVENT_CHANNEL
 *
 * Pattern: CL_LOG_FILE_HIGH_WATERMARK_CROSSED, #defined to 0x5.
 *
 * Payload: 1. ClCharT * fileName, a NULL terminated string identifying the
 *             prefix for the Log File Unit name.
 *
 *          2. ClCharT * fileLocation, a NULL terminated string identifying the
 *             node name and the absolute path name on that string for all the
 *             Log File Units.
 */

ClRcT
clLogHighWatermarkEvent(ClStringT  *fileName,
                        ClStringT  *fileLocation)
{
    ClRcT rc = CL_OK;
    ClLogFileEventT *pFileEvent = NULL;
    if(!gClLogEventInitialize || !gClLogFileEventTaskPool) return rc;
    rc = logFileEventSet(&pFileEvent, fileName, fileLocation, (ClUint32T)CL_LOG_FILE_HIGH_WATERMARK_CROSSED);
    if(rc != CL_OK)
        return rc;
    return clTaskPoolRun(gClLogFileEventTaskPool, logFileEventTask, (ClPtrT)pFileEvent);
}    

/* FileUnitFull Event
 *
 * This event is generated when on fileUnit of the logical file is full,
 * fileUnitSize is specifid as part of stream attributes.
 *
 * Channel Name: CL_LOG_EVENT_CHANNEL.
 *
 * Pattern : CL_LOG_FILE_UNIT_FULL, #defined to 0x6.
 *
 * Payload: 1. ClCharT * fileName, a NULL terminated string identifying the
 *             prefix for the Log File Unit name.
 *
 *          2. ClCharT * fileLocation, a NULL terminated string identifying the
 *             node name and the absolute path name on that string for all the
 *             Log File Units.
 *             
 **/

ClRcT
clLogFileUnitFullEvent(ClStringT  *fileName,
                       ClStringT  *fileLocation)
{
    ClRcT rc = CL_OK;
    ClLogFileEventT *pFileEvent = NULL;
    if(!gClLogEventInitialize || !gClLogFileEventTaskPool) return rc;
    rc = logFileEventSet(&pFileEvent, fileName, fileLocation, (ClUint32T)CL_LOG_FILE_UNIT_FULL);
    if(rc != CL_OK)
        return rc;
    return clTaskPoolRun(gClLogFileEventTaskPool, logFileEventTask, (ClPtrT)pFileEvent);
}

ClRcT
clLogEvtDataPrepare(ClStringT  *fileName,
                    ClStringT  *fileLocation,
                    ClUint8T   **ppPayLoad,
                    ClUint32T  *pPayloadLen)
{
    ClRcT            rc         = CL_OK;
    ClBufferHandleT  buffHandle = 0;
    ClUint32T        length     = 0;
    
    /* Create a Buffer */
    rc = clBufferCreate(&buffHandle);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferCreate(): rc[0x%x]", rc)) ;
        return rc;
    }
    
    rc = clXdrMarshallClStringT(fileName, buffHandle, 0);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clXdrMarshallClStringT(): rc[0x%x]", rc));
        clBufferDelete(&buffHandle);
        return rc;
    }

    rc = clXdrMarshallClStringT(fileLocation, buffHandle, 0);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clXdrMarshallClStringT(): rc[0x%x]", rc));
        clBufferDelete(&buffHandle);
        return rc;
    }

    rc = clBufferLengthGet(buffHandle, &length);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferDelete(): rc[0x%x]", rc));
        clBufferDelete(&buffHandle);
        return rc;
    }

    *ppPayLoad = (ClUint8T*) clHeapCalloc(length, sizeof(ClCharT));
    if( NULL == *ppPayLoad )
    {
        CL_LOG_DEBUG_ERROR(("clBufferDelete(): rc[0x%x]", rc));
        clBufferDelete(&buffHandle);
        return rc;
    }
    
    rc = clBufferNBytesRead(buffHandle, *ppPayLoad, &length);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferDelete(): rc[0x%x]", rc));
        clHeapFree(*ppPayLoad);
        clBufferDelete(&buffHandle);
        return rc;
    }
    /* Populate length of the payload: Used in clEventPublish */
    *pPayloadLen = length;
    
    /* Delete the buffer */
    rc = clBufferDelete(&buffHandle);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferDelete(): rc[0x%x]", rc));
        clHeapFree(*ppPayLoad);
    }
    
    return rc;
}

ClRcT
clLogFilePublishEvent(ClStringT  *fileName,
                      ClStringT  *fileLocation,
                      ClUint32T  patternType)

{
    ClRcT                  rc              = CL_OK;
    ClEoExecutionObjT      *pEoObj         = NULL;
    ClLogSvrCommonEoDataT  *pCommonEoEntry = NULL;
    ClUint8T               *pPayLoad       = NULL;
    ClUint32T              payloadLen      = 0;
    ClEventIdT             eventId         = 0;
    ClEventHandleT         eventHandle     = 0;
    
    CL_LOG_DEBUG_TRACE(( "Enter"));

    rc = clEoMyEoObjectGet(&pEoObj); 
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(( "clEoMyEoObjectGet(): rc[0x %x]", rc));
        return rc;
    }    

    rc = clEoPrivateDataGet(pEoObj, CL_LOG_SERVER_COMMON_EO_ENTRY_KEY, 
                           (void **) &pCommonEoEntry); 
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(( "clEoPrivateDataGet(): rc[0x %x]", rc));
        return rc;
    }

    /* Publish the event: Multiplex the eventHandle based on the
     * PatternType parameter. Allocate these events while startup
     * and keep diff handles for diff Log specific events instead
     * of setting the event attributes every time.
     * */
    switch(patternType)
    {
    case CL_LOG_FILE_CREATED:
        eventHandle = pCommonEoEntry->hEvtFileCreated;
        break;
        
    case CL_LOG_FILE_CLOSED:
        eventHandle = pCommonEoEntry->hEvtFileClosed;
        break;
        
    case CL_LOG_FILE_UNIT_FULL:
        eventHandle = pCommonEoEntry->hEvtFileUnitFull;
        break;
        
    case CL_LOG_FILE_HIGH_WATERMARK_CROSSED:
        eventHandle = pCommonEoEntry->hEvtFileHWMarkCrossed;
        break;
        
    default:
        CL_LOG_DEBUG_ERROR(("Invalid Event Pattern: returning"));
        return rc;
    }
    if( CL_HANDLE_INVALID_VALUE == eventHandle )
    {
        CL_LOG_DEBUG_TRACE(("Event library has already been finalized "));
        return CL_OK;
    }
    
    rc = clLogEvtDataPrepare(fileName,
                             fileLocation,
                             &pPayLoad,
                             &payloadLen);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogEvtDataPrepare(): rc[0x%x]", rc));
        return rc;
    }    

    rc = clEventPublish(eventHandle,
                        (const void*) pPayLoad,
                        payloadLen, 
                        &eventId);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clEventPublish(): rc[0x%x]", rc));
        clHeapFree(pPayLoad);
        return rc;
    }
    /* Free the payLoad */
    clHeapFree(pPayLoad);
    
    CL_LOG_DEBUG_TRACE(( "Exit"));
    return rc;
}

/* Gets the ClLogStreamInfoT struct, marshalls it and puts the data to
 * ClUint8T *Payload and also returns the length of the data that is
 * pPayloadLen
 */
ClRcT
clLogStreamEvtDataPrepare(ClLogStreamInfoIDLT  *pLogStreamInfo,
                          ClUint8T             **ppPayLoad,
                          ClUint32T            *pPayloadLen)
{
    ClRcT            rc         = CL_OK;
    ClBufferHandleT  buffHandle = 0;
    ClUint32T        length     = 0;
    
    /* Create the buffer */
    rc = clBufferCreate(&buffHandle);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferCreate(): rc[0x%x]", rc));
        return rc;
    }
    
    /* Marshall the pLogStreamInfo struct */
    rc = VDECL_VER(clXdrMarshallClLogStreamInfoIDLT, 4, 0, 0)(pLogStreamInfo, buffHandle, 0);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("VDECL_VER(clXdrMarshallClLogStreamInfoIDLT, 4, 0, 0)(): rc[0x%x]", rc));
        clBufferDelete(&buffHandle);
        return rc;
    }

    rc = clBufferLengthGet(buffHandle, &length);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferLengthGet(): rc[0x%x]", rc));
        clBufferDelete(&buffHandle);
        return rc;
    }

    *ppPayLoad = (ClUint8T*) clHeapCalloc(length, sizeof(ClCharT));
    if( NULL == *ppPayLoad )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
        clBufferDelete(&buffHandle);
        return CL_ERR_NO_MEMORY;
    }
    
    *pPayloadLen = length;
    rc = clBufferNBytesRead(buffHandle, *ppPayLoad, &length);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferNBytesRead(): rc[0x%x]", rc));
        clHeapFree(*ppPayLoad);
        clBufferDelete(&buffHandle);
        return rc;
    }

    rc = clBufferDelete(&buffHandle);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferDelete():rc[0x%x]", rc));
        clHeapFree(*ppPayLoad);
        return rc;
    }
    
    return rc;
}

/* Gets the ClLogStreamInfoT struct, marshalls it and puts the data to
 * ClUint8T *Payload and also returns the length of the data that is
 * pPayloadLen
 */
ClRcT
clLogStreamDataPrepare(SaNameT            *pStreamName,
                       ClLogStreamScopeT  streamScope, 
                       SaNameT            *pStreamScopeNode,
                       ClUint8T           **ppPayLoad,
                       ClUint32T          *pPayloadLen)
{
    ClRcT            rc         = CL_OK;
    ClBufferHandleT  buffHandle = 0;
    ClUint32T        length     = 0;
    
    /* Create the buffer */
    rc = clBufferCreate(&buffHandle);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferCreate(): rc[0x%x]", rc));
        return rc;
    }
    
    /* Marshall the pLogStreamInfo struct */
    rc = clXdrMarshallSaNameT(pStreamName, buffHandle, 0);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clXdrMarshallSaNameT(): rc[0x %x]", rc));
        clBufferDelete(&buffHandle);
        return rc;
    }
    rc = clXdrMarshallClUint32T(&streamScope, buffHandle, 0);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clXdrMarshallClUint32T(): rc[0x %x]", rc));
        clBufferDelete(&buffHandle);
        return rc;
    }
    rc = clXdrMarshallSaNameT(pStreamScopeNode, buffHandle, 0);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clXdrMarshallSaNameT(): rc[0x %x]", rc));
        clBufferDelete(&buffHandle);
        return rc;
    }

    rc = clBufferLengthGet(buffHandle, &length);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferLengthGet(): rc[0x%x]", rc));
        clBufferDelete(&buffHandle);
        return rc;
    }

    *ppPayLoad = (ClUint8T*) clHeapCalloc(length, sizeof(ClCharT));
    if( NULL == *ppPayLoad )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
        clBufferDelete(&buffHandle);
        return CL_LOG_RC(CL_ERR_NO_MEMORY);
    }
    
    *pPayloadLen = length;
    rc = clBufferNBytesRead(buffHandle, *ppPayLoad, &length);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferNBytesRead(): rc[0x%x]", rc));
        clHeapFree(*ppPayLoad);
        clBufferDelete(&buffHandle);
        return rc;
    }

    rc = clBufferDelete(&buffHandle);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferDelete():rc[0x%x]", rc));
        clHeapFree(*ppPayLoad);
        return rc;
    }
    
    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogStreamCreationEvtPublish(ClLogStreamInfoIDLT  *pLogStreamInfo)
{
    ClRcT                  rc              = CL_OK;
    ClUint8T               *pPayLoad       =  NULL;
    ClUint32T              payloadLen      = 0;
    ClEoExecutionObjT      *pEoObj         = NULL;
    ClEventIdT             eventId         = 0;
    ClLogSvrCommonEoDataT  *pCommonEoEntry = NULL;
    
    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clEoMyEoObjectGet(&pEoObj); 
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(( "clEoMyEoObjectGet(): rc[0x %x]", rc));
        return rc;
    }    

    rc = clEoPrivateDataGet(pEoObj, CL_LOG_SERVER_COMMON_EO_ENTRY_KEY, 
                           (void **) &pCommonEoEntry); 
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(( "clEoPrivateDataGet(): rc[0x %x]", rc));
        return rc;
    }
    if( CL_HANDLE_INVALID_VALUE == pCommonEoEntry->hStreamCreationEvt )
    {
        CL_LOG_DEBUG_ERROR(("Event library has been already finalized"));
        return CL_OK;
    }

    rc = clLogStreamEvtDataPrepare(pLogStreamInfo, &pPayLoad, &payloadLen);
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = clEventPublish(pCommonEoEntry->hStreamCreationEvt, (void *) pPayLoad,
                        payloadLen, &eventId);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clEventPublish(): rc[0x%x]", rc));
        clHeapFree(pPayLoad);
        return rc;
    }
    clHeapFree(pPayLoad);

    clLogInfo(CL_LOG_AREA_STREAM_OWNER, CL_LOG_CTX_SO_INIT, 
              "Published event for stream [%.*s] creation", 
               pLogStreamInfo->streamName.length,
               pLogStreamInfo->streamName.value);
    
    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}    

ClRcT
clLogStreamCloseEvtPublish(SaNameT            *pStreamName,
                           ClLogStreamScopeT  streamScope,
                           SaNameT            *pStreamScopeNode)
{

    ClRcT                  rc              = CL_OK;
    ClUint8T               *pPayLoad       =  NULL;
    ClUint32T              payloadLen      = 0;
    ClEoExecutionObjT      *pEoObj         = NULL;
    ClEventIdT             eventId         = 0;
    ClLogSvrCommonEoDataT  *pCommonEoEntry = NULL;
    
    rc = clEoMyEoObjectGet(&pEoObj); 
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(( "clEoMyEoObjectGet(): rc[0x %x]", rc));
        return rc;
    }    

    rc = clEoPrivateDataGet(pEoObj, CL_LOG_SERVER_COMMON_EO_ENTRY_KEY, 
                           (void **) &pCommonEoEntry); 
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(( "clEoPrivateDataGet(): rc[0x %x]", rc));
        return rc;
    }
    if( CL_HANDLE_INVALID_VALUE == pCommonEoEntry->hStreamCloseEvt )
    {
        CL_LOG_DEBUG_TRACE(("Event library has been already finalized"));
        return CL_OK;
    }
 
    rc = clLogStreamDataPrepare(pStreamName, streamScope, pStreamScopeNode,
                                &pPayLoad, &payloadLen);
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = clEventPublish(pCommonEoEntry->hStreamCloseEvt, (void *) pPayLoad,
                        payloadLen, &eventId);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clEventPublish(): rc[0x%x]", rc));
        clHeapFree(pPayLoad);
        return rc;
    }
    clHeapFree(pPayLoad);
    
    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}    

ClRcT
clLogCompDataPrepare(SaNameT            *pCompName,
                     ClUint32T          clientId, 
                     ClUint8T           **ppPayLoad,
                     ClUint32T          *pPayloadLen)
{
    ClRcT            rc         = CL_OK;
    ClBufferHandleT  buffHandle = 0;
    ClUint32T        length     = 0;
    ClLogCompDataT   compData   = {{0}};
    
    /* Create the buffer */
    rc = clBufferCreate(&buffHandle);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferCreate(): rc[0x%x]", rc));
        return rc;
    }
    
    /* Marshall the pLogStreamInfo struct */
    compData.compName = *pCompName;
    compData.clientId = clientId;
    rc = VDECL_VER(clXdrMarshallClLogCompDataT, 4, 0, 0)(&compData, buffHandle, 0);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clXdrMarshallSaNameT(): rc[0x %x]", rc));
        clBufferDelete(&buffHandle);
        return rc;
    }

    rc = clBufferLengthGet(buffHandle, &length);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferLengthGet(): rc[0x%x]", rc));
        clBufferDelete(&buffHandle);
        return rc;
    }

    *ppPayLoad = (ClUint8T*) clHeapCalloc(length, sizeof(ClCharT));
    if( NULL == *ppPayLoad )
    {
        CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
        clBufferDelete(&buffHandle);
        return CL_LOG_RC(CL_ERR_NO_MEMORY);
    }
    
    *pPayloadLen = length;
    rc = clBufferNBytesRead(buffHandle, *ppPayLoad, &length);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferNBytesRead(): rc[0x%x]", rc));
        clHeapFree(*ppPayLoad);
        clBufferDelete(&buffHandle);
        return rc;
    }

    rc = clBufferDelete(&buffHandle);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferDelete():rc[0x%x]", rc));
        clHeapFree(*ppPayLoad);
        return rc;
    }
    
    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogCompAddEvtPublish(SaNameT    *pCompName,
                       ClUint32T  clientId)
{

    ClRcT                  rc              = CL_OK;
    ClUint8T               *pPayLoad       =  NULL;
    ClUint32T              payloadLen      = 0;
    ClEoExecutionObjT      *pEoObj         = NULL;
    ClEventIdT             eventId         = 0;
    ClLogSvrCommonEoDataT  *pCommonEoEntry = NULL;
    
    rc = clEoMyEoObjectGet(&pEoObj); 
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(( "clEoMyEoObjectGet(): rc[0x %x]", rc));
        return rc;
    }    

    rc = clEoPrivateDataGet(pEoObj, CL_LOG_SERVER_COMMON_EO_ENTRY_KEY, 
                           (void **) &pCommonEoEntry); 
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(( "clEoPrivateDataGet(): rc[0x %x]", rc));
        return rc;
    }
    if( CL_HANDLE_INVALID_VALUE == pCommonEoEntry->hCompAddEvt )
    {
        CL_LOG_DEBUG_ERROR(("Event library has been already finalized"));
        return CL_OK;
    }
 
    rc = clLogCompDataPrepare(pCompName, clientId, &pPayLoad, &payloadLen);
    if( CL_OK != rc )
    {
        return rc;
    }

    rc = clEventPublish(pCommonEoEntry->hCompAddEvt, (void *) pPayLoad, 
                        payloadLen, &eventId);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clEventPublish(): rc[0x%x]", rc));
        clHeapFree(pPayLoad);
        return rc;
    }
    clHeapFree(pPayLoad);
    
    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}    

ClRcT
clLogEvtSubscribe(ClLogSvrCommonEoDataT  *pSvrCommonEoEntry)
{
    ClRcT      rc       = CL_OK;
    ClHandleT  hCompChl = CL_HANDLE_INVALID_VALUE;
    ClHandleT  hNodeChl = CL_HANDLE_INVALID_VALUE;

    CL_LOG_DEBUG_TRACE(("Enter"));

    rc = clLogCompDownSubscribe(pSvrCommonEoEntry->hEventInitHandle,
                                &hCompChl);
    if (CL_OK != rc)
    {
        return rc;
    }

    rc = clLogNodeDownSubscribe(pSvrCommonEoEntry->hEventInitHandle, 
                                &hNodeChl);
    if (CL_OK != rc)
    {
        CL_LOG_CLEANUP(clEventChannelClose(hCompChl), CL_OK);
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogFileOwnerEventSubscribe(void)
{
    ClRcT                  rc                 = CL_OK;
    ClLogSvrCommonEoDataT  *pSvrCommonEoEntry = NULL; 
    ClEventFilterT         createFilter[]     = { {CL_EVENT_EXACT_FILTER,  
           {0, sizeof(gStreamCreatedPattern), 
            (ClUint8T *) &gStreamCreatedPattern}}
                                                };
    ClEventFilterArrayT    createFilterArray  = 
                               {
                                   sizeof(createFilter) / sizeof(ClEventFilterT), 
                                   createFilter 
                               };
    ClEventFilterT         closeFilter[]      = { {CL_EVENT_EXACT_FILTER,  
           {0, sizeof(gStreamClosedPattern), 
                     (ClUint8T *) &gStreamClosedPattern}}
                                                };
    ClEventFilterArrayT    closeFilterArray   = 
                               {
                                   sizeof(closeFilter) / sizeof(ClEventFilterT), 
                                   closeFilter 
                               };
    ClEventFilterT         compFilter[]      = { {CL_EVENT_EXACT_FILTER,  
           {0, sizeof(gCompAddPattern), (ClUint8T *) &gCompAddPattern}}
                                               };
    ClEventFilterArrayT    compFilterArray   = 
                               {
                                   sizeof(compFilter) / sizeof(ClEventFilterT), 
                                   compFilter
                               };

    rc = clLogSvrEoEntryGet(NULL, &pSvrCommonEoEntry);
    if( CL_OK != rc )
    {
        return rc;
    }

    logEventPatternSwap();

    rc = clEventSubscribe(pSvrCommonEoEntry->hLogEvtChannel,
                          &createFilterArray, CL_LOG_STREAMCREAT_EVT_SUBID,
                          NULL);
    if(CL_OK != rc)
    {
        CL_LOG_DEBUG_ERROR(("clEventSubscribe(): rc[0x %x]", rc));
        return rc;
    }

    rc = clEventSubscribe(pSvrCommonEoEntry->hLogEvtChannel, 
                          &closeFilterArray, CL_LOG_STREAMCLOSE_EVT_SUBID,
                          NULL);
    if(CL_OK != rc)
    {
        CL_LOG_DEBUG_ERROR(("clEventSubscribe(): rc[0x %x]", rc));
        return rc;
    }

    rc = clEventSubscribe(pSvrCommonEoEntry->hLogEvtChannel, 
                          &compFilterArray, CL_LOG_COMPADD_EVT_SUBID,
                          NULL);
    if(CL_OK != rc)
    {
        CL_LOG_DEBUG_ERROR(("clEventSubscribe(): rc[0x %x]", rc));
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogCompDownSubscribe(ClEventInitHandleT  hEvtSvcInit, 
                       ClHandleT           *phCompChl)
{
    ClRcT                     rc           = 0;
    SaNameT                   cpmChnlName  = {0};
    ClEventChannelOpenFlagsT  openFlags    = 0;
    ClUint32T                 deathPattern   = htonl(CL_CPM_COMP_DEATH_PATTERN);
    ClEventFilterT            compDeathFilter[]  = {{CL_EVENT_EXACT_FILTER, 
                                                {0, (ClSizeT)sizeof(deathPattern), (ClUint8T*)&deathPattern}}
    };
    ClEventFilterArrayT      compDeathFilterArray = {sizeof(compDeathFilter)/sizeof(compDeathFilter[0]), 
                                                     compDeathFilter
    };

    CL_LOG_DEBUG_TRACE(("Enter"));

    openFlags = CL_EVENT_CHANNEL_SUBSCRIBER| CL_EVENT_GLOBAL_CHANNEL;

    cpmChnlName.length = strlen(CL_CPM_EVENT_CHANNEL_NAME);
    memcpy(cpmChnlName.value, CL_CPM_EVENT_CHANNEL_NAME, cpmChnlName.length);
    rc = clEventChannelOpen(hEvtSvcInit, &cpmChnlName, openFlags, (ClTimeT)-1, 
                            phCompChl);
    if ((CL_OK != rc) && (rc != CL_EVENT_ERR_EXIST))
    {
        CL_LOG_DEBUG_ERROR(("clEventChannelOpen(): rc[0x %x]", rc));
        return rc;
    }

    rc = clEventSubscribe(*phCompChl, &compDeathFilterArray, CL_LOG_COMPDOWN_EVT_SUBID,
                          NULL);
    if (CL_OK != rc)
    {
        CL_LOG_DEBUG_ERROR(("clEventSubscribe(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clEventChannelClose(*phCompChl), CL_OK);
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogNodeDownSubscribe(ClEventInitHandleT hEvtSvcInit,
                       ClEventHandleT     *phNodeChl)
{
    ClRcT                     rc           = CL_OK;
    ClEventChannelOpenFlagsT  openFlags    = 0;
    SaNameT                   cpmChnlName  = {0};
    ClUint32T                 nodeDeparturePattern = htonl(CL_CPM_NODE_DEATH_PATTERN);
    ClEventFilterT            nodeDepartureFilter[]         = { {CL_EVENT_EXACT_FILTER,
                                                                {0, (ClSizeT)sizeof(nodeDeparturePattern),
                                                                (ClUint8T*)&nodeDeparturePattern}}
    };
    ClEventFilterArrayT       nodeDepartureFilterArray = {sizeof(nodeDepartureFilter)/sizeof(nodeDepartureFilter[0]),
                                                          nodeDepartureFilter 
    };

    CL_LOG_DEBUG_TRACE(("Enter"));

    openFlags = CL_EVENT_CHANNEL_SUBSCRIBER| CL_EVENT_GLOBAL_CHANNEL;

    cpmChnlName.length = strlen(CL_CPM_NODE_EVENT_CHANNEL_NAME);
    memcpy(cpmChnlName.value, CL_CPM_NODE_EVENT_CHANNEL_NAME, 
           cpmChnlName.length);

    rc = clEventChannelOpen(hEvtSvcInit, &cpmChnlName, openFlags, (ClTimeT)-1,
                            phNodeChl);
    if ((CL_OK != rc) && (rc != CL_EVENT_ERR_EXIST))
    {
        CL_LOG_DEBUG_ERROR(("clEventChannelOpen(): rc[0x %x]", rc));
        return rc;
    }

    rc = clEventSubscribe(*phNodeChl, &nodeDepartureFilterArray, CL_LOG_NODEDOWN_EVT_SUBID,
                          NULL);
    if(CL_OK != rc)
    {
        CL_LOG_DEBUG_ERROR(("clEventSubscribe(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clEventChannelClose(*phNodeChl), CL_OK);
        return rc;
    }

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

static void
clLogEventDeliverCb(ClEventSubscriptionIdT  subscriptionId,
                    ClEventHandleT          eventHandle,
                    ClSizeT                 eventDataSize)
{
    ClRcT                   rc          = CL_OK;
    ClCpmEventPayLoadT      payLoad     = {{0}};
    ClCpmEventNodePayLoadT  nodePayload = {{0}};
    ClUint8T               *pBuffer     = NULL;

    CL_LOG_DEBUG_TRACE(("Enter"));
    
    switch(subscriptionId)
    {
    case CL_LOG_COMPDOWN_EVT_SUBID:
        {
            rc = clCpmEventPayLoadExtract(eventHandle, eventDataSize,
                                          CL_CPM_COMP_EVENT, 
                                          (void *) &payLoad);
            if (CL_OK != rc)
            {
                CL_LOG_DEBUG_ERROR(("clCpmEventPayLoadExtract(): rc[0x %x]", rc));
                goto out_free;
            }
                
            if (payLoad.operation != CL_CPM_COMP_DEATH)
                goto out_free;

            rc =
                clLogStreamOwnerCompdownStateUpdate(payLoad.nodeIocAddress,
                                                    payLoad.eoIocPort);
            if( CL_OK != rc )
            {
                CL_LOG_DEBUG_ERROR(("clLogStreamOwnerNodedownStateUpdate(): rc[0x %x]", rc));
                goto out_free;
            }
            
            if(payLoad.nodeIocAddress == clIocLocalAddressGet())
            {
                rc = clLogSvrCompdownStateUpdate(payLoad.eoIocPort);
                if( CL_OK != rc )
                {
                    CL_LOG_DEBUG_ERROR(("clLogSvrCompdownStateUpdate(): rc[0x %x]", rc));
                    goto out_free;
                }
            }
   
            break;
        }
    case CL_LOG_NODEDOWN_EVT_SUBID:
        {
            rc = clCpmEventPayLoadExtract(eventHandle, eventDataSize,
                                          CL_CPM_NODE_EVENT, 
                                          (void *) &nodePayload);
            if (CL_OK != rc)
            {
                CL_LOG_DEBUG_ERROR(("clCpmEventPayLoadExtract(): rc[0x %x]", rc));
                goto out_free;
            }
            if (nodePayload.operation != CL_CPM_NODE_DEATH)
                goto out_free;

            rc =
                clLogStreamOwnerNodedownStateUpdate(
                                                    nodePayload.nodeIocAddress);
            if( CL_OK != rc )
            {
                goto out_free;
            }
            rc  = clLogNodeDownMasterDBUpdate(&nodePayload.nodeName);
            if( CL_OK != rc )
            {
                goto out_free;
            }
            break;
        }
    case CL_LOG_STREAMCREAT_EVT_SUBID:            
        {
            CL_LOG_DEBUG_TRACE(("Got an Event for stream Creation: %llu",
                                eventDataSize));
                
            pBuffer = (ClUint8T*) clHeapCalloc(eventDataSize, sizeof(ClUint8T));
            if( NULL == pBuffer )
            {
                CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
                goto out_free;
            }

            rc = clEventDataGet(eventHandle, (void *) pBuffer,
                                &eventDataSize);
            if (CL_OK != rc)
            {
                CL_LOG_DEBUG_ERROR(("clEventDataGet(): rc[0x %x]", rc));
                goto out_free;
            }
            rc = clLogStreamCreationEventDataExtract(pBuffer, eventDataSize);
            if( CL_OK != rc )
            {
                goto out_free;
            }
            break;
        }
    case CL_LOG_STREAMCLOSE_EVT_SUBID:            
        {
            pBuffer = (ClUint8T*)clHeapCalloc(eventDataSize, sizeof(ClUint8T));
            if( NULL == pBuffer )
            {
                CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
                goto out_free;
            }
            rc = clEventDataGet(eventHandle, (void *) pBuffer,
                                &eventDataSize);
            if (CL_OK != rc)
            {
                CL_LOG_DEBUG_ERROR(("clEventDataGet(): rc[0x %x]", rc));
                goto out_free;
            }
            rc = clLogStreamCloseEventDataExtract(pBuffer, eventDataSize);
            if( CL_OK != rc )
            {
                goto out_free;
            }
            break;
        }
    case CL_LOG_COMPADD_EVT_SUBID:            
        {
            pBuffer = (ClUint8T*) clHeapCalloc(eventDataSize, sizeof(ClUint8T));
            if( NULL == pBuffer )
            {
                CL_LOG_DEBUG_ERROR(("clHeapCalloc()"));
                goto out_free;
            }
            rc = clEventDataGet(eventHandle, (void *) pBuffer,
                                &eventDataSize);
            if (CL_OK != rc)
            {
                CL_LOG_DEBUG_ERROR(("clEventDataGet(): rc[0x %x]", rc));
                goto out_free;
            }
            rc = clLogCompAddEventDataExtract(pBuffer, eventDataSize);
            if( CL_OK != rc )
            {
                goto out_free;
            }
            break;
        }
    default:
        break;
    }

    out_free:
    clEventFree(eventHandle);
    if(pBuffer) clHeapFree(pBuffer);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return ;
}

ClRcT
clLogStreamCreationEventDataExtract(ClUint8T   *pBuffer,
                                    ClUint32T  size)
{
    ClRcT                rc         = CL_OK;
    ClLogStreamInfoIDLT  streamInfo = {{0}};
    ClBufferHandleT      msg        = CL_HANDLE_INVALID_VALUE;

    CL_LOG_DEBUG_TRACE(("Enter: size: %d", size));
    
    rc = clBufferCreate(&msg);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferCreate(): rc[0x %x]", rc));
        return rc;
    }
    rc = clBufferNBytesWrite(msg, pBuffer, size);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferNBytesWrite():rc[0x %x]", rc));
        CL_LOG_CLEANUP(clBufferDelete(&msg), CL_OK);
        return rc;
    }

    rc = VDECL_VER(clXdrUnmarshallClLogStreamInfoIDLT, 4, 0, 0)(msg, &streamInfo);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clBufferDelete(&msg), CL_OK);
        return rc;
    }
    if( NULL != streamInfo.streamAttr.fileName.pValue )
    {
        rc = clLogFileOwnerStreamCreateEvent(&streamInfo.streamName, 
                (ClLogStreamScopeT)streamInfo.streamScope,
                &streamInfo.nodeName,
                streamInfo.streamId,
                &streamInfo.streamAttr, 
                CL_TRUE); 
        if( CL_OK != rc )
        {
            CL_LOG_CLEANUP(clBufferDelete(&msg), CL_OK);
            clHeapFree(streamInfo.streamAttr.fileName.pValue);
            clHeapFree(streamInfo.streamAttr.fileLocation.pValue);
            return rc;
        }
        clHeapFree(streamInfo.streamAttr.fileName.pValue);
        clHeapFree(streamInfo.streamAttr.fileLocation.pValue);
    }
    CL_LOG_CLEANUP(clBufferDelete(&msg), CL_OK);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogStreamCloseEventDataExtract(ClUint8T   *pBuffer,
                                 ClUint32T  size)
{
    ClRcT              rc              = CL_OK;
    ClBufferHandleT    msg             = CL_HANDLE_INVALID_VALUE;
    SaNameT            streamName      = {0}; 
    ClLogStreamScopeT  streamScope     = CL_LOG_STREAM_GLOBAL; 
    SaNameT            streamScopeNode = {0}; 

    CL_LOG_DEBUG_TRACE(("Enter"));
    
    rc = clBufferCreate(&msg);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferCreate(): rc[0x %x]", rc));
        return rc;
    }
    rc = clBufferNBytesWrite(msg, pBuffer, size);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferNBytesWrite():rc[0x %x]", rc));
        CL_LOG_CLEANUP(clBufferDelete(&msg), CL_OK);
        return rc;
    }

    rc = clXdrUnmarshallSaNameT(msg, &streamName);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clXdrUnmarshallClLogNameT(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clBufferDelete(&msg), CL_OK);
        return rc;
    }
    rc = clXdrUnmarshallClUint32T(msg, &streamScope);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clXdrUnmarshallClUint32T(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clBufferDelete(&msg), CL_OK);
        return rc;
    }
    rc = clXdrUnmarshallSaNameT(msg, &streamScopeNode);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clXdrUnmarshallClLogNameT(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clBufferDelete(&msg), CL_OK);
        return rc;
    }

    rc = clLogFileOwnerStreamCloseEvent(&streamName, streamScope,
                                         &streamScopeNode);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clBufferDelete(&msg), CL_OK);
        return rc;
    }
    CL_LOG_CLEANUP(clBufferDelete(&msg), CL_OK);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clLogCompAddEventDataExtract(ClUint8T   *pBuffer,
                             ClUint32T  size)
{
    ClRcT            rc       = CL_OK;
    ClBufferHandleT  msg      = CL_HANDLE_INVALID_VALUE;
    ClLogCompDataT   compData = {{0}}; 

    CL_LOG_DEBUG_TRACE(("Enter"));
    
    rc = clBufferCreate(&msg);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferCreate(): rc[0x %x]", rc));
        return rc;
    }
    rc = clBufferNBytesWrite(msg, pBuffer, size);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clBufferNBytesWrite():rc[0x %x]", rc));
        CL_LOG_CLEANUP(clBufferDelete(&msg), CL_OK);
        return rc;
    }

    rc = VDECL_VER(clXdrUnmarshallClLogCompDataT, 4, 0, 0)(msg, &compData);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("VDECL_VER(clXdrUnmarshallClLogCompDataT, 4, 0, 0)(): rc[0x %x]", rc));
        CL_LOG_CLEANUP(clBufferDelete(&msg), CL_OK);
        return rc;
    }

    rc = clLogFileOwnerCompAddEvent(&compData);
    if( CL_OK != rc )
    {
        CL_LOG_CLEANUP(clBufferDelete(&msg), CL_OK);
        return rc;
    }
    CL_LOG_CLEANUP(clBufferDelete(&msg), CL_OK);

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}
