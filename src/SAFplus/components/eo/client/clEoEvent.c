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
/*******************************************************************************
 * ModuleName  : eo
File        : clEoEvent.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *          This file provides for Event Publish when a Water Mark is hit.
 *
 *
 ****************************************************************************/

/* 
 * Includes 
 */
#include <clEoEvent.h>
#include <clDebugApi.h>

/* 
 * Defines 
 */
#define isEventInitDone() (gEoEventInfo.isInitDone == CL_TRUE)

/* 
 * TypeDefs 
 */

typedef struct ClEoEventInfo
{
    ClBoolT isInitDone;
    ClEventInitHandleT initHandle;
    ClEventChannelHandleT channelHandle;
    ClEventHandleT eventHandle;
} ClEoEventInfoT; 

static ClEoEventInfoT gEoEventInfo;

#ifdef CL_EO_TBD 
typedef struct ClEoEventData {
    ClCharT eoName[CL_EO_MAX_NAME_LEN],
            ClEoLibIdT libId,
            ClUint8T wmId,
            ClEoWaterMarkFlagT wmflag,
} ClEoEventDataT;

ClEoEventDataT gEoEventData;
#endif

#ifdef CL_EO_DEBUG
static void eoEventOnWaterMarkHit( ClEventSubscriptionIdT subscriptionId,
        ClEventHandleT eventHandle, ClSizeT eventDataSize )
{
    ClRcT rc = CL_OK;

    ClEventPriorityT priority = 0;
    ClTimeT retentionTime = 0;
    ClNameT publisherName = { 0 };
    ClEventIdT eventId = 0;
    ClEventPatternArrayT patternArray = { 0 };
    ClTimeT publishTime = 0;
    void *pCookie = NULL;

#ifdef CL_EO_TBD 
    ClUint32T payLoad[2] = { 0 };
    ClSizeT payLoadLen = 20;
    ClUint8T i = 0;             /* We know the no. of patterns is 4 */

    rc = clEventDataGet(eventHandle, (void *) &payLoad, &payLoadLen);
#endif

    rc = clEventAttributesGet(eventHandle, &patternArray, &priority,
            &retentionTime, &publisherName, &publishTime,
            &eventId);

    rc = clEventCookieGet(eventHandle, &pCookie);

    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("-------------------------------------------------------\n"));
    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("!!!!!!!!!!!!!!! Event Delivery Callback !!!!!!!!!!!!!!!\n"));
    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("-------------------------------------------------------\n"));
    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("             Subscription ID   : 0x%X\n", subscriptionId));
    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("             Event Priority    : 0x%X\n", priority));
    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("             Retention Time    : 0x%llX\n",
                retentionTime));
    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("             Publisher Name    : %.*s\n",
                publisherName.length, publisherName.value));
#ifdef CL_EO_TBD 
    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("             EventID           : 0x%X\n", eventId));
    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("             Payload[0]        : 0x%X\n", payLoad[0]));
    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("             Payload[1]        : 0x%X\n", payLoad[1]));
#endif

    /*
     * Free the allocated Event 
     */
    rc = clEventFree(eventHandle);

    /*
     * Display the Water Mark Details and Free the patterns.
     */

    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("             EO Name           : %.*s\n",
                (ClInt32T)patternArray.pPatterns[0].patternSize, 
                (ClCharT *)patternArray.pPatterns[0].pPattern));
    clHeapFree(patternArray.pPatterns[0].pPattern);

    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("             Library ID        : 0x%x\n",
                *(ClEoLibIdT *)patternArray.pPatterns[1].pPattern));
    clHeapFree(patternArray.pPatterns[1].pPattern);

    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("             Water Mark ID     : 0x%x\n",
                *(ClWaterMarkIdT *)patternArray.pPatterns[2].pPattern));
    clHeapFree(patternArray.pPatterns[2].pPattern);

    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("             Water Mark Type   : %s\n",
                (*(ClEoWaterMarkFlagT *)patternArray.pPatterns[3].pPattern == CL_WM_HIGH_LIMIT)? "HIGH" : "LOW"));
    clHeapFree(patternArray.pPatterns[3].pPattern);

    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("             Water Mark Value  : %u\n",
                *(ClUint32T *)patternArray.pPatterns[4].pPattern));
    clHeapFree(patternArray.pPatterns[4].pPattern);

    clHeapFree(patternArray.pPatterns);

    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("-------------------------------------------------------\n"));

    return;
}
#endif

static ClRcT eoEventInit(void) 
{
    ClRcT rc = CL_OK;

    ClVersionT version = CL_EVENT_VERSION;    
    ClNameT channelName = {sizeof(CL_EO_EVENT_CHANNEL_NAME)-1, CL_EO_EVENT_CHANNEL_NAME};

#ifdef CL_EO_DEBUG
    const ClEventCallbacksT evtCallbacks = 
    {
        NULL,
        eoEventOnWaterMarkHit,  
    };
    ClEventChannelOpenFlagsT evtFlags = CL_EVENT_CHANNEL_SUBSCRIBER | CL_EVENT_CHANNEL_PUBLISHER | CL_EVENT_GLOBAL_CHANNEL;
#else
    const ClEventCallbacksT evtCallbacks = {NULL};
    ClEventChannelOpenFlagsT evtFlags = CL_EVENT_CHANNEL_PUBLISHER | CL_EVENT_GLOBAL_CHANNEL;
#endif

    rc = clEventInitialize(&gEoEventInfo.initHandle, &evtCallbacks, &version);
    if(CL_OK != rc)
    {
        goto failure;
    }

    rc = clEventChannelOpen(gEoEventInfo.initHandle, &channelName, 
            evtFlags, (ClTimeT)-1, 
            &gEoEventInfo.channelHandle);
    if(CL_OK != rc)
    {
        goto init_done;
    }

#ifdef CL_EO_DEBUG
    rc = clEventSubscribe(gEoEventInfo.channelHandle, NULL, 0, NULL);
    if(CL_OK != rc)
    {
        goto channel_opened;
    }
#endif

    rc = clEventAllocate(gEoEventInfo.channelHandle, &gEoEventInfo.eventHandle);
    if(CL_OK != rc)
    {
        goto channel_opened;
    }

    gEoEventInfo.isInitDone = CL_TRUE;

    return CL_OK;

channel_opened:
    clEventChannelClose(gEoEventInfo.channelHandle);

init_done:
    clEventFinalize(gEoEventInfo.initHandle);

failure:
    return rc;
}

ClRcT clEoEventExit(void )
{
    if(!isEventInitDone())
    {
        /*
         * Event Library wasn't initialized so finalize not necessary
         */
        return CL_OK;
    }

#ifdef CL_EO_TBD
    clEventFree(gEoEventInfo.eventHandle);
    clEventChannelClose(gEoEventInfo.channelHandle);
#endif
    clEventFinalize(gEoEventInfo.initHandle);

    gEoEventInfo.isInitDone = CL_FALSE;

    return CL_OK;
}

ClRcT clEoTriggerEvent(ClEoLibIdT libId, ClWaterMarkIdT wmId, ClUint32T wmValue, ClEoWaterMarkFlagT wmFlag)
{
    ClRcT rc = CL_OK;

    ClEventIdT eventId = 0;
    ClNameT publisherName = {sizeof(CL_EO_EVENT_PUBLISHER_NAME)-1, CL_EO_EVENT_PUBLISHER_NAME};

    ClEventPatternT patterns[5] = {{0}};

    ClEventPatternArrayT patternArray = {
        0,
        CL_SIZEOF_ARRAY(patterns),
        patterns 
    };

    if(!isEventInitDone())
    {
        rc = eoEventInit();
        if(CL_OK != rc)
        {
            return rc;
        }

    }

    patterns[0].patternSize = sizeof(CL_EO_NAME);
    patterns[0].pPattern = (ClUint8T *)CL_EO_NAME;

    patterns[1].patternSize = sizeof(libId);
    patterns[1].pPattern = (ClUint8T *)&libId;

    patterns[2].patternSize = sizeof(wmId);
    patterns[2].pPattern = (ClUint8T *)&wmId;

    patterns[3].patternSize = sizeof(wmFlag);
    patterns[3].pPattern = (ClUint8T *)&wmFlag;

    patterns[4].patternSize = sizeof(wmValue);
    patterns[4].pPattern = (ClUint8T *)(&wmValue);

    rc = clEventAttributesSet(gEoEventInfo.eventHandle, &patternArray, 
            CL_EVENT_HIGHEST_PRIORITY, 0, &publisherName);
    if(CL_OK != rc)
    {
        return rc;
    }

    rc = clEventPublish(gEoEventInfo.eventHandle, NULL, 0, &eventId);
    if(CL_OK != rc)
    {
        return rc;
    }

    return CL_OK;
}
