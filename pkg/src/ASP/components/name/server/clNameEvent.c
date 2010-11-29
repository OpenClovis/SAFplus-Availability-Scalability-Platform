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
/*******************************************************************************
 * ModuleName  : name
 * File        : clNameEvent.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file contains Name Service Server Side functionality
 * related to events.  
 *****************************************************************************/
                                                                                                                             
                                                                                                                             

#include "stdio.h"
#include "string.h"
#include "clCommon.h"
#include "clEoApi.h"
#include "../common/clNameCommon.h"
#include "clNameErrors.h"
#include "clCommonErrors.h"
#include "clNameIpi.h"
#include "clRmdApi.h"
#include "clDebugApi.h"
#include "clNameEventIpi.h"
#include "clCpmApi.h"
#include "clEventExtApi.h"
#include "clCpmExtApi.h"

#define CL_NM_EVT_COMP_DEATH_SUBSCRIPTION_ID 1
#define CL_NM_EVT_NODE_DEATH_SUBSCRIPTION_ID 2

ClEventChannelHandleT gEvtChannelHdl;
ClEventChannelHandleT gEvtChannelSubHdl;
ClEventChannelHandleT gEvtNodeChannelSubHdl;
ClEventInitHandleT    gNameEvtHdl;
ClEventHandleT        gEventHdl;

void nameSvcEventCallback( ClEventSubscriptionIdT subscriptionId,
        ClEventHandleT eventHandle, ClSizeT eventDataSize );

extern void invokeWalkForDelete(ClNameSvcDeregisInfoT *walkData);

ClNameT nameSvcPubChannelName = {
    sizeof(CL_NAME_PUB_CHANNEL)-1,
    CL_NAME_PUB_CHANNEL
};
                                                                                                                             
ClNameT nameSvcNodeSubChannelName = {
    sizeof(CL_CPM_NODE_EVENT_CHANNEL_NAME)-1,
    CL_CPM_NODE_EVENT_CHANNEL_NAME
};

ClNameT nameSvcSubChannelName = {
    sizeof(CL_CPM_EVENT_CHANNEL_NAME)-1,
    CL_CPM_EVENT_CHANNEL_NAME
};

ClEventCallbacksT evtCallbacks =
{
    NULL,
    nameSvcEventCallback,
};



/**
 *  Name: nameSvcEventInitialize
 *
 *  Function for initializing EM library, creating a channel and setting 
 *  the channel attributes and creating the event needed for updating 
 *  the NS users whenever threr is a change in NS DB.
 *
 *  @param  none 
 *
 *  @returns
 *    CL_OK                    - everything is ok <br>
 *
 */

ClRcT nameSvcEventInitialize()
{
    ClVersionT version = CL_EVENT_VERSION;
    ClRcT rc;
    ClNameT publisherName = {sizeof(CL_NAME_PUB_NAME)-1, CL_NAME_PUB_NAME}; 
    ClUint32T                 deathPattern   = htonl(CL_CPM_COMP_DEATH_PATTERN);
    ClUint32T                 nodeDeparturePattern = htonl(CL_CPM_NODE_DEATH_PATTERN);
    ClEventFilterT            compDeathFilter[]  = {{CL_EVENT_EXACT_FILTER, 
                                                {0, (ClSizeT)sizeof(deathPattern), (ClUint8T*)&deathPattern}}
    };
    ClEventFilterArrayT      compDeathFilterArray = {sizeof(compDeathFilter)/sizeof(compDeathFilter[0]), 
                                                     compDeathFilter
    };
    ClEventFilterT            nodeDepartureFilter[]         = { {CL_EVENT_EXACT_FILTER,
                                                                {0, (ClSizeT)sizeof(nodeDeparturePattern),
                                                                (ClUint8T*)&nodeDeparturePattern}}
    };
    ClEventFilterArrayT       nodeDepartureFilterArray = {sizeof(nodeDepartureFilter)/sizeof(nodeDepartureFilter[0]),
                                                          nodeDepartureFilter 
    };
   
    /* Initialize the event lib */
    rc = clEventInitialize(&gNameEvtHdl, &evtCallbacks, &version);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: Init Failed [%x]\n\r",rc));
        return rc;
    }

    /* Open a channel for publishing */
    rc = clEventChannelOpen(gNameEvtHdl, &nameSvcPubChannelName, 
                     CL_EVENT_CHANNEL_PUBLISHER, (ClTimeT)-1, &gEvtChannelHdl);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: Channel Open Failed [%x]\n\r",rc));
        /* Do necessary cleanup */
        goto label3;
    }

    /* Allocate the channel */
    rc = clEventAllocate(gEvtChannelHdl, &gEventHdl);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: Event Allocate Failed [%x]\n\r",rc));
        /* Do necessary cleanup */
        goto label2;
    }

    /* Set the attributes */
    rc = clEventExtAttributesSet(gEventHdl, CL_NAME_EVENT_TYPE, 
                                 1, 0, &publisherName);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Event Attribute Set Failed [%x]\n\r",rc));
        /* Do necessary cleanup */
        goto label1;
    }

    /* Open a channel for subscribing */
    rc = clEventChannelOpen(gNameEvtHdl, &nameSvcSubChannelName, 
                     CL_EVENT_GLOBAL_CHANNEL | CL_EVENT_CHANNEL_SUBSCRIBER,
                     (ClTimeT)-1, &gEvtChannelSubHdl);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: Channel Open Failed [%x]\n\r",rc));
        /* Do necessary cleanup */
        goto label3;
    }

    rc = clEventSubscribe(gEvtChannelSubHdl, &compDeathFilterArray, CL_NM_EVT_COMP_DEATH_SUBSCRIPTION_ID, NULL);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Subscriptions Failed [%x]\n\r",rc));
        return rc;
    }

    /* Open a channel for subscribing */
    rc = clEventChannelOpen(gNameEvtHdl, &nameSvcNodeSubChannelName, 
                     CL_EVENT_GLOBAL_CHANNEL | CL_EVENT_CHANNEL_SUBSCRIBER,
                     (ClTimeT)-1, &gEvtNodeChannelSubHdl);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: Channel Open Failed [%x]\n\r",rc));
        /* Do necessary cleanup */
        goto label3;
    }

    rc = clEventSubscribe(gEvtNodeChannelSubHdl, &nodeDepartureFilterArray, CL_NM_EVT_NODE_DEATH_SUBSCRIPTION_ID, NULL);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Subscriptions Failed [%x]\n\r",rc));
        return rc;
    }
    goto label4;

label1:
    clEventFree(gEventHdl);
label2:
    clEventChannelClose(gEvtChannelHdl);
    clEventChannelClose(gEvtChannelSubHdl);
label3:
    clEventFinalize(gNameEvtHdl);
label4:
    return rc;
}


/**
 *  Name: nameSvcEventFinalize 
 *
 *  This function deletes th eevevnt, event channel and finalizes the  
 *  EM library
 *
 *  @param  none 
 *
 *  @returns none
 *
 */

void nameSvcEventFinalize()
{
   clEventFree(gEventHdl);
   clEventChannelClose(gEvtChannelHdl);
   clEventChannelClose(gEvtChannelSubHdl);
   clEventFinalize(gNameEvtHdl);
   return;
}


void nameSvcEventCallback( ClEventSubscriptionIdT subscriptionId,
                           ClEventHandleT eventHandle, ClSizeT eventDataSize )
{
    ClRcT rc = CL_OK;
    ClCpmEventPayLoadT compPayload;
    ClCpmEventNodePayLoadT nodePayload;
    ClNameSvcDeregisInfoT    walkData = {0};

    switch (subscriptionId)
    {
    case CL_NM_EVT_NODE_DEATH_SUBSCRIPTION_ID:
        rc = clCpmEventPayLoadExtract(eventHandle, eventDataSize, 
                                      CL_CPM_NODE_EVENT, &nodePayload);
        if (rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Could not extract payload info from "
                                           "CPM event. rc 0x%x",rc));
            break;
        }

        if (nodePayload.operation != CL_CPM_NODE_DEATH)
            goto out_free;

        CL_DEBUG_PRINT(CL_DEBUG_TRACE,("Node with IOC address %d is going down"
                                       " Removing the name entries from that node",
                                       nodePayload.nodeIocAddress));

        walkData.nodeAddress = nodePayload.nodeIocAddress;
        walkData.operation   = CL_NS_NODE_DEREGISTER_OP;

        invokeWalkForDelete(&walkData);
        break;

    case CL_NM_EVT_COMP_DEATH_SUBSCRIPTION_ID:
        rc = clCpmEventPayLoadExtract(eventHandle, eventDataSize, 
                                      CL_CPM_COMP_EVENT, &compPayload);
        if (rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Could not extract payload info from "
                                           "CPM event. rc 0x%x",rc));
            break;
        }

        if (compPayload.operation != CL_CPM_COMP_DEATH)
            goto out_free;

        CL_DEBUG_PRINT(CL_DEBUG_TRACE,("Component with ID %d going down"
                                       " Removing the name entries from this component",
                                       compPayload.compId));
        walkData.compId    = compPayload.compId;
        walkData.eoID      = compPayload.eoId;
        walkData.operation = CL_NS_COMP_DEATH_DEREGISTER_OP;

        invokeWalkForDelete(&walkData);
            
        break;
    default:
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Received an event with subID %d",
                                       subscriptionId));
    }

    out_free:
    clEventFree(eventHandle);
}
