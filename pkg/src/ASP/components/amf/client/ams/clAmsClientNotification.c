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

#include <clAmsClientNotification.h>
#include <xdrClAmsNotificationDescriptorT.h>
#include <clCpmExtApi.h>

static ClEventInitHandleT gClAmsEventNotificationHandle;
static ClEventChannelHandleT gClAmsEventChannelHandle;
static ClEventChannelHandleT gClCpmEventChannelHandle;
static ClEventChannelHandleT gClCpmNodeEventChannelHandle;
static ClAmsClientNotificationCallbackT gClAmsEventNotificationCallback;

ClRcT clAmsNotificationEventPayloadExtract(ClEventHandleT eventHandle,
                                           ClSizeT eventDataSize,
                                           void *payLoad)
{
    ClRcT rc = CL_OK;
    ClBufferHandleT payLoadMsg = 0;
    void *eventData = NULL;

    AMS_CHECKPTR ( !payLoad );
    
    eventData = clHeapAllocate(eventDataSize);
    if (!eventData) return CL_AMS_RC(CL_ERR_NO_MEMORY);

    rc = clEventDataGet (eventHandle, eventData,  &eventDataSize);
    if (CL_OK != rc)
    {
        clLogError("AMS", "NTF", "Unable to get event data, error [%#x]", rc);
        goto out_free;
    }
        
    rc = clBufferCreate(&payLoadMsg);
    if (CL_OK != rc) goto out_free;

    rc = clBufferNBytesWrite(payLoadMsg, (ClUint8T *)eventData, eventDataSize);
    if (CL_OK != rc) goto out;

    rc = VDECL_VER(clXdrUnmarshallClAmsNotificationDescriptorT, 4, 0, 0)(payLoadMsg, payLoad);
    if (CL_OK != rc) goto out;
    
out:
    clBufferDelete(&payLoadMsg);
out_free:
    clHeapFree(eventData);
    return rc;
}

static void clAmsClientNotificationCallback(
                                            ClEventSubscriptionIdT subscriptionId,
                                            ClEventHandleT eventHandle,
                                            ClSizeT eventDataSize)
{
    ClEventPatternArrayT    patternArray = {0};
    ClEventPriorityT        priority = 0;
    ClTimeT                 retentionTime = 0;
    ClNameT                 publisherName = {0};
    ClTimeT                 publishTime = 0;
    ClEventIdT              eventId = 0;
    ClAmsNotificationInfoT  amsNotificationInfo = {0};
    ClRcT                   rc = CL_OK;
    ClInt32T                i;

    rc = clEventAttributesGet(eventHandle, 
                              &patternArray, 
                              &priority,
                              &retentionTime, 
                              &publisherName,
                              &publishTime, 
                              &eventId );
    if(rc != CL_OK)
    {
        clLogError("AMS", "NTF", "Event attributes get failed, rc=[%#x]", rc);
        goto out_free;
    }

    switch(subscriptionId)
    {

    case 1:
        {
            ClRcT rc = CL_OK;
            rc = clAmsNotificationEventPayloadExtract(eventHandle, eventDataSize, 
                                                      (ClPtrT)&amsNotificationInfo.amsStateNotification);
            if(rc != CL_OK)
            {
                clLogError("AMS", "NTF", "Notification event payload extract returned [%#x]", rc);
                goto out_free2;
            }
            amsNotificationInfo.type = amsNotificationInfo.amsStateNotification.type;
        }
        break;

    case 2: 
        {
            rc = clCpmEventPayLoadExtract(eventHandle, 
                                          eventDataSize, 
                                          CL_CPM_COMP_EVENT, 
                                          (void *)&amsNotificationInfo.amsCompNotification);
            if (rc != CL_OK)
            {
                clLogError("AMS", "NTF", "Event payload extract failed, rc=[%#x]", rc);
                goto  out_free2;
            }
            if(amsNotificationInfo.amsCompNotification.operation == CL_CPM_COMP_ARRIVAL)
                amsNotificationInfo.type = CL_AMS_NOTIFICATION_COMP_ARRIVAL;
            else
                amsNotificationInfo.type = CL_AMS_NOTIFICATION_COMP_DEPARTURE;
            break;
        }

    case 3: 
        {
            rc = clCpmEventPayLoadExtract(eventHandle, 
                                          eventDataSize, 
                                          CL_CPM_NODE_EVENT, 
                                          (void *)&amsNotificationInfo.amsNodeNotification);
            if (rc != CL_OK)
            {
                goto out_free2;
            }
            if(amsNotificationInfo.amsNodeNotification.operation == CL_CPM_NODE_ARRIVAL)
                amsNotificationInfo.type = CL_AMS_NOTIFICATION_NODE_ARRIVAL;
            else
                amsNotificationInfo.type = CL_AMS_NOTIFICATION_NODE_DEPARTURE;
            break;
        }

    default: break;
    }
        
    if(gClAmsEventNotificationCallback)
    {
        gClAmsEventNotificationCallback(&amsNotificationInfo);
    }

    out_free2:
    for (i = 0; i < patternArray.patternsNumber; i++)
    {
        clHeapFree(patternArray.pPatterns[i].pPattern);
    }
    clHeapFree(patternArray.pPatterns);

    out_free:
    clEventFree(eventHandle);
}

ClRcT clAmsClientNotificationInitialize(ClAmsClientNotificationCallbackT callback)
{
    ClRcT rc = CL_OK;
    const ClEventCallbacksT evtCallbacks = 
        {
            NULL,
            .clEvtEventDeliverCallback=clAmsClientNotificationCallback,
        };
    ClNameT evtChannelName = { sizeof(CL_AMS_EVENT_CHANNEL_NAME)-1, CL_AMS_EVENT_CHANNEL_NAME };
    ClVersionT version = {'B', 0x1, 0x1};
    ClEventFilterT stateFilters[] = 
        {
            {CL_EVENT_EXACT_FILTER, {0, sizeof(CL_AMS_EVENT_PATTERN)-1, (ClUint8T *)CL_AMS_EVENT_PATTERN}}
        };
    ClEventFilterArrayT subscribeFilters = 
        {
            sizeof(stateFilters)/sizeof(ClEventFilterT),
            stateFilters
        };

    if(gClAmsEventNotificationHandle)
    {
        clLogError("AMS", "NTF", "AMS notification already registered");
        return CL_AMS_RC(CL_ERR_ALREADY_EXIST);
    }

    if(!callback)
    {
        clLogError("AMS", "NTF", "Notification callback cannot be NULL");
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    }

    rc = clEventInitialize(&gClAmsEventNotificationHandle, &evtCallbacks, &version);
    if(CL_OK != rc)
    {
        clLogError("AMS", "NTF", "Unable to initialize event client,"
                   " error [%#x]", rc);
        goto out;
    }

    rc = clEventChannelOpen(gClAmsEventNotificationHandle,
                            &evtChannelName,
                            CL_EVENT_CHANNEL_SUBSCRIBER | CL_EVENT_GLOBAL_CHANNEL,
                            (ClTimeT)-1,
                            &gClAmsEventChannelHandle);
    if(CL_OK != rc)
    {
        clLogError("AMS", "NTF", "Unable to open event channel,"
                   " error [%#x]", rc);
        goto out_free;
    }

    gClAmsEventNotificationCallback = callback;

    rc = clEventSubscribe(gClAmsEventChannelHandle, &subscribeFilters, 1, NULL);
    if(CL_OK != rc)
    {
        clLogError("AMS", "NTF", "Unable to subscribe for the event,"
                   " error [%#x]", rc);
        goto out_free;
    }

    clNameSet(&evtChannelName, CL_CPM_EVENT_CHANNEL_NAME);
    rc = clEventChannelOpen(gClAmsEventNotificationHandle,
                            &evtChannelName, 
                            CL_EVENT_CHANNEL_SUBSCRIBER | 
                            CL_EVENT_GLOBAL_CHANNEL, 
                            (ClTimeT)-1, 
                            &gClCpmEventChannelHandle);
    if (rc != CL_OK)
    {
        clLogError("AMS", "NTF",
                   "Channel [%.*s] open failed, rc=[%#x]", 
                   evtChannelName.length, 
                   evtChannelName.value,
                   rc);
        clEventUnsubscribe(gClAmsEventChannelHandle, 1);
        goto out_free;
    }

    rc = clEventSubscribe(gClCpmEventChannelHandle,
                          NULL,
                          2, 
                          NULL);
    if (rc != CL_OK)
    {
        clLogError("AMS", "NTF",
                   "Channel [%.*s] subscribe failed, rc=[%#x]", 
                   evtChannelName.length, 
                   evtChannelName.value,
                   rc);
        clEventUnsubscribe(gClAmsEventChannelHandle, 1);
        goto out_free;
    }

    clNameSet(&evtChannelName, CL_CPM_NODE_EVENT_CHANNEL_NAME);
    rc = clEventChannelOpen(gClAmsEventNotificationHandle, 
                            &evtChannelName, 
                            CL_EVENT_CHANNEL_SUBSCRIBER | 
                            CL_EVENT_GLOBAL_CHANNEL, 
                            (ClTimeT)-1, 
                            &gClCpmNodeEventChannelHandle);
    if (rc != CL_OK)
    {
        clLogError("AMS", "NTF",
                   "Channel [%.*s] open failed, rc=[%#x]", 
                   evtChannelName.length, 
                   evtChannelName.value,
                   rc);
        clEventUnsubscribe(gClAmsEventChannelHandle, 1);
        clEventUnsubscribe(gClCpmEventChannelHandle, 2);
        goto out_free;
    }

    rc = clEventSubscribe(gClCpmNodeEventChannelHandle,
                          NULL,
                          3, 
                          NULL);
    if (rc != CL_OK)
    {
        clLogError("AMS", "NTF",
                   "Channel [%.*s] subscribe failed, rc=[%#x]", 
                   evtChannelName.length, 
                   evtChannelName.value,
                   rc);
        clEventUnsubscribe(gClAmsEventChannelHandle, 1);
        clEventUnsubscribe(gClCpmEventChannelHandle, 2);
        goto out_free;
    }

    goto out;
    
    out_free:

    if(gClAmsEventChannelHandle)
    {
        clEventChannelClose(gClAmsEventChannelHandle);
        gClAmsEventChannelHandle = 0;
    }
    
    if(gClCpmEventChannelHandle)
    {
        clEventChannelClose(gClCpmEventChannelHandle);
        gClCpmEventChannelHandle = 0;
    }

    if(gClCpmNodeEventChannelHandle)
    {
        clEventChannelClose(gClCpmNodeEventChannelHandle);
        gClCpmNodeEventChannelHandle = 0;
    }

    if(gClAmsEventNotificationHandle)
    {
        clEventFinalize(gClAmsEventNotificationHandle);
        gClAmsEventNotificationHandle = 0;
    }

    if(gClAmsEventNotificationCallback) 
        gClAmsEventNotificationCallback = NULL;

    out:
    return rc;
}

ClRcT clAmsClientNotificationFinalize(void)
{
    if(!gClAmsEventNotificationHandle) return CL_AMS_RC(CL_ERR_NOT_INITIALIZED);
    gClAmsEventNotificationCallback = NULL;
    clEventChannelClose(gClAmsEventChannelHandle);
    clEventChannelClose(gClCpmEventChannelHandle);
    clEventChannelClose(gClCpmNodeEventChannelHandle);
    clEventFinalize(gClAmsEventNotificationHandle);
    gClAmsEventNotificationHandle = 0;
    return CL_OK;
}
