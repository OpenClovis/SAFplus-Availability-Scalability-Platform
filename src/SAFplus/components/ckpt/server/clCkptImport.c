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
 * ModuleName  : ckpt                                                          
 * File        : clCkptImport.c
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
*
*   This file contains routines that ckpt uses to get information from outside
*   world 
*
*
*****************************************************************************/

#include <netinet/in.h> 
#include <clCommon.h>
#include <clCkptSvr.h>
#include "clCkptSvrIpi.h"
#include "clCkptMaster.h"
#include <clCkptUtils.h>
#include <clClmApi.h>
#include <clEoApi.h>
#include <clCntApi.h>
#include "clCkptMaster.h"
#include "clCkptMasterUtils.h"
#include "clCkptIpi.h"
#include <string.h>
#include <clCkptPeer.h>
#include "clCkptDs.h"
#include "ckptEockptServerMasterDeputyClient.h"
#include <clCpmExtApi.h>

extern CkptSvrCbT  *gCkptSvr;

extern ClVersionT gVersion;

void clCkptTrackCallback();


/*
 * gms callback function structure.
 */
 
ClGmsCallbacksT ckptGmsCallbacks = {
    NULL,
    (ClGmsClusterTrackCallbackT) clCkptTrackCallback,
    NULL,
    NULL,
};



/*
 * Key compare function for clientHdl list
 */
ClInt32T ckptCkptListKeyComp(ClCntKeyHandleT key1,
                             ClCntKeyHandleT key2)
{
    return (ClWordT)key1 - (ClWordT)key2;
}

/*
 * Delete callback function for clientHdl list.
 */
 
void ckptCkptListDeleteCallback(ClCntKeyHandleT  userKey,
                                ClCntDataHandleT userData)
{
    CkptNodeListInfoT   *peerInfoDH = (CkptNodeListInfoT *) userData;
    clHeapFree(peerInfoDH);
    return;
}


/*
 * Key compare function for masterHdl list.
 */
 
ClInt32T ckptMastHdlListtKeyComp(ClCntKeyHandleT key1,
                             ClCntKeyHandleT key2)
{
    return *(ClHandleT *)key1 - *(ClHandleT *)key2;
}

/*
 * Delete callback function for masterHdl list.
 */
 
void ckptMastHdlListDeleteCallback(ClCntKeyHandleT  userKey,
                                ClCntDataHandleT userData)
{
    clHeapFree(userKey);
    return;
}

/*
 * This function will be called either from GMS track callback or 
 * IOC notification, it receives the master address & deputy and 
 * process the same
 */
ClRcT
clCkptMasterAddressUpdate(ClIocNodeAddressT  leader, 
                          ClIocNodeAddressT  deputy)
{
    ClIocTLInfoT tlInfo    = {0};
    SaNameT      name      = {0};
    ClRcT        rc        = CL_OK;
    ClBoolT      updateReq = CL_FALSE;

    /*
     * Check whether master or deputy address has changed.
     */
    if(gCkptSvr->masterInfo.masterAddr != leader)
    {
        /*
         * Master address changed.
         */
        updateReq = CL_TRUE;
        gCkptSvr->masterInfo.prevMasterAddr = gCkptSvr->masterInfo.masterAddr;    
        gCkptSvr->masterInfo.masterAddr     = leader;    
        
        /*
         * Deregister the old TL entry.
         */
        if(gCkptSvr->masterInfo.compId != CL_CKPT_UNINIT_VALUE)
        {
            rc = clIocTransparencyDeregister(gCkptSvr->masterInfo.compId);
        }
        else
        {
            clCpmComponentNameGet(gCkptSvr->amfHdl, &name);
            clCpmComponentIdGet(gCkptSvr->amfHdl, &name, 
                                &gCkptSvr->compId);
            gCkptSvr->masterInfo.compId = gCkptSvr->compId;
        }

        /*
         * Update the TL.
         */
        if(gCkptSvr->masterInfo.masterAddr == clIocLocalAddressGet())
        {
            ckptOwnLogicalAddressGet(&tlInfo.logicalAddr);
            tlInfo.compId                   = gCkptSvr->compId;
            gCkptSvr->masterInfo.compId     = gCkptSvr->compId;
            tlInfo.contextType              = CL_IOC_TL_GLOBAL_SCOPE;
            tlInfo.physicalAddr.nodeAddress = clIocLocalAddressGet();
            tlInfo.physicalAddr.portId      = CL_IOC_CKPT_PORT;
            tlInfo.haState                  = CL_IOC_TL_ACTIVE;
            rc = clIocTransparencyRegister(&tlInfo);
        }
        /*
         * update the ioc notify callbacks for the new master address 
         * Once address update is over, then we have uninstall the registered
         * callback and reregistered to new master address
         */
        clCkptIocCallbackUpdate();
    }
    
    if(gCkptSvr->masterInfo.deputyAddr != deputy)
    {
        /*
         * Deputy address has changed.
         */
        updateReq = CL_TRUE;
        gCkptSvr->masterInfo.deputyAddr = deputy ;    
    }

    /*
     * Signal the receipt of master and deputy addresses. 
     */
    clOsalMutexLock(gCkptSvr->mutexVar);
    if(gCkptSvr->condVarWaiting == CL_TRUE)
    {
        gCkptSvr->condVarWaiting = CL_FALSE;
        clOsalCondSignal(gCkptSvr->condVar);
    }
    clOsalMutexUnlock(gCkptSvr->mutexVar);

    /* 
     * Update the old master(if existing) with the new leader addresses.
     */
    if((updateReq == CL_TRUE) && 
       ((gCkptSvr->masterInfo.prevMasterAddr != -1) &&
       (gCkptSvr->masterInfo.prevMasterAddr != CL_CKPT_UNINIT_ADDR)))
    {
        rc = ckptIdlHandleUpdate(gCkptSvr->masterInfo.prevMasterAddr,
                                 gCkptSvr->ckptIdlHdl,0);
        rc = VDECL_VER(clCkptLeaderAddrUpdateClientAsync, 4, 0, 0)(gCkptSvr->ckptIdlHdl,
                                    gCkptSvr->masterInfo.masterAddr,
                                    gCkptSvr->masterInfo.deputyAddr,
                                    NULL,0);
    }
    clLogNotice("ADDR", "UPDATE", "CKPT master [%d], deputy [%d]",
                gCkptSvr->masterInfo.masterAddr, gCkptSvr->masterInfo.deputyAddr);
    return rc;
}


/*
 * Update the master and deputy server address in the master DB.\
 * Also update TL with master address.
 */
 
void   _clCkptAddressesUpdate
          (ClGmsClusterNotificationBufferT *notificationBuffer)
{
    ClRcT  rc = CL_OK;

    if( NULL  == gCkptSvr)
    {
        CKPT_DEBUG_E(("gCkptSvr is null\n"));
        return;
    }
    
    /*
     * Lock the master DB.
     */
    CKPT_LOCK(gCkptSvr->masterInfo.ckptMasterDBSem);

    rc = clCkptMasterAddressUpdate(notificationBuffer->leader,
            notificationBuffer->deputy);

    /*
     * Unlock the master DB.
     */
    CKPT_UNLOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
}



/*
 * Track change callback function. This function will be called bu gms 
 * whenever leader or deputy address gets changed.
 */
 
void clCkptTrackCallback(ClGmsClusterNotificationBufferT *notificationBuffer,
                         ClUint32T                       numberOfMembers,
                         ClRcT                           rc)
{
    ClIocNodeAddressT deputy = 0;
    ClIocNodeAddressT newDeputy = 0;

    /* Set Master and deputy addresses */
    if(!gCkptSvr) 
    {
        clLogWarning("GMS", "TRACK", "CKPT server context is not initialized");
        return;
    }
    clOsalMutexLock(&gCkptSvr->ckptClusterSem);
    deputy = gCkptSvr->masterInfo.deputyAddr;
    newDeputy = notificationBuffer->deputy;
    _clCkptAddressesUpdate(notificationBuffer);
    clOsalMutexUnlock(&gCkptSvr->ckptClusterSem);

    CKPT_LOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
    if (gCkptSvr->localAddr == newDeputy
        && 
        (deputy != gCkptSvr->localAddr
         ||
         !gCkptSvr->isSynced)
        )
    {
        /*
         * Add the deputy to our masterinfo peer list and announce the master about our arrival
         * and do the same.
         */
        _ckptSvrArrvlAnnounce();
        if (CL_OK != ckptMasterDatabaseSyncup(gCkptSvr->masterInfo.masterAddr))
        {
            clLogWarning(CL_CKPT_AREA_ACTIVE, "IOC", "ckptMasterDatabaseSyncup failed");
        } 
        else
        {
            clLogWarning(CL_CKPT_AREA_ACTIVE, "IOC", "ckptMasterDatabaseSyncup succeeded");
        }

        /*
         * If the masterdbsyncup fetched an old deputy during a IOC callback update 
         * overlapping on the master, then just patch it with the right one here.
         */
        if(gCkptSvr->masterInfo.deputyAddr != newDeputy)
            gCkptSvr->masterInfo.deputyAddr = newDeputy;

    }
    CKPT_UNLOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
}



/*
 * Register  with GMS for the leader election.
 */
 
ClRcT clCkptMasterAddressesSet()
{
    ClRcT rc = CL_OK;
#ifdef CL_CKPT_GMS 
    /*
     * GMS available.
     */
     
    /* 
     * Contact GMS to get master and backup master addresses.
     */
    ClGmsClusterNotificationBufferT notBuffer;
    memset((void*)&notBuffer , 0, sizeof(notBuffer));
    memset( &notBuffer , '\0' ,sizeof(ClGmsClusterNotificationBufferT));
    
    /*
     * Initialize the gms client library.
     */
    rc = clGmsInitialize(&gCkptSvr->gmsHdl, &ckptGmsCallbacks,
                        &gVersion);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
    (" clCkptMasterAddressesSet failed rc[0x %x]\n",rc),
        rc);
        
    /* 
     * Register for current track and track changes.
     */
    rc = clGmsClusterTrack(gCkptSvr->gmsHdl, 
                           CL_GMS_TRACK_CHANGES|CL_GMS_TRACK_CURRENT, 
                           &notBuffer);
                           
    /*
     * Call the track change callback funtion. This is needed for getting 
     * current track information.
     */
    if(notBuffer.leader && 
       notBuffer.leader != CL_GMS_INVALID_NODE_ID)
    {
        _clCkptAddressesUpdate(&notBuffer);
    }
    clHeapFree(notBuffer.notification);

#else
    /*
     * GMS unavailable.
     */

     /*
      * Select cpm master as ckpt master. Deputy will be unspecified.
      */
    clCpmMasterAddressGet(&gCkptSvr->masterInfo.masterAddr);
    gCkptSvr->masterInfo.deputyAddr = -1;

    /*
     * Update the TL with ckpt master address.
     */
    if(gCkptSvr->masterInfo.masterAddr == gCkptSvr->localAddr )
    {
        ClIocTLInfoT tlInfo = {0};
        ClUint32T    compId = 0; 
        SaNameT      name   = {0};
 
        clCpmComponentNameGet(gCkptSvr->amfHdl, &name);
        clCpmComponentIdGet(gCkptSvr->amfHdl, &name, &compId);
        tlInfo.compId                   = compId;
        gCkptSvr->masterInfo.compId     = compId;
        ckptOwnLogicalAddressGet(&tlInfo.logicalAddr);
        tlInfo.contextType              = CL_IOC_TL_GLOBAL_SCOPE;
        tlInfo.physicalAddr.nodeAddress = clIocLocalAddressGet();
        tlInfo.physicalAddr.portId      = CL_IOC_CKPT_PORT;
        tlInfo.haState                  = CL_IOC_TL_ACTIVE;
        rc = clIocTransparencyRegister(&tlInfo);
    }
#endif
#ifdef CL_CKPT_GMS 
exitOnError:
#endif
    {
        return rc;
    }
}



/*
 * Function call to update the peerlist of the master.
 * This function will be invoked whenever a node/component goes down/up.
 * SHOULD be invoked with ckpt masterdb lock held.
 */
 
ClRcT clCkptMasterPeerUpdateNoLock(ClIocPortT        portId, 
                                   ClUint32T         flag, 
                                   ClIocNodeAddressT localAddr,
                                   ClUint8T          credential) 
{
    ClRcT              rc           = CL_OK;
    CkptPeerInfoT      *pPeerInfo   = NULL;
    CkptNodeListInfoT  *pPeerInfoDH = NULL;
    ClCntNodeHandleT   nodeHdl      = 0;
    ClCntNodeHandleT   tempHdl      = 0;
    ClHandleT         *pMasterHandle  = NULL;
    
    /*
     * Check whether node/component is coming up or going down.
     */
    if(flag == CL_CKPT_SERVER_UP)
    {
        /*
         * Checkpoint server up scenario.
         */
         clLogDebug(CL_CKPT_AREA_MAS_DEP, CL_CKPT_CTX_PEER_ANNOUNCE,
		    "Received welcome message from master, updating the peerlist for [%d]", 
	            localAddr);
        /* 
         * Add an entry to the peer list if not existing.
         * Mark the node as "available" i.e. available for checkpoint 
         * operations like storing replicas etc..
         */
        rc = clCntDataForKeyGet( gCkptSvr->masterInfo.peerList,
                                 (ClPtrT)(ClWordT)localAddr,
                                 (ClCntDataHandleT *)&pPeerInfo);
        if( rc == CL_OK && pPeerInfo != NULL)
        {
            CL_ASSERT(pPeerInfo->ckptList != 0);
            pPeerInfo->credential = credential;
            pPeerInfo->available  = CL_CKPT_NODE_AVAIL;

            if(localAddr != gCkptSvr->localAddr)
            {
                clLogNotice("PEER", "UPDATE", 
                            "Resetting the replica list for the peer [%#x] being welcomed", localAddr);
                clCkptMasterReplicaListUpdateNoLock(localAddr);
                pPeerInfo->replicaCount = 0;
            }
        }
        else
        {
            if( CL_OK !=( rc = _ckptMasterPeerListInfoCreate(localAddr, 
                            credential,0)))
            {
                return rc;
            }
        }        
    }
    else
    {
        /*
         * Node/component down scenario.
         */
        clLogDebug(CL_CKPT_AREA_MAS_DEP, CL_CKPT_CTX_PEER_DOWN, 
                   "Updating the peerAddr [%d] for down notification",
                   localAddr);
        /* 
         * Find the corresponding entry from the peer list.
         */
        if( CL_OK != (rc = clCntDataForKeyGet(gCkptSvr->masterInfo.peerList,
                                             (ClCntKeyHandleT)(ClWordT)localAddr,
                                             (ClCntDataHandleT *) &pPeerInfo)))
        {
            rc = CL_OK;
            goto exitOnError;
        }

        if( flag != CL_CKPT_COMP_DOWN)
        {
            clLogDebug(CL_CKPT_AREA_MAS_DEP, CL_CKPT_CTX_PEER_DOWN,
                       "Either ckpt server or node down, "
                       "changing active address");
                    
            clCntFirstNodeGet(pPeerInfo->mastHdlList,&nodeHdl);
            tempHdl = 0;
            while(nodeHdl != 0)
            {
                rc = clCntNodeUserKeyGet(pPeerInfo->mastHdlList,nodeHdl,
                                    (ClCntKeyHandleT *)&pMasterHandle);
                if( CL_OK != rc )
                {
                    clLogError(CL_CKPT_AREA_MAS_DEP, CL_CKPT_CTX_PEER_DOWN, 
                            "Not able get the data for node handle rc[0x %x]",
                            rc);
                    goto exitOnError;
                }
                rc = clCntNextNodeGet(pPeerInfo->mastHdlList, nodeHdl, 
                                      &tempHdl);
                /*
                 * Update the active address and inform the clients.
                 */
                if( CL_OK != (rc = _clCkpMastertReplicaAddressUpdate(*pMasterHandle, 
                                                                 localAddr)))
                {
                    return rc;
                }
                nodeHdl = tempHdl;
                tempHdl = 0;
            }
        }
        
        if (flag != CL_CKPT_SVR_DOWN)
        {
            /* 
             * Component down/ node down case.
             * In case of component down close the associated client Hdl.
             * Incase of node down close all client Hdl.
             * Delete the ckpt Hdls from the client handle List.
             */
            clLogDebug(CL_CKPT_AREA_MAS_DEP, CL_CKPT_CTX_PEER_DOWN, 
                  "Closing the opened handles from this slot id [%d]...", 
                   localAddr);
            clCntFirstNodeGet(pPeerInfo->ckptList,&nodeHdl);
            while(nodeHdl != 0)
            {
                rc = clCntNodeUserDataGet(pPeerInfo->ckptList,nodeHdl,
                        (ClCntDataHandleT *)&pPeerInfoDH);
                if( CL_OK != rc )
                {
                    clLogError(CL_CKPT_AREA_MAS_DEP, CL_CKPT_CTX_PEER_DOWN, 
                            "Not able get the data for node handle rc[0x %x]",
                            rc);
                    goto exitOnError;
                }
                clCntNextNodeGet(pPeerInfo->ckptList,nodeHdl,&tempHdl);
                if ( (flag == CL_CKPT_COMP_DOWN && 
                     pPeerInfoDH->appPortNum == portId) || 
                     (flag == CL_CKPT_NODE_DOWN) )
                {
                    /*
                     * Close the checkpoint hdl but dont delete the entry from
                     * masterHdl list.
                     */
                    if(gCkptSvr->masterInfo.masterAddr == 
                                    gCkptSvr->localAddr) 
                    {                                    
                        clLogInfo(CL_CKPT_AREA_MAS_DEP,
                                  CL_CKPT_CTX_PEER_DOWN, 
                                  "Closing the handle [%#llX]...", 
                                  pPeerInfoDH->clientHdl);
                        _clCkptMasterCloseNoLock(pPeerInfoDH->clientHdl, 
                        localAddr, !CL_CKPT_MASTER_HDL); 
                    }    
                }
                nodeHdl = tempHdl;
                tempHdl = 0; 
            }
        }
        else if (flag == CL_CKPT_SVR_DOWN)
        {
            /*
             * Mark the availability of checkpoint server as UNAVAILABLE.
             */
            if(pPeerInfo->credential == CL_CKPT_CREDENTIAL_POSITIVE)
                gCkptSvr->masterInfo.availPeerCount--;
            pPeerInfo->available = CL_CKPT_NODE_UNAVAIL;
        }   

        if(flag == CL_CKPT_NODE_DOWN
           ||
           flag == CL_CKPT_SVR_DOWN)
        {
            
            /*
             * Node down case, delete the entry from master's peer list.
             */
            rc = clCntAllNodesForKeyDelete(gCkptSvr->masterInfo.peerList,
                                (ClPtrT)(ClWordT)localAddr);
             CKPT_ERR_CHECK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
             (" MasterPeerUpdate failed rc[0x %x]\n",rc),
             rc);

        }
        
        if( flag != CL_CKPT_COMP_DOWN)
        {
            /*
             * Find other nodes to store the replicas of checkpoints for whom
             * this node was storing the replicas.
             */
             if(gCkptSvr->masterInfo.masterAddr == gCkptSvr->localAddr)
             {
                 _ckptCheckpointLoadBalancing();
             }
        }
    }
exitOnError:
    {
        return rc;
    }
}  

ClRcT clCkptMasterPeerUpdate(ClIocPortT        portId, 
                             ClUint32T         flag, 
                             ClIocNodeAddressT localAddr,
                             ClUint8T          credential) 
{
    ClRcT rc = CL_OK;
    CKPT_LOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
    rc = clCkptMasterPeerUpdateNoLock(portId, flag, localAddr, credential);
    CKPT_UNLOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
    return rc;
}



/*
 * Function to get new active replica addrss and update the clients
 * about the change in active replica.
 */
 
ClRcT _clCkpMastertReplicaAddressUpdate(ClHandleT         mastHdl,
                                        ClIocNodeAddressT actAddr)
{
    ClRcT                   rc               = CL_OK;
    ClCntNodeHandleT        nodeHdl          = 0;
    ClCntDataHandleT        dataHdl          = 0;
    CkptMasterDBEntryT      *pMasterDBEntry  = NULL;
    ClCkptClientUpdInfoT    eventInfo        = {0};
    ClEventIdT              eventId          = 0;

    clLogDebug(CL_CKPT_AREA_MAS_DEP, CL_CKPT_CTX_PEER_DOWN, 
               "Changing the active address of ckpt handle [%#llX]",
               mastHdl);
    /*
     * Retrieve the information associated with the master hdl.
     */
    if( CL_OK != (rc = clHandleCheckout(gCkptSvr->masterInfo.masterDBHdl,
                                        mastHdl, (void **) &pMasterDBEntry)))
    {
        clLogError(CL_CKPT_AREA_MAS_DEP, CL_CKPT_CTX_PEER_DOWN, 
                   "Master db entry doesn't have this handle [%#llX]", 
                   mastHdl);
        return rc;
    }  

    /*
     * Delete the entry of the node that went down from the checkpoint's
     * replica list.
     */
    if (pMasterDBEntry->replicaList)
        clCntAllNodesForKeyDelete(pMasterDBEntry->replicaList, (ClPtrT)(ClWordT)actAddr);
    else
    {
        clLogWarning(CL_CKPT_AREA_MAS_DEP, CL_CKPT_CTX_PEER_DOWN,"Replicalist for %s is empty",pMasterDBEntry->name.value);
    }


    /*
     * Store the node's address as prev active address.
     */
    pMasterDBEntry->prevActiveRepAddr = actAddr; 

    /*
     * Select the new active address. In case of COLLOCATED checkpoint,
     * the new active address to UNINIT value.
     */
    if (CL_CKPT_IS_COLLOCATED(pMasterDBEntry->attrib.creationFlags)) 
    {
        if(pMasterDBEntry->activeRepAddr == actAddr)
        {
            pMasterDBEntry->activeRepAddr = CL_CKPT_UNINIT_ADDR;
        }
        else 
        {
            if(pMasterDBEntry->activeRepAddr != CL_CKPT_UNINIT_ADDR)
            {
                /*
                 * If we have traces of the master handle in our peer list 
                 * missed by the active replica set, then remove it here.
                 */
                clLogNotice(CL_CKPT_AREA_MAS_DEP, CL_CKPT_CTX_PEER_DOWN, 
                            "Active replica is [%d]."
                            "Removing master ckpt handle [%#llX] from last active [%d]",
                            pMasterDBEntry->activeRepAddr, mastHdl, actAddr);
                _ckptPeerListMasterHdlAdd(mastHdl, actAddr, CL_CKPT_UNINIT_ADDR);
            }
            goto exitOnError;
        }
    }
    else 
    {
        rc = clCntFirstNodeGet(pMasterDBEntry->replicaList, &nodeHdl);
        if(CL_ERR_INVALID_HANDLE == CL_GET_ERROR_CODE(rc) || 
           nodeHdl == 0)
        {
            rc = CL_OK;
            pMasterDBEntry->activeRepAddr = CL_CKPT_UNINIT_ADDR;
        }
        CKPT_ERR_CHECK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
                       ("clCkptActiveReplicaAddrGet failed rc[0x %x]\n",rc),
                       rc);
        if( nodeHdl != 0 ) 
        {
            rc = clCntNodeUserDataGet(pMasterDBEntry->replicaList, nodeHdl, &dataHdl);
            CKPT_ERR_CHECK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
                           ("clCkptActiveReplicaAddrGet failed rc[0x %x]\n",rc),
                           rc);
            pMasterDBEntry->activeRepAddr = (ClIocNodeAddressT)(ClWordT)dataHdl;
        }
    }

    /*
     * Inform the client about the change in active replica.
     */
    if((gCkptSvr->masterInfo.masterAddr == gCkptSvr->localAddr) ||
       (clCpmIsSCCapable() && 
        (pMasterDBEntry->prevActiveRepAddr == 
         gCkptSvr->masterInfo.masterAddr)))
    {
        eventInfo.eventType   = htonl(CL_CKPT_ACTIVE_REP_CHG_EVENT);
        eventInfo.actAddr     = htonl(pMasterDBEntry->activeRepAddr);
        saNameCopy(&eventInfo.name, &pMasterDBEntry->name);
        eventInfo.name.length = htons(pMasterDBEntry->name.length);

        clLogNotice(CL_CKPT_AREA_MAS_DEP, CL_CKPT_CTX_PEER_DOWN, 
                    "Changing the address from [%d] to [%d] for checkpoint [%.*s]",
                    actAddr, pMasterDBEntry->activeRepAddr, pMasterDBEntry->name.length, pMasterDBEntry->name.value);
        rc = clEventPublish(gCkptSvr->clntUpdEvtHdl, 
                            (const void*)&eventInfo,
                            sizeof(ClCkptClientUpdInfoT), &eventId);
    }

    /*
     * Delete the masterHdl from old address's peerlist and add to 
     * new address's peerlist.
     */
    if(!CL_CKPT_IS_COLLOCATED(pMasterDBEntry->attrib.creationFlags))
    {
        _ckptPeerListMasterHdlAdd(mastHdl, actAddr,
                                  pMasterDBEntry->activeRepAddr);
    }   
     
    exitOnError:
    {
        /* 
         * Checkin the updated stuff.
         */
        clHandleCheckin(gCkptSvr->masterInfo.masterDBHdl, mastHdl);
        return rc;
    }
}



/*
 * ckptMasterPeerListInfoCreate()
 * - Allocate memory for pPeerInfo. 
 * - create the ckptList.
 * - create the masterHdl list.
 * - Add the info to the peerList.
 * - if it any failure, deallocate the resources and exit. 
 */
 
ClRcT
_ckptMasterPeerListInfoCreate(ClIocNodeAddressT nodeAddr,
                              ClUint32T         credential,
                              ClUint32T         replicaCount)
{
    CkptPeerInfoT  *pPeerInfo = NULL;
    ClRcT          rc         = CL_OK;


    /*
     * Allocate memory for storing peer info.
     */
    if (NULL == (pPeerInfo = (CkptPeerInfoT*) clHeapCalloc(1, 
                    sizeof(CkptPeerInfoT))))
    {
        rc = CL_CKPT_ERR_NO_MEMORY;
        CKPT_DEBUG_E(("PeerInfo No Memory\n")); 
        return rc;
    }

    /*
     * Copy the passed info.
     */
    pPeerInfo->addr         = nodeAddr;
    pPeerInfo->credential   = credential;
    pPeerInfo->available    = CL_CKPT_NODE_AVAIL;
    pPeerInfo->replicaCount = replicaCount;

    /* 
     * Create the list to store the client hdls that
     * will be opened on that node.
     */
    rc = clCntLlistCreate(ckptCkptListKeyComp,
            ckptCkptListDeleteCallback,
            ckptCkptListDeleteCallback,
            CL_CNT_UNIQUE_KEY,
            &pPeerInfo->ckptList);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
            ("CkptList create failed rc[0x %x]\n",rc),
            rc);

    /* 
     * Create the list to store the master hdls for checkpoints that
     * will be created on that node.
     */
    rc = clCntLlistCreate(ckptMastHdlListtKeyComp,
            ckptMastHdlListDeleteCallback,
            ckptMastHdlListDeleteCallback,
            CL_CNT_UNIQUE_KEY,
            &pPeerInfo->mastHdlList);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
            ("MastHdlList create failed rc[0x %x]\n",rc),
            rc);

    if(credential == CL_CKPT_CREDENTIAL_POSITIVE)
        gCkptSvr->masterInfo.availPeerCount++;

    /*
     * Add the node to the master's peer list.
     */
    rc = clCntNodeAdd(gCkptSvr->masterInfo.peerList, (ClPtrT)(ClWordT)nodeAddr,
            (ClCntDataHandleT)pPeerInfo, NULL);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
            ("PeerInfo Add is failed rc[0x %x]\n",rc),
            rc);
    return rc;        

exitOnError:    
    /*
     * Do the necessary cleanup.
     */
    if (pPeerInfo->ckptList != 0)
        clCntDelete(pPeerInfo->ckptList);
    if (pPeerInfo->mastHdlList != 0)
        clCntDelete(pPeerInfo->mastHdlList);
    clHeapFree(pPeerInfo);
    return rc;
}
