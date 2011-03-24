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

/**
 * This file implements CPM-GMS interaction.
 */

/*
 * Standard header files 
 */
#include <string.h>
#include <time.h>
#include <unistd.h>

/*
 * ASP header files 
 */
#include <clCommon.h>
#include <clCommonErrors.h>

#include <clOsalApi.h>
#include <clDebugApi.h>
#include <clLogApi.h>
#include <clIocApi.h>
#include <clIocErrors.h>
#include <clClmApi.h>
#include <clCmApi.h>
#include <clHandleApi.h>

/*
 * CPM internal include files 
 */
#include <clCpmInternal.h>
#include <clCpmCkpt.h>
#include <clCpmLog.h>
#include <clAms.h>

extern ClAmsT gAms;

#define CL_CPM_GMS_TRACK_RETRIES 3

ClRcT cpmUpdateTL(ClAmsHAStateT haState)
{
    ClIocTLInfoT tlInfo = {0};
    ClRcT rc = CL_OK;

    /*
     * Populate the TL structure 
     */
    cpmGetOwnLogicalAddress(&(tlInfo.logicalAddr));
    
    /*
     * CPM's component ID is slot number shifted
     * by CL_CPM_IOC_SLOT_BITS
     */
    CL_CPM_IOC_ADDRESS_SLOT_GET(clAspLocalId, tlInfo.compId);
    tlInfo.compId = tlInfo.compId << CL_CPM_IOC_SLOT_BITS;

    tlInfo.contextType = CL_IOC_TL_GLOBAL_SCOPE;
    tlInfo.physicalAddr.nodeAddress = clIocLocalAddressGet();
    tlInfo.physicalAddr.portId = CL_IOC_CPM_PORT;
    tlInfo.haState = haState;

    if (haState == CL_AMS_HA_STATE_ACTIVE)
    {
        tlInfo.haState = CL_IOC_TL_ACTIVE;

        if(gpClCpm->activeMasterNodeId != -1)
        {
            clLogDebug(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_GMS,
                       "Deregistering TL entry for node [%d]",
                       gpClCpm->activeMasterNodeId);

            clIocTransparencyDeregister((gpClCpm->activeMasterNodeId) <<
                                        CL_CPM_IOC_SLOT_BITS);
        }
    }
    else if(haState == CL_AMS_HA_STATE_STANDBY)
    {
        tlInfo.haState = CL_IOC_TL_STDBY;
    }

    clLogMultiline(CL_LOG_DEBUG, CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_GMS,
                   "Updating transparency layer information for node [%s] -- \n"
                   "Component ID : [%#x] \n"
                   "Context type : [%s] \n"
                   "Node address : [%d] \n"
                   "Port ID : [%d] \n"
                   "HA state : [%s]",
                   gpClCpm->pCpmLocalInfo->nodeName,
                   tlInfo.compId,
                   tlInfo.contextType == CL_IOC_TL_GLOBAL_SCOPE ? "Global" :
                   "Local",
                   tlInfo.physicalAddr.nodeAddress,
                   tlInfo.physicalAddr.portId,
                   tlInfo.haState == CL_IOC_TL_ACTIVE ? "Active" : "Standby");

    /*
     * Update the TL with the CPM's logical address 
     */
    rc = clIocTransparencyRegister(&tlInfo);
    if (CL_IOC_ERR_TL_DUPLICATE_ENTRY == CL_GET_ERROR_CODE(rc)
        ||
        CL_ERR_ALREADY_EXIST == CL_GET_ERROR_CODE(rc))
    {
        clLogWarning(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_GMS,
                     "Transparency register returned "
                     "duplicate entry for node address [%d]",
                     tlInfo.physicalAddr.nodeAddress);
        return CL_OK;
    }

    return rc;
}

static void cpmInitializeStandby(void)
{
    clLogInfo(CPM_LOG_AREA_CPM, CL_LOG_CONTEXT_UNSPECIFIED, "Started as STANDBY, so initialize checkpoint...");

    cpmCpmLStandbyCheckpointInitialize();
    if (gpClCpm->cpmToAmsCallback != NULL &&
        gpClCpm->cpmToAmsCallback->ckptServerReady != NULL)
    {
        gpClCpm->cpmToAmsCallback->ckptServerReady(gpClCpm->ckptHandle, CL_AMS_INSTANTIATE_MODE_STANDBY);
    }

    /* Inform AMS , that Event is up and running */
    if (gpClCpm->cpmToAmsCallback != NULL &&
        gpClCpm->cpmToAmsCallback->eventServerReady != NULL)
    {
        gpClCpm->cpmToAmsCallback->eventServerReady(CL_TRUE);
    }
    
}

static void cpmMakeSCActiveOrDeputy(const ClGmsClusterNotificationBufferT *notificationBuffer,
                                    ClCpmLocalInfoT *pCpmLocalInfo)
{
    ClUint32T rc = CL_OK;
    ClGmsNodeIdT prevMasterNodeId = gpClCpm->activeMasterNodeId;
    ClBoolT leadershipChanged = notificationBuffer->leadershipChanged; 
    
    /*  
     * Check for initial leadership state incase the cluster track from AMF was issued
     * after GMS leader election was done and GMS responded back with a track with a leadership changed
     * set to FALSE for a CURRENT async request from AMF.
     */
    if(leadershipChanged == CL_FALSE && 
       notificationBuffer->leader == pCpmLocalInfo->nodeId &&
       prevMasterNodeId != pCpmLocalInfo->nodeId && 
       gpClCpm->haState == CL_AMS_HA_STATE_NONE)
        leadershipChanged = CL_TRUE;
        
    if (leadershipChanged == CL_TRUE)
    {
        if (notificationBuffer->leader == pCpmLocalInfo->nodeId)
        {
            clLogInfo(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_GMS,
                      "Node [%d] has become the leader of the cluster",
                      pCpmLocalInfo->nodeId);

            if(gpClCpm->haState != CL_AMS_HA_STATE_ACTIVE)
            {
                rc = cpmUpdateTL(CL_AMS_HA_STATE_ACTIVE);
                if (rc != CL_OK)
                {
                    clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
                               CL_CPM_LOG_1_TL_UPDATE_FAILURE, rc);
                }
            }
            gpClCpm->deputyNodeId = notificationBuffer->deputy;
        }
        else if (notificationBuffer->deputy == pCpmLocalInfo->nodeId)
        {
            clLogInfo(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_GMS,
                      "Node [%d] has become the deputy of the cluster",
                      pCpmLocalInfo->nodeId);

            rc = cpmUpdateTL(CL_AMS_HA_STATE_STANDBY);
            if (rc != CL_OK)
            {
                clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
                           CL_CPM_LOG_1_TL_UPDATE_FAILURE, rc);
            }
        }

        if ((gpClCpm->haState == CL_AMS_HA_STATE_ACTIVE) && 
            (notificationBuffer->leader != pCpmLocalInfo->nodeId))
        {
            clLogDebug(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_GMS,
                       "Node [%d] is changing HA state from active to standby",
                       pCpmLocalInfo->nodeId);
              
            /*
             * Deregister the entry during a state change.
             */
            if (notificationBuffer->deputy != pCpmLocalInfo->nodeId)
            {
                clIocTransparencyDeregister((pCpmLocalInfo->nodeId) <<
                                            CL_CPM_IOC_SLOT_BITS);
            }

            /*
             * Inform AMS to become standby and read the checkpoint.
             */
            if ((gpClCpm->cpmToAmsCallback != NULL) && 
                (gpClCpm->cpmToAmsCallback->amsStateChange != NULL))
            {
                clLogDebug(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_GMS,
                           "Informing AMS on node [%d] to change state "
                           "from active to standby...",
                           pCpmLocalInfo->nodeId);

                rc = gpClCpm->cpmToAmsCallback->amsStateChange(CL_AMS_STATE_CHANGE_ACTIVE_TO_STANDBY |
                                                               CL_AMS_STATE_CHANGE_USE_CHECKPOINT);
                if (CL_OK != rc)
                {
                    clLogWarning(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_AMS,
                                 "AMS state change from active to standby "
                                 "returned [%#x]",
                                 rc);
                }

                cpmWriteNodeStatToFile("AMS", CL_NO);
                   
                if (((notificationBuffer->numberOfItems == 0) &&
                     (notificationBuffer->notification == NULL)) &&
                    gpClCpm->polling &&
                    (gpClCpm->nodeLeaving == CL_FALSE))
                {
                    /*
                     * This indicates that leader election API of
                     * GMS was called.  Since this involves
                     * interaction among only system controllers,
                     * we don't need to restart the worker nodes
                     * like in the case of split brain handling.
                     */
                        
                    cpmActive2Standby(CL_NO);
                }
                else if ((notificationBuffer->deputy == pCpmLocalInfo->nodeId)
                         && 
                         gpClCpm->polling 
                         &&
                         (gpClCpm->nodeLeaving == CL_FALSE))
                {
                    /*
                     * We try and handle a possible split brain
                     * since presently GMS shouldnt be reelecting.
                     * And even if it does, its pretty much
                     * invalid with respect to AMS where you could
                     * land up with 2 actives.
                     */
                     
                    /*
                     * We arent expected to return back.
                     */
                    cpmActive2Standby(CL_YES);
                }
            }

            /*
             * Bug 4168:
             * Updating the data structure gpClCpm, when the active becomes
             * standby.
             */
            gpClCpm->haState = CL_AMS_HA_STATE_STANDBY;
            gpClCpm->activeMasterNodeId = notificationBuffer->leader;
            gpClCpm->deputyNodeId = notificationBuffer->deputy;
        }
        else if ((gpClCpm->haState == CL_AMS_HA_STATE_STANDBY) && 
                 (notificationBuffer->leader == pCpmLocalInfo->nodeId))
        {
            rc = cpmStandby2Active(prevMasterNodeId, 
                                   notificationBuffer->deputy);
            if (CL_OK != rc)
            {
                return;
            }
        }
        else if ((gpClCpm->haState == CL_AMS_HA_STATE_NONE) && 
                 (notificationBuffer->leader ==
                  pCpmLocalInfo->nodeId))
        {
            /*
             * Bug 4411:
             * Added the if-else block.
             * Calling the AMS initialize only if both the callback pointers
             * are not NULL.
             * FIXME: This change is sort of workaround for 2.2.
             * Neat solution is to protect gpClCpm structure and fields properly
             */
            if ((gpClCpm->amsToCpmCallback != NULL) &&
                (gpClCpm->cpmToAmsCallback != NULL))
            {
                clLogDebug(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_AMS,
                           "Initializing AMS in active mode...");

                rc = clAmsInitialize(&gAms,
                                     gpClCpm->amsToCpmCallback,
                                     gpClCpm->cpmToAmsCallback,
                                     CL_AMS_INSTANTIATE_MODE_ACTIVE | CL_AMS_INSTANTIATE_USE_CHECKPOINT);
                /*
                 * Bug 4092:
                 * If the AMS intitialize fails then do the 
                 * self shut down.
                 */
                if (CL_OK != rc)
                {
                    clLogCritical(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_AMS,
                                  "Unable to initialize AMS, "
                                  "error = [%#x]", rc);

                    gpClCpm->amsToCpmCallback = NULL;
                    cpmSelfShutDown();

                    return;
                }
            }
            else
            {
                rc = CL_CPM_RC(CL_ERR_NULL_POINTER);
                 
                if (!gpClCpm->polling)
                {
                    clLogCritical(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_AMS,
                                  "AMS finalize called before AMS initialize "
                                  "during node shutdown.");
                }
                else
                {
                    clLogCritical(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_AMS,
                                  "Unable to initialize AMS, "
                                  "error = [%#x]", rc);
                     
                    cpmSelfShutDown();
                }
                return;
            }
             
            gpClCpm->haState = CL_AMS_HA_STATE_ACTIVE;
            gpClCpm->activeMasterNodeId = notificationBuffer->leader;
            gpClCpm->deputyNodeId = notificationBuffer->deputy;
        }
        else if ((gpClCpm->haState == CL_AMS_HA_STATE_NONE) && 
                 (notificationBuffer->deputy ==
                  pCpmLocalInfo->nodeId))
        {
            /*
             * Bug 4411:
             * Added the if-else block.
             * Calling the clAmsInitialize only if both the callback pointers
             * are not NULL.
             * FIXME: This change is sort of workaround for 2.2.
             * Neat solution is to protect gpClCpm structure and fields properly
             */
            if ((gpClCpm->amsToCpmCallback != NULL) &&
                (gpClCpm->cpmToAmsCallback != NULL))
            {
                clLogDebug(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_AMS,
                           "Initializing AMS in standby mode...");

                rc = clAmsInitialize(&gAms,
                                     gpClCpm->amsToCpmCallback,
                                     gpClCpm->cpmToAmsCallback,
                                     CL_AMS_INSTANTIATE_MODE_STANDBY);
                /*
                 * Bug 4092:
                 * If the AMS initialize fails then do the 
                 * self shut down.
                 */
                if (CL_OK != rc)
                {
                    clLogCritical(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_AMS,
                                  "Unable to initialize AMS, "
                                  "error = [%#x]", rc);

                    gpClCpm->amsToCpmCallback = NULL;
                    cpmSelfShutDown();

                    return;
                }
            }
            else
            {
                rc = CL_CPM_RC(CL_ERR_NULL_POINTER);
                 
                if (!gpClCpm->polling)
                {
                    clLogCritical(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_AMS,
                                  "AMS finalize called before AMS initialize "
                                  "during node shutdown.");
                }
                else
                {
                    clLogCritical(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_AMS,
                                  "Unable to initialize AMS, "
                                  "error = [%#x]", rc);
                    cpmSelfShutDown();
                }
                return;
            }

            gpClCpm->haState = CL_AMS_HA_STATE_STANDBY;
            gpClCpm->activeMasterNodeId = notificationBuffer->leader;
            gpClCpm->deputyNodeId = notificationBuffer->deputy;
            if(gpClCpm->bmTable->currentBootLevel == pCpmLocalInfo->defaultBootLevel)
            {
                cpmInitializeStandby();
            }
        }
        else
        {
            gpClCpm->activeMasterNodeId = notificationBuffer->leader;
            gpClCpm->deputyNodeId = notificationBuffer->deputy;
        }

        clLogNotice(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_CPM,
                    "HA state of node [%s] with node ID [%d] is [%s], "
                    "Master node is [%d]",
                    pCpmLocalInfo->nodeName,
                    pCpmLocalInfo->nodeId,
                    gpClCpm->haState == CL_AMS_HA_STATE_ACTIVE  ? "Active":
                    gpClCpm->haState == CL_AMS_HA_STATE_STANDBY ? "Standby":
                    "None",
                    gpClCpm->activeMasterNodeId);
    }
    else
    {
        /*
         * Always update the deputy node ID.  It may be that this
         * path is reached because a deputy node failed.
         */
        gpClCpm->deputyNodeId = notificationBuffer->deputy;

        if (CL_CPM_IS_ACTIVE())
        {
            if(notificationBuffer->notification && 
               notificationBuffer->numberOfItems == 1)
            {

                if (CL_GMS_NODE_LEFT ==
                    notificationBuffer->notification->clusterChange)
                {
                    cpmFailoverNode(notificationBuffer->notification->
                                    clusterNode.nodeId, CL_FALSE);
                }
                else
                {
                    clLogNotice(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_AMS, 
                                "Ignoring notification of type [%d] for node [%d]",
                                notificationBuffer->notification->clusterChange,
                                notificationBuffer->notification->clusterNode.nodeId);
                }
            }
            else if(notificationBuffer->notification)
            {
                clLogNotice(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_AMS, 
                            "Ignoring notification with number of items [%d], first type [%d]",
                            notificationBuffer->numberOfItems,
                            notificationBuffer->notification->clusterChange);
            }
        }
        else if( gpClCpm->haState == CL_AMS_HA_STATE_STANDBY
                 &&
                 notificationBuffer->deputy != pCpmLocalInfo->nodeId )
        {
            clLogNotice(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_CPM,
                        "HA state of node [%s] with node ID [%d] transitioning from standby to None, "
                        "Master node is still [%d]", 
                        pCpmLocalInfo->nodeName, pCpmLocalInfo->nodeId,
                        gpClCpm->activeMasterNodeId);
            gpClCpm->haState = CL_AMS_HA_STATE_NONE;
        }
        else if ((gpClCpm->haState == CL_AMS_HA_STATE_NONE) &&
                 (notificationBuffer->deputy == pCpmLocalInfo->nodeId))
        {
            if(CL_GMS_NODE_JOINED ==
               notificationBuffer->notification->clusterChange)
            {
                cpmStandbyRecover(notificationBuffer);
            }
            else if(gpClCpm->bmTable->currentBootLevel == pCpmLocalInfo->defaultBootLevel)
            {
                cpmStandbyRecover(notificationBuffer);
                cpmInitializeStandby();
            }
        }
    }
    return ;
}

static void cpmPayload2StandbySC(const ClGmsClusterNotificationBufferT *notificationBuffer,
                                 ClCpmLocalInfoT *pCpmLocalInfo)
{
    ClUint32T rc = CL_OK;

    if(gpClCpm->haState == CL_AMS_HA_STATE_STANDBY)
    {
        clLogWarning(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_AMS,
                     "Payload node already promoted to Deputy. Skipping initialization of controller functions");
        return;
    }

    clLogNotice(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_AMS, 
                "Payload node promoted to deputy. Initializing System controller functions on this node.");

    gpClCpm->pCpmConfig->cpmType = CL_CPM_GLOBAL;
    rc = cpmUpdateTL(CL_AMS_HA_STATE_STANDBY);
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
                   CL_CPM_LOG_1_TL_UPDATE_FAILURE, rc);
    }
    rc = clCpmAmsToCpmInitialize(&(gpClCpm->amsToCpmCallback));
    CL_CPM_CHECK(CL_DEBUG_ERROR,
                 ("Unable Initialize AmsToCpm callback %x\n", rc), rc);

    gpClCpm->cpmToAmsCallback =
        (ClCpmCpmToAmsCallT *) clHeapAllocate(sizeof(ClCpmCpmToAmsCallT));
    CL_ASSERT(gpClCpm->cpmToAmsCallback != NULL);
    rc = clAmsInitialize(&gAms,
                         gpClCpm->amsToCpmCallback,
                         gpClCpm->cpmToAmsCallback,
                         CL_AMS_INSTANTIATE_MODE_STANDBY);
    gpClCpm->haState = CL_AMS_HA_STATE_STANDBY;
    gpClCpm->activeMasterNodeId = notificationBuffer->leader;
    gpClCpm->deputyNodeId = notificationBuffer->deputy;
    if (CL_OK != rc)
    {
        clLogCritical(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_AMS,
                      "Unable to initialize AMS, "
                      "error = [%#x]", rc);

        gpClCpm->amsToCpmCallback = NULL;
        cpmSelfShutDown();

        return;
    }
    clLogInfo(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_AMS,
              "initialization of AMS is successful. ");
    
    if (gpClCpm->bmTable->currentBootLevel == pCpmLocalInfo->defaultBootLevel)
    {
        cpmInitializeStandby();
    }
    
    clLogNotice(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_AMS,
                "Payload to System Controller conversion done");

    failure:    
    return ;
}

void cpmHandleGroupInformation(const ClGmsClusterNotificationBufferT
                               *notificationBuffer)
{
    ClCpmLocalInfoT *pCpmLocalInfo = NULL;
    ClInt32T nodeUp = 0;

    clOsalMutexLock(&gpClCpm->clusterMutex);

    clOsalMutexLock(&gpClCpm->heartbeatMutex);
    nodeUp = gpClCpm->polling;
    clOsalMutexUnlock(&gpClCpm->heartbeatMutex);

    if(!nodeUp)
        goto failure;

    clOsalMutexLock(&gpClCpm->cpmMutex);
    pCpmLocalInfo = gpClCpm->pCpmLocalInfo;
    clOsalMutexUnlock(&gpClCpm->cpmMutex);
    if(pCpmLocalInfo == NULL)
    {
        goto failure;
    }

    if (CL_CPM_IS_SC())
    {
        cpmMakeSCActiveOrDeputy(notificationBuffer, pCpmLocalInfo);
    }
    else if (CL_CPM_IS_WB())
    {
        if (notificationBuffer->deputy == pCpmLocalInfo->nodeId)
        {
            cpmPayload2StandbySC(notificationBuffer, pCpmLocalInfo);
        }
        else if (notificationBuffer->leader == pCpmLocalInfo->nodeId)
        {
            clLogCritical(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_CPM,
                          "currently we are not supporting promoting Payload to Active SC directly");
            CL_ASSERT(0);
        }
        else
        {
            gpClCpm->activeMasterNodeId = notificationBuffer->leader;
            gpClCpm->deputyNodeId = notificationBuffer->deputy;
            if (gpClCpm->bmTable->currentBootLevel > CL_CPM_BOOT_LEVEL_2 )
            {
                if (-1 == notificationBuffer->leader)
                {
                    clLogMultiline(CL_LOG_CRITICAL,
                                   CPM_LOG_AREA_CPM,
                                   CPM_LOG_CTX_CPM_GMS,
                                   "The cluster has no leader !! "
                                   "Recovering...");
                    cpmGoBackToRegister();
                }
            }
        }
    }
    else
    {
        CL_ASSERT(0);
    }

    failure:
    clOsalMutexUnlock(&gpClCpm->clusterMutex);
    return;
}

void cpmClusterTrackCallBack(const ClGmsClusterNotificationBufferT
                             *clusterNotificationBuffer,
                             ClUint32T nMembers,
                             ClRcT rc)
{
    clLogMultiline(CL_LOG_DEBUG, CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_GMS,
                   "Received cluster track callback from GMS on node [%s] -- \n"
                   "Leader : [%d] \n"
                   "Deputy : [%d] (-1 -> No deputy) \n"
                   "Leader changed ? [%s] \n"
                   "Number of nodes in callback : [%d]",
                   gpClCpm->pCpmLocalInfo->nodeName,
                   clusterNotificationBuffer->leader,
                   clusterNotificationBuffer->deputy,
                   clusterNotificationBuffer->leadershipChanged ? "Yes" : "No",
                   clusterNotificationBuffer->numberOfItems);

    clOsalMutexLock(&gpClCpm->cpmGmsMutex);
    gpClCpm->trackCallbackInProgress = CL_TRUE;
    clOsalMutexUnlock(&gpClCpm->cpmGmsMutex);
    cpmHandleGroupInformation(clusterNotificationBuffer);

    rc = clOsalMutexLock(&gpClCpm->cpmGmsMutex);
    gpClCpm->trackCallbackInProgress = CL_FALSE;
    CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_OSAL_MUTEX_LOCK_ERR, rc, rc,
                   CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
    rc = clOsalCondSignal(&gpClCpm->cpmGmsCondVar);
    CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_OSAL_COND_SIGNAL_ERR, rc, rc,
                   CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
    rc = clOsalMutexUnlock(&gpClCpm->cpmGmsMutex);
    CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_OSAL_MUTEX_UNLOCK_ERR, rc, rc,
                   CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
failure:
    return;
}

ClRcT cpmGmsInitialize(void)
{
    ClRcT rc = CL_OK;
    ClTimerTimeOutT timeOut = {0, 0};

    ClGmsCallbacksT cpmGmsCallbacks =
        {
            .clGmsClusterTrackCallback = cpmClusterTrackCallBack
        };
    
    gpClCpm->version.releaseCode = 'B';
    gpClCpm->version.majorVersion = 0x01;
    gpClCpm->version.minorVersion = 0x01;
    rc = clGmsInitialize(&gpClCpm->cpmGmsHdl,
                         &cpmGmsCallbacks,
                         &gpClCpm->version);
    if (CL_OK != rc)
    {
        clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_GMS,
                   "Failed to do GMS initialization, error [%#x]",
                   rc);
        gpClCpm->cpmGmsHdl = CL_HANDLE_INVALID_VALUE;
        goto failure;
    }

    rc = clOsalMutexLock(&gpClCpm->cpmGmsMutex);
    CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_OSAL_MUTEX_LOCK_ERR, rc, rc,
                   CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

    rc = clGmsClusterTrack(gpClCpm->cpmGmsHdl, CL_GMS_TRACK_CHANGES_ONLY | CL_GMS_TRACK_CURRENT, NULL);
    if (CL_OK != rc)
    {
        clOsalMutexUnlock(&gpClCpm->cpmGmsMutex);
        clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_GMS,
                   "The GMS cluster track function failed, error [%#x]",
                   rc);
        goto failure;
    }

    /*
     * Wait for the GMS callback
     */
    retry:
#ifdef VXWORKS_BUILD
    timeOut.tsSec = gpClCpm->cpmGmsTimeout + 20;
#else
    timeOut.tsSec = gpClCpm->cpmGmsTimeout + 10;
#endif
    timeOut.tsMilliSec = 0;

    clLogInfo(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_GMS,
              "Node [%s] waiting for GMS cluster track callback for [%d.%d] secs",
              gpClCpm->pCpmLocalInfo->nodeName, timeOut.tsSec, timeOut.tsMilliSec);

    rc = clOsalCondWait(&gpClCpm->cpmGmsCondVar,
                        &gpClCpm->cpmGmsMutex,
                        timeOut);
    if (CL_OK != rc)
    {
        if(gpClCpm->trackCallbackInProgress)
        {
            clLogWarning(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_GMS,
                         "GMS cluster track callback in progress. Waiting for it to complete ...");
            goto retry;
        }
        clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_GMS,
                   "Failed to receive GMS cluster track callback, "
                   "error [%#x].",
                   rc);
        clOsalMutexUnlock(&gpClCpm->cpmGmsMutex);
        goto failure;
    }

    rc = clOsalMutexUnlock(&gpClCpm->cpmGmsMutex);
    CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_OSAL_MUTEX_UNLOCK_ERR, rc,
                   rc, CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

    return CL_OK;

    failure:
    return rc;
}

ClRcT cpmGmsTimerCallback(void *arg)
{
    ClRcT rc = CL_OK;
    
    rc = clGmsClusterTrack(gpClCpm->cpmGmsHdl, CL_GMS_TRACK_CHANGES_ONLY, NULL);
    if (CL_OK != rc)
    {
        clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_GMS,
                   "The GMS cluster track function failed, error [%#x]",
                   rc);
    }

    return rc;
}

ClRcT cpmGmsFinalize(void)
{
    if (gpClCpm->cpmGmsHdl != CL_HANDLE_INVALID_VALUE)
    {
        clGmsFinalize(gpClCpm->cpmGmsHdl);
    }

    return CL_OK;
}
