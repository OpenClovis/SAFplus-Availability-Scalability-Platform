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
 * ModuleName  : ckpt                                                          
 * File        : clCkptEvt.c
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
*
*   This file contains event related interactions of checkpoint server
*
*
*****************************************************************************/
#include <string.h>

#include <clCommon.h>
#include <clEventApi.h>
#include <clIocApi.h>
#include <clDebugApi.h>
#include <clCkptSvr.h>
#include <clCkptExtApi.h>
#include <clCkptUtils.h>
#include "clCkptSvrIpi.h"
#include <clEventExtApi.h>
#include <clCpmExtApi.h>
#include <clCkptPeer.h>
#include <clCkptCommon.h>
#include "xdrCkptPeerT.h"
#include "clCkptMaster.h"
#include <ckptEockptServerPeerPeerClient.h>

#define CL_CKPT_COMP_DEATH_SUBSCRIPTION_ID    1
#define CL_CKPT_NODE_ARRIVAL_SUBSCRIPTION_ID  2
#define CL_CKPT_NODE_DEATH_SUBSCRIPTION_ID  3

extern CkptSvrCbT  *gCkptSvr;
extern  ClVersionT gVersion;
extern  ClRcT clCkptMasterReplicaListUpdate(ClIocNodeAddressT peerAddr);

/*
 * Publisher name for the events published by ckpt.
 */
ClNameT publisherName = {sizeof(CL_CKPT_PUB_NAME)-1, CL_CKPT_PUB_NAME};

/* This routine is a call back routine for the subscribed events */
static void ckptEvtSubscribeCallBack( 
                               ClEventSubscriptionIdT    subscriptionId,
                               ClEventHandleT            eventHandle,
                               ClSizeT                   eventDataSize );



/* 
 *  This routine initializes the event service library to be used by ckpt 
 */

ClRcT   ckptEventSvcInitialize()
{
    ClVersionT                version        = CL_EVENT_VERSION;
    ClRcT                     rc             = CL_OK;
    ClEventChannelOpenFlagsT  openFlags      = 0;
    ClEventCallbacksT         evtCallbacks   = {NULL, 
                                               ckptEvtSubscribeCallBack};
    ClNameT                   clntUpdChlName = {0};
    ClNameT                   cpmChnlName    = {0};
    ClUint32T                 deathPattern   = htonl(CL_CPM_COMP_DEATH_PATTERN);
    ClUint32T                 nodeArrivalPattern = htonl(CL_CPM_NODE_ARRIVAL_PATTERN);
    ClUint32T                 nodeDeparturePattern = htonl(CL_CPM_NODE_DEATH_PATTERN);
    ClEventFilterT            compDeathFilter[]  = {{CL_EVENT_EXACT_FILTER, 
                                                {0, (ClSizeT)sizeof(deathPattern), (ClUint8T*)&deathPattern}}
    };
    ClEventFilterArrayT      compDeathFilterArray = {sizeof(compDeathFilter)/sizeof(compDeathFilter[0]), 
                                                     compDeathFilter
    };
    ClEventFilterT            nodeArrivalFilter[]         = { {CL_EVENT_EXACT_FILTER,
                                                                   {0, (ClSizeT)sizeof(nodeArrivalPattern),
                                                                   (ClUint8T*)&nodeArrivalPattern}}
    };
    ClEventFilterArrayT       nodeArrivalFilterArray = {sizeof(nodeArrivalFilter)/sizeof(nodeArrivalFilter[0]),
                                                        nodeArrivalFilter 
    };
    ClEventFilterT            nodeDepartureFilter[]         = { {CL_EVENT_EXACT_FILTER,
                                                                {0, (ClSizeT)sizeof(nodeDeparturePattern),
                                                                (ClUint8T*)&nodeDeparturePattern}}
    };
    ClEventFilterArrayT       nodeDepartureFilterArray = {sizeof(nodeDepartureFilter)/sizeof(nodeDepartureFilter[0]),
                                                          nodeDepartureFilter 
    };

    /* 
     * Initialize the event client library.
     */
    rc = clEventInitialize(&gCkptSvr->evtSvcHdl,
                           &evtCallbacks,
                           &version);
    if (rc != CL_EVENT_ERR_ALREADY_INITIALIZED )
        CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                ("Failed to initialize Event service rc[0x %x]\n",rc), rc);

    /* 
     * Open a channel for publishing event related to change in active 
     * replica address. 
     */
    clntUpdChlName.length = strlen(CL_CKPT_CLNTUPD_EVENT_CHANNEL);
    memcpy(clntUpdChlName.value, CL_CKPT_CLNTUPD_EVENT_CHANNEL, 
           clntUpdChlName.length);
           
    /* 
     * Open the channel.
     */
    rc = clEventChannelOpen(gCkptSvr->evtSvcHdl, &clntUpdChlName,
                            CL_EVENT_CHANNEL_PUBLISHER | 
                            CL_EVENT_GLOBAL_CHANNEL, 
                            (ClTimeT)-1, &gCkptSvr->clntUpdHdl);
    /* 
     * Allocate and set the attributes.
     */
    rc = clEventAllocate(gCkptSvr->clntUpdHdl, &gCkptSvr->clntUpdEvtHdl);
    rc = clEventExtAttributesSet(gCkptSvr->clntUpdEvtHdl, 
                                 CL_CKPT_UPDCLIENT_EVENT_TYPE,
                                 1, 0, &publisherName);

    /*
     * Open a channel for subscribing to node/component up/down events
     * from cpm.
     */

    /*
     * This is a global event.
     */
    openFlags = CL_EVENT_CHANNEL_SUBSCRIBER| CL_EVENT_GLOBAL_CHANNEL;

    cpmChnlName.length = strlen(CL_CPM_EVENT_CHANNEL_NAME);
    memcpy(cpmChnlName.value, CL_CPM_EVENT_CHANNEL_NAME, cpmChnlName.length);

    /* 
     * Open the channel for subscribing.
     */
    rc = clEventChannelOpen( gCkptSvr->evtSvcHdl,
            &cpmChnlName,
            openFlags,
            (ClTimeT)-1,
            &gCkptSvr->compChHdl);
    if (rc != CL_EVENT_ERR_EXIST)
    {
        CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR, 
                ("Channel %s open failed rc[0x%x]\n", cpmChnlName.value, rc),
                rc);
    }

    /* 
     * Subscribe to the component HB channel.
     */
    rc = clEventSubscribe(gCkptSvr->compChHdl, &compDeathFilterArray,
                          CL_CKPT_COMP_DEATH_SUBSCRIPTION_ID, NULL);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Event Subscribe failed :Channel %s rc[0x %x]\n",
             cpmChnlName.value, rc), rc);

    cpmChnlName.length = strlen(CL_CPM_NODE_EVENT_CHANNEL_NAME);
    memcpy(cpmChnlName.value, CL_CPM_NODE_EVENT_CHANNEL_NAME, 
            cpmChnlName.length);

    /* 
     * Open the channel for subscribing.
     */
    rc = clEventChannelOpen(gCkptSvr->evtSvcHdl, &cpmChnlName,
                            openFlags, (ClTimeT)-1,
                            &gCkptSvr->nodeChHdl);
    if (rc != CL_EVENT_ERR_EXIST)
    {
        CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR, 
                       ("Channel %s open failed [Rc: 0x%x]\n", 
                       cpmChnlName.value, rc), rc);
    }

    /* 
     * Subscribe to the node HB channel.
     */
    rc = clEventSubscribe(gCkptSvr->nodeChHdl, &nodeArrivalFilterArray,
                          CL_CKPT_NODE_ARRIVAL_SUBSCRIPTION_ID, NULL);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Event Subscribe failed for node arrival : Channel %s rc[0x %x]\n", 
             cpmChnlName.value, rc), rc);

    rc = clEventSubscribe(gCkptSvr->nodeChHdl, &nodeDepartureFilterArray,
                          CL_CKPT_NODE_DEATH_SUBSCRIPTION_ID, NULL);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Event Subscribe failed for node death : Channel %s rc[0x %x]\n", 
             cpmChnlName.value, rc), rc);

    return rc;

exitOnError:
    {
       /*
        * Do the necessary Cleanup.
        */
       if(gCkptSvr->compChHdl > 0) clEventChannelClose(gCkptSvr->compChHdl);
       if(gCkptSvr->nodeChHdl > 0) clEventChannelClose(gCkptSvr->nodeChHdl);
       if(gCkptSvr->clntUpdEvtHdl > 0) clEventFree(gCkptSvr->clntUpdEvtHdl);
       if(gCkptSvr->clntUpdHdl > 0) clEventChannelClose(gCkptSvr->clntUpdHdl);
       if (gCkptSvr->evtSvcHdl != 0) clEventFinalize(gCkptSvr->evtSvcHdl);
       return rc;
    }
}



/* 
 * This routine finalizes the event service library used by ckpt.
 */
 
ClRcT   ckptEventSvcFinalize()
{
    /* 
     * Close all the channels that were opened and all the events.
     */
    clEventChannelClose(gCkptSvr->compChHdl);
    clEventChannelClose(gCkptSvr->nodeChHdl);
    clEventFree(gCkptSvr->clntUpdEvtHdl);
    clEventChannelClose(gCkptSvr->clntUpdHdl);
    clEventFinalize(gCkptSvr->evtSvcHdl);
    return CL_OK;
}



/* 
 * This routine is a callback routine for the subscribed events.
 */

void ckptEvtSubscribeCallBack( ClEventSubscriptionIdT    subscriptionId,
                               ClEventHandleT            eventHandle,
                               ClSizeT                   eventDataSize )
{
    ClRcT                  rc          = CL_OK;
    ClCpmEventPayLoadT     payLoad     = {{0}}; 
    ClCpmEventNodePayLoadT nodePayload = {{0}};
    CkptPeerInfoT          *pPeerInfo  = NULL;

    if(gCkptSvr->serverUp == CL_FALSE)
    {
        clEventFree(eventHandle);
        return;
    }

    /*
     * Based on the subscriptionId, perform the required action.
     */
    switch(subscriptionId)
    {
    case CL_CKPT_COMP_DEATH_SUBSCRIPTION_ID:
        {
            /*
             * Extract the required information from the published event.
             */
            rc = clCpmEventPayLoadExtract(eventHandle, eventDataSize,
                                          CL_CPM_COMP_EVENT, (void *)&payLoad);
            CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR, 
                           ("Event Data Get failed rc[0x %x]\n", rc), rc);
            if(payLoad.operation != CL_CPM_COMP_DEATH)
                goto out_free;

            /* Based on the ioc port of the component that went down,
             * find whether it was ckpt server or any other application
             * and take appropriate actions.
             */
            if (payLoad.eoIocPort == CL_IOC_CKPT_PORT)
            {
                ckptPeerDown(payLoad.nodeIocAddress, CL_CKPT_SVR_DOWN, 
                             payLoad.eoIocPort);
            }
            else 
            {
                ckptPeerDown(payLoad.nodeIocAddress, CL_CKPT_COMP_DOWN, 
                             payLoad.eoIocPort); 
            }  
            break;
        }

    case CL_CKPT_NODE_DEATH_SUBSCRIPTION_ID:
        {
            /*
             * Extract the required information from the published event.
             */
            rc = clCpmEventPayLoadExtract(eventHandle, eventDataSize,
                                          CL_CPM_NODE_EVENT, (void *)&nodePayload);
            CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR, 
                           ("Event Data Get failed  [Rc: 0x%x]" , rc), rc);
                    
            /* 
             * Based on the "operation", identify whether the event is for
             * node up or node down and act accordingly.
             */
            if(nodePayload.operation != CL_CPM_NODE_DEATH)
                goto out_free;

            /*
             * Node going down.
             */
            CL_DEBUG_PRINT(CL_DEBUG_INFO,("Node is going down, name [%s], IOC address [%d]",
                                          nodePayload.nodeName.value, nodePayload.nodeIocAddress));
                
            /*
             * This call is not required for self node departure events.
             */
            if(nodePayload.nodeIocAddress != gCkptSvr->localAddr)
            {
                ckptPeerDown(nodePayload.nodeIocAddress, 
                             CL_CKPT_NODE_DOWN, 0); 
            }
            break;
        }
        
    case CL_CKPT_NODE_ARRIVAL_SUBSCRIPTION_ID:
        {
            /*
             * Node up event.
             * take the lock to avoid race 
             */
            rc = clCpmEventPayLoadExtract(eventHandle, eventDataSize,
                                          CL_CPM_NODE_EVENT, (void *)&nodePayload);
            CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR, 
                           ("Event Data Get failed  [Rc: 0x%x]" , rc), rc);

            if(nodePayload.operation != CL_CPM_NODE_ARRIVAL)
                goto out_free;
            
            CKPT_LOCK(gCkptSvr->masterInfo.ckptMasterDBSem);

            /*
             * If master server, then add the node's address to the 
             * peer list. Mark the entry as "unavailable" i.e.ckpt server
             * is not running on that node as of now. Whenever ckpt 
             * server comes up and sends "hello" then the master marks
             * the entrry as "available".
             */
            if(gCkptSvr->masterInfo.masterAddr == gCkptSvr->localAddr ||
               gCkptSvr->masterInfo.deputyAddr == gCkptSvr->localAddr )
            {
                rc = clCntDataForKeyGet(gCkptSvr->masterInfo.peerList,
                                        (ClPtrT)(ClWordT)nodePayload.nodeIocAddress,
                                        (ClCntDataHandleT *)&pPeerInfo);
                if( CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST)                    
                {
                    pPeerInfo = (CkptPeerInfoT*) clHeapCalloc(1, 
                                                              sizeof(CkptPeerInfoT));
                    pPeerInfo->addr        = nodePayload.nodeIocAddress;
                    pPeerInfo->credential  = CL_CKPT_CREDENTIAL_POSITIVE;
                    pPeerInfo->available   = CL_CKPT_NODE_UNAVAIL;
                        
                    rc = clCntLlistCreate(ckptCkptListKeyComp,
                                          ckptCkptListDeleteCallback,
                                          ckptCkptListDeleteCallback,
                                          CL_CNT_UNIQUE_KEY,
                                          &pPeerInfo->ckptList);
                    if( CL_OK != rc )
                    {
                        clLogError(CL_CKPT_AREA_PEER, CL_CKPT_CTX_REPL_NOTIFY, 
                                   "Checkpoint list create failed for peer list rc [#%x]", rc);
                        clHeapFree(pPeerInfo);
                        CKPT_UNLOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
                        goto exitOnError;
                    }

                    rc = clCntLlistCreate(ckptMastHdlListtKeyComp,
                                          ckptMastHdlListDeleteCallback,
                                          ckptMastHdlListDeleteCallback,
                                          CL_CNT_UNIQUE_KEY,
                                          &pPeerInfo->mastHdlList);
                    if( CL_OK != rc )
                    {
                        clLogError(CL_CKPT_AREA_PEER, CL_CKPT_CTX_REPL_NOTIFY, 
                                   "master handle list create failed for peer list rc [#%x]", rc);
                        clCntDelete(pPeerInfo->ckptList);                                       
                        clHeapFree(pPeerInfo);
                        CKPT_UNLOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
                        goto exitOnError;
                    }

                    rc = clCntNodeAdd(gCkptSvr->masterInfo.peerList,
                                      (ClPtrT)(ClWordT)nodePayload.nodeIocAddress,
                                      (ClCntDataHandleT)pPeerInfo, NULL);
                    if( CL_OK != rc )
                    {
                        clLogError(CL_CKPT_AREA_PEER, CL_CKPT_CTX_REPL_NOTIFY, 
                                   "Adding node address [%d] to peerlist failed for peer list rc [#%x]", 
                                   nodePayload.nodeIocAddress, rc);
                        clCntDelete(pPeerInfo->mastHdlList);
                        clCntDelete(pPeerInfo->ckptList);                                       
                        clHeapFree(pPeerInfo);
                        CKPT_UNLOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
                        goto exitOnError;
                    }
                }
            }     
            CKPT_UNLOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
            
            break;
	    }

    default:
        break;
    }

    /*
     * Free the event handle.
     */
    out_free:
    clEventFree(eventHandle);

    return;

    exitOnError:
    {
        /*
         * Free the event handle.
         */
        clEventFree(eventHandle);
        return;
    }
}
