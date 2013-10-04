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
/*****************************************************************************
 * ModuleName  : ckpt                                                          
File        : clCkptMaster.c
        clCkptMaster.c $
 ****************************************************************************/

/****************************************************************************
 * Description :                                                                
 *
 *  This file contains CKPT master node functionality.
 *
 *
 ****************************************************************************/

#include <netinet/in.h> 
#include <string.h>
#include "clCommon.h"
#include <clCommonErrors.h>
#include <clEoApi.h>
#include <clTimerApi.h>
#include <clHandleApi.h>
#include <clVersion.h>
#include <ipi/clHandleIpi.h>
#include <clIocErrors.h>

#include <ckptEoServer.h>
#include "clCkptMaster.h"
#include "clCkptPeer.h"
#include "clCkptSvr.h"
#include <clCkptLog.h>
#include "clCkptSvrIpi.h"
#include "clCkptUtils.h"
#include "clCkptMasterUtils.h"

#include <ckptEockptServerMasterActiveClient.h>
#include <ckptEockptServerMasterDeputyServer.h>
#include <ckptEockptServerActivePeerClient.h>
#include <ckptEockptServerPeerPeerServer.h>
#include <ckptEockptServerPeerPeerExtFuncServer.h>
#include "xdrCkptMasterDBInfoIDLT.h"
#include "xdrCkptMasterDBEntryIDLT.h"
#include "xdrCkptMasterDBClientInfoT.h"

static ClUint32T gMastHdlCnt  = 0;
static ClUint32T gClntHdlCnt  = 0;
static ClUint32T gXlationCnt  = 0;
static ClUint32T gPeerListCnt = 0;
static ClUint32T gRepListCnt  = 0;
static ClUint32T gCltEntryCnt = 0;
static ClUint32T gMstEntryCnt = 0;

typedef struct ClCkptMasterSyncupWalk
{
    ClPtrT pData;
    ClUint32T count;
    ClUint32T index;
}ClCkptMasterSyncupWalkT;                                                                                                                             
extern CkptSvrCbT  *gCkptSvr;
static ClTimerHandleT gClPeerReplicateTimer;

void ckptPeerSyncCallback(ClRcT rc , void *pData,
        ClBufferHandleT inMsg,
        ClBufferHandleT outMsg)
{
    ClOsalCondIdT  *pCondVar = (ClOsalCondIdT *)pData;
    clOsalCondSignal(*pCondVar);
}


static ClRcT
clCkptFirstReplicaGet(ClCntHandleT  replicaList,
                      ClIocNodeAddressT  *pNodeAddr)
{
    ClRcT             rc   = CL_OK;
    ClCntKeyHandleT   key  = CL_HANDLE_INVALID_VALUE;
    ClCntNodeHandleT  node = CL_HANDLE_INVALID_VALUE;

    rc = clCntFirstNodeGet(replicaList, &node);
    if( CL_OK != rc )
    {
        return rc;
    }
    rc = clCntNodeUserKeyGet(replicaList, node, &key); 
    if( CL_OK != rc )
    {
        return rc;
    }
    *pNodeAddr = (ClIocNodeAddressT)(ClWordT) key;
    return rc;
}

static ClRcT
clCkptNextReplicaGet(ClCntHandleT  replicaList,
                     ClIocNodeAddressT  *pNodeAddr)
{
    ClRcT             rc   = CL_OK;
    ClCntKeyHandleT   key  = (ClCntKeyHandleT)(ClWordT) *pNodeAddr; 
    ClCntNodeHandleT  node = CL_HANDLE_INVALID_VALUE;
    
    *pNodeAddr = CL_CKPT_UNINIT_ADDR;
    rc = clCntNodeFind(replicaList, key, &node);
    if( CL_OK != rc )
    {
        rc = clCntFirstNodeGet(replicaList, &node);
        if( CL_OK != rc )
        {
            return rc;
        }
    }
    else
    {
        rc = clCntNextNodeGet(replicaList, node, &node); 
        if( CL_OK != rc )
        {
            return rc;
        }
    }
    rc = clCntNodeUserDataGet(replicaList, node, &key);
    if( CL_OK != rc )
    {
        return rc;
    }
    *pNodeAddr = (ClIocNodeAddressT)(ClWordT) key;
    return rc;
}

static ClRcT ckptCloseOpenFailure(ClHandleT         clientHdl, 
                                  ClIocNodeAddressT localAddr)
{
    ClRcT                   rc              = CL_OK;
    CkptPeerInfoT           *pPeerInfo      = NULL;

    /* 
     * Delete the entry from clientDB database 
     */
    rc = clHandleDestroy(gCkptSvr->masterInfo.clientDBHdl,
                         clientHdl); 
    CKPT_ERR_CHECK(CL_CKPT_SVR, CL_LOG_SEV_ERROR,
                   ("MasterOpen ckpt close failed rc[0x %x]\n",rc),
                   rc);

    /*
     * Decrement the client handle count.
     */
    gCkptSvr->masterInfo.clientHdlCount--;        

    /*
     * Delete the client handle from the node's ckptHdl list.
     */
    rc = clCntDataForKeyGet(gCkptSvr->masterInfo.peerList, (ClPtrT)(ClWordT)localAddr,
                            (ClCntDataHandleT *)&pPeerInfo); 
    CKPT_ERR_CHECK(CL_CKPT_SVR, CL_LOG_SEV_ERROR,
                   ("MasterOpen ckpt close: Failed to get info of addr %d in peerList rc[0x %x]\n",
                    localAddr, rc), rc);
    if ( CL_OK != (rc = clCntAllNodesForKeyDelete(pPeerInfo->ckptList,
                                                  (ClPtrT)(ClWordT)clientHdl)))
    {
        CKPT_DEBUG_E(("Master open ckpt close: ClientHdl %#llX Delete from list failed", clientHdl));
    }
                    
    exitOnError:
    return rc;
}

/*
 * Server implementation of checkpoint open functionality.
 */
 
ClRcT VDECL_VER(clCkptMasterCkptOpen, 4, 0, 0)(ClVersionT       *pVersion,
                                               ClCkptSvcHdlT                       ckptSvcHdl,
                                               SaNameT                             *pName,
                                               ClCkptCheckpointCreationAttributesT *pCreateAttr,
                                               ClCkptOpenFlagsT                    ckptOpenFlags,
                                               ClIocNodeAddressT                   localAddr,
                                               ClIocPortT                          localPort, 
                                               CkptHdlDbT                          *pHdlInfo)

{
    ClRcT                   rc              = CL_OK;
    ClUint32T               cksum           = 0;
    ClHandleT               masterHdl       = CL_CKPT_INVALID_HDL;
    ClHandleT               clientHdl       = CL_CKPT_INVALID_HDL;
    CkptMasterDBEntryT      *pStoredData    = NULL;
    CkptXlationDBEntryT     *pStoredDBEntry = NULL;
    ClHandleT               storedDBHdl     = CL_CKPT_INVALID_HDL;
    ClIocNodeAddressT       nodeAddr        = 0;
    ClIocNodeAddressT       activeAddr      = 0; 
    CkptXlationLookupT      lookup          = {0};
    ClVersionT              ckptVersion     = {0};
    CkptPeerInfoT           *pPeerInfo      = NULL;

    CL_FUNC_ENTER();

    /*
     * Check whether the server is fully up or not.
     */
    CL_CKPT_SVR_EXISTENCE_CHECK; 

    /*
     * Verify the version.
     */
    rc = clVersionVerify(&gCkptSvr->versionDatabase,pVersion);
    if( rc != CL_OK)
    {
        return CL_OK; 
    }

    memset(pVersion,'\0',sizeof(ClVersionT));

    /*
     * Check the validity of the checkpoint name.
     */
    CL_CKPT_NAME_VALIDATE(pName);

    /*
     * Lock the master DB.
     */
    CKPT_LOCK(gCkptSvr->masterInfo.ckptMasterDBSem);

    if( (CL_CKPT_CHECKPOINT_CREATE == (ckptOpenFlags & CL_CKPT_CHECKPOINT_CREATE)) && (pCreateAttr == NULL))
    {
        rc = CL_CKPT_RC(CL_ERR_NULL_POINTER);
        CKPT_ERR_CHECK(CL_CKPT_SVR,CL_LOG_SEV_ERROR, ("ckptMasterOpen failed rc[0x %x]\n", rc), rc);
    }

    /* 
     * Check whether the meta data for the given ckpt exists or not.
     */
    memset(&lookup, 0, sizeof(lookup));
    clCksm32bitCompute ((ClUint8T *)pName->value, pName->length, &cksum);
    lookup.cksum = cksum;
    saNameCopy(&lookup.name, pName);
    memcpy( &ckptVersion,
            gCkptSvr->versionDatabase.versionsSupported,
            sizeof(ClVersionT));

    rc = clCntDataForKeyGet(gCkptSvr->masterInfo.nameXlationDBHdl, 
                            (ClCntKeyHandleT)&lookup,
                            (ClCntDataHandleT*)&pStoredDBEntry);

    /* If entry not found then
       1. If the call is for create, then add the entry to both Xlation table 
       and master DB table.
       2. Else return error.
    */
    if(rc != CL_OK)
    {
        /*
         * If call is not for create, return ERROR.
         */
        if(!(ckptOpenFlags & CL_CKPT_CHECKPOINT_CREATE))
        {
            rc = CL_CKPT_RC(CL_ERR_NOT_EXIST);
            /* Note, this error is only at info level because it is correct
               programming practice for clients to first attempt to open the
               checkpoint and then create it if an error is returned. */
            CKPT_ERR_CHECK(CL_CKPT_SVR,CL_LOG_SEV_INFO,
                           ("Checkpoint open failed, checkpoint does not exist.  rc[0x %x].  This is expected if the app is checking to see if the checkpoint was created by another node.", rc), rc);
        } 
        else
        {
            /* 
             * Check Whether the application is running on our cluster.
             */  
            rc = clCntDataForKeyGet(gCkptSvr->masterInfo.peerList,
                                    (ClPtrT)(ClWordT)localAddr, (ClCntDataHandleT*)&pPeerInfo);
            CKPT_ERR_CHECK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
                           ("ckptMasterOpen failed rc[0x %x]\n",
                            rc), rc);

            /* 
             * Check if the node has checkpoint server running or not.
             */
            if(pPeerInfo->available == CL_CKPT_NODE_UNAVAIL)
            {
                /*
                 * Ckpt welcome has to be received and peer instantiated
                 * by the master for collocated or hot-standby checkpoints
                 */
                if(CL_CKPT_IS_COLLOCATED(pCreateAttr->creationFlags)
                   ||
                   (pCreateAttr->creationFlags & CL_CKPT_DISTRIBUTED))
                {
                    rc = CL_CKPT_ERR_TRY_AGAIN;
                    clLogError(CL_CKPT_AREA_MASTER, CL_CKPT_CTX_CKPT_OPEN, 
                               "Ckpt peer [%d] hasn't been welcomed by master."
                               "Forcing a try again in case remote welcome messages are delayed",
                               localAddr);
                    goto exitOnError;
                }

                /*
                 * Choose a node from the cluster which has 
                 * checkpoint server running 
                 */
                rc = ckptReplicaNodeGet( CL_CKPT_UNINIT_ADDR, &localAddr);
                if( localAddr == CL_CKPT_UNINIT_ADDR )
                {
                    rc = CL_CKPT_ERR_NOT_EXIST;
                    clLogError(CL_CKPT_AREA_MASTER, CL_CKPT_CTX_CKPT_OPEN, 
                               "No checkpoint server is available to store data rc [0x %x]", rc); 
                    goto exitOnError;
                }
            }
            CL_ASSERT( localAddr != CL_CKPT_UNINIT_ADDR );

            /*
             * Create masterHdl for the checkpoint.
             */
            rc = clHandleCreate(gCkptSvr->masterInfo.masterDBHdl, 
                                sizeof(CkptMasterDBEntryT),
                                &masterHdl);
            CKPT_ERR_CHECK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
                           ("MasterHdl create err rc[0x %x]\n",
                            rc), rc);
            clLogDebug(CL_CKPT_AREA_MASTER, CL_CKPT_CTX_CKPT_OPEN, 
                       "Master handle [%#llX] has been created", 
                       masterHdl);

            if(localAddr != gCkptSvr->localAddr)  
            {
                /*
                 * Create the checkpoint at the specified Address.
                 */
                rc = ckptIdlHandleUpdate(localAddr,gCkptSvr->ckptIdlHdl,
                                         0);
                CKPT_ERR_CHECK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
                               ("ckptMasterOpen failed rc[0x %x]\n",
                                rc), rc);
                rc = VDECL_VER(clCkptActiveCkptOpenClientSync, 4, 0, 0)( gCkptSvr->ckptIdlHdl,
                                                                         &ckptVersion,
                                                                         masterHdl,
                                                                         pName,
                                                                         ckptOpenFlags,
                                                                         pCreateAttr, 
                                                                         localAddr, 
                                                                         localPort);

                /*
                 * Check for version mismatch.
                 */
                if( (ckptVersion.releaseCode != '\0') && (rc == CL_OK))
                {
                    rc = CL_CKPT_ERR_VERSION_MISMATCH;
                    clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL,
                               CL_CKPT_LOG_6_VERSION_NACK, "CkptOpen",
                               localAddr,
                               CL_IOC_CKPT_PORT, ckptVersion.releaseCode,
                               ckptVersion.majorVersion, 
                               ckptVersion.minorVersion);

                    clLogError(CL_CKPT_AREA_MASTER, CL_CKPT_CTX_CKPT_OPEN,"Ckpt nack recieved from "
                                                    "NODE[0x%x:0x%x], rc=[0x%x]\n", 
                                                    localAddr, 
                                                    CL_IOC_CKPT_PORT,
                                                    CL_CKPT_ERR_VERSION_MISMATCH);
                }
            }
            else
            {
                /*
                 * Update locally.
                 */
                rc = _ckptLocalDataUpdate(masterHdl,
                                          pName,
                                          pCreateAttr,
                                          cksum, 
                                          localAddr, 
                                          localPort);
            }
            if( CL_OK != rc )
            {
                /* 
                 * Checkpoint creation failed.
                 * Delete the created handle.
                 */
                clHandleDestroy(gCkptSvr->masterInfo.masterDBHdl, masterHdl);
            }
            CKPT_ERR_CHECK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
                           ("ckptMasterOpen failed,  rc[0x %x]\n",
                            rc), rc);

            /* 
             * Associate checkpoint info with the master hdl.
             */
            if (CL_OK != (rc = _ckptMasterHdlInfoFill(masterHdl, pName,
                                                      pCreateAttr, localAddr,
                                                      CL_CKPT_SOURCE_MASTER,
                                                      &activeAddr)))
            {
                CKPT_UNLOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
                return rc;
            }

            /* 
             * Add corresponding entry to Xlation table.
             */
            if (CL_OK != (rc = _ckptMasterXlationDBEntryAdd(pName, cksum,
                                                            masterHdl)))
            {
                clHandleDestroy(gCkptSvr->masterInfo.masterDBHdl,masterHdl);
                CKPT_UNLOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
                return rc;
            }

            /* 
             * Create the client handle and add the client info.
             */
            if( CL_OK != (rc = clHandleCreate(
                                              gCkptSvr->masterInfo.clientDBHdl,
                                              sizeof(CkptMasterDBClientInfoT),
                                              &clientHdl)))
            {
                clCntAllNodesForKeyDelete(
                                          gCkptSvr->masterInfo.nameXlationDBHdl,
                                          (ClCntKeyHandleT)&lookup);
                clHandleDestroy(gCkptSvr->masterInfo.masterDBHdl,masterHdl);
                CKPT_UNLOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
                CKPT_DEBUG_E(("client Handle create err: rc[0x%x]\n",rc));
                return rc;
            }

            if (CL_OK != (rc = _ckptClientHdlInfoFill(masterHdl, clientHdl, 
                                                      CL_CKPT_SOURCE_MASTER)))
            {
                clCntAllNodesForKeyDelete(
                                          gCkptSvr->masterInfo.nameXlationDBHdl,
                                          (ClCntKeyHandleT)&lookup);
                clHandleDestroy(gCkptSvr->masterInfo.masterDBHdl,masterHdl);
                CKPT_UNLOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
                return rc;
            }

            /* 
             * Add the handle to the list of opens on that node.
             */
            if (CL_OK != (rc = _ckptMasterPeerListHdlsAdd(clientHdl, 
                                                          masterHdl,
                                                          localAddr,
                                                          localPort,
                                                          CL_CKPT_CREAT,
                                                          pCreateAttr->creationFlags)))
            {
                clHandleDestroy(gCkptSvr->masterInfo.clientDBHdl,
                                clientHdl);
                clCntAllNodesForKeyDelete(
                                          gCkptSvr->masterInfo.nameXlationDBHdl,
                                          (ClCntKeyHandleT)&lookup);
                clHandleDestroy(gCkptSvr->masterInfo.masterDBHdl,masterHdl);
                CKPT_UNLOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
                return rc;
            }

            /* 
             * Update the backup/deputy/secondary master.
             */
            {
                ckptIdlHandleUpdate( CL_IOC_BROADCAST_ADDRESS,
                                     gCkptSvr->ckptIdlHdl,0);
                VDECL_VER(clCkptDeputyCkptCreateClientAsync, 4, 0, 0)(gCkptSvr->ckptIdlHdl,
                                                                      masterHdl, clientHdl,
                                                                      pName, pCreateAttr,
                                                                      localAddr, localPort,
                                                                      &ckptVersion,
                                                                      NULL,NULL);
            } 

            /* 
             *  If the checkpoint is not CL_CKPT_DISTRIBUTED, then replicate
             *  coz, this flags specifies the reader application should open
             *  the checkpoint, so we will create the replica on where the
             *  standby application runs.
             *  Removed Collocation flag check, as we need to ensure that we 
             *  are having two replicas for a checkpoint at any point of the
             *  time. 
             */
            if( (!(pCreateAttr->creationFlags & CL_CKPT_DISTRIBUTED))
                &&
                !(CKPT_COLLOCATE_REPLICA_ON_OPEN(gCkptSvr->replicationFlags)
                  && 
                  CL_CKPT_IS_COLLOCATED(pCreateAttr->creationFlags)) )
            {
                /* 
                 * Choose a node for storing the checkpoint replica.
                 */
                nodeAddr = localAddr; 
                rc = ckptReplicaNodeGet(CL_CKPT_UNINIT_ADDR,&nodeAddr);
                if(nodeAddr != CL_CKPT_UNINIT_ADDR)
                {
                    /* 
                     * Ask the chosen node to pull the ckptInfo 
                     * from active replica.No need of error checking,
                     * even it fails,checkpoint server can proceed this 
                     * operation.
                     */
                    clLogDebug(CL_CKPT_AREA_MASTER, CL_CKPT_CTX_CKPT_OPEN, 
                               "Notifying address [%d] to pull info from [%d] for handle [%#llX]", 
                               nodeAddr, localAddr, masterHdl);
                    _ckptReplicaIntimation( nodeAddr, localAddr,
                                            masterHdl, CL_CKPT_RETRY, 
                                            !CL_CKPT_ADD_TO_MASTHDL_LIST);
                }
            }
        }  
        pHdlInfo->ckptActiveHdl       = masterHdl;
        pHdlInfo->activeAddr          = activeAddr;
        pHdlInfo->creationFlag        = pCreateAttr->creationFlags;

        clLogInfo("CKP","MSTR","Created Checkpoint [%.*s]  client handle [%llx] active handle [%llx] active addr [%d]", pName->length,pName->value,clientHdl, masterHdl,activeAddr);
        
    }
    else /* Entry found in the Xlation table */
    {          
        /*
         * Checkpoint already exists.
         */
        storedDBHdl = pStoredDBEntry->mastHdl;
        rc = clHandleCheckout(gCkptSvr->masterInfo.masterDBHdl, storedDBHdl,(void **) &pStoredData);  
        CKPT_ERR_CHECK(CL_CKPT_SVR,CL_LOG_SEV_ERROR, ("Master DB creation failed rc[0x %x]\n",rc), rc);

        /*
         */
        if( ckptOpenFlags & CL_CKPT_CHECKPOINT_CREATE)
        {
            /* 
             * Match the creation attributes. Incase of mismatch throw ERROR.
             */
            if((pStoredData->attrib.creationFlags    != 
                pCreateAttr->creationFlags)      ||  
               (pStoredData->attrib.checkpointSize   != 
                pCreateAttr->checkpointSize)     ||
               (pStoredData->attrib.maxSections      != 
                pCreateAttr->maxSections)        || 
               (pStoredData->attrib.maxSectionSize   != 
                pCreateAttr->maxSectionSize)     || 
               (pStoredData->attrib.maxSectionIdSize != 
                pCreateAttr->maxSectionIdSize)   || 
               (pStoredData->attrib.retentionDuration!= 
                pCreateAttr->retentionDuration)) 
            {
                /* 
                 * ERROR: Ckpt already exists 
                 */
                clHandleCheckin(gCkptSvr->masterInfo.masterDBHdl, 
                                storedDBHdl);
                rc = CL_CKPT_ERR_ALREADY_EXIST;
                CKPT_ERR_CHECK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
                               ("Master DB creation failed: "
                                "Ckpt already exists, rc[0x %x]\n",rc),
                               rc);
            }
        }

        /* 
         * Create the client handle.Add the info.
         */
        if( CL_OK != (rc = clHandleCreate(
                                          gCkptSvr->masterInfo.clientDBHdl,
                                          sizeof(CkptMasterDBClientInfoT),
                                          &clientHdl)))
        {
            clHandleCheckin(gCkptSvr->masterInfo.masterDBHdl, 
                            storedDBHdl);
            CKPT_UNLOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
            CKPT_DEBUG_E(("client Handle create err: rc[0x %x]\n",rc));
            return rc;
        }
        clLogDebug("CKP","MSTR","Created ckpt client handle [%llx]", clientHdl);        
        if (CL_OK != (rc = _ckptClientHdlInfoFill(storedDBHdl, clientHdl, CL_CKPT_SOURCE_MASTER)))
        {
            clHandleCheckin(gCkptSvr->masterInfo.masterDBHdl, storedDBHdl);
            CKPT_UNLOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
            return rc;                
        }

        /*
         * Update the info in the ckptList about the handles.
         */
        if (CL_OK != (rc = _ckptMasterPeerListHdlsAdd(clientHdl, storedDBHdl,
                                                      localAddr, localPort,
                                                      CL_CKPT_OPEN, 0)))
        {
            clHandleDestroy(gCkptSvr->masterInfo.clientDBHdl,
                            clientHdl);
            --gCkptSvr->masterInfo.clientHdlCount;
            clHandleCheckin(gCkptSvr->masterInfo.masterDBHdl, 
                            storedDBHdl);
            CKPT_UNLOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
            return rc;                
        }
        pHdlInfo->creationFlag = pStoredData->attrib.creationFlags; 
        activeAddr = pStoredData->activeRepAddr;
        pHdlInfo->ckptActiveHdl = pStoredData->activeRepHdl;

        /*
         * Increment the checkpoint ref count.
         */
        pStoredData->refCount = pStoredData->refCount+1;

        clLogInfo("CKP","MSTR","Opened Checkpoint [%.*s]. Reference count [%d] client handle [0x%llx] active handle [0x%llx] active addr [%d]", pName->length,pName->value,pStoredData->refCount,clientHdl, masterHdl,activeAddr);

        /* 
         * Stop/Destroy the retention timer if runing.
         */
        if(pStoredData->retenTimerHdl != 0)
        {
            clTimerDelete(&pStoredData->retenTimerHdl);
        }
        clHandleCheckin(gCkptSvr->masterInfo.masterDBHdl, storedDBHdl);

        /* 
         * Update the backup/deputy/secondary master.
         */
        {
            clLogDebug(CL_CKPT_AREA_MASTER, CL_CKPT_CTX_CKPT_OPEN, "Updating deputy for checkpoint [%.*s] clientHdl [%#llX] MasterHdl [%#llX]", pName->length,pName->value,clientHdl, storedDBHdl);
            ckptIdlHandleUpdate( CL_IOC_BROADCAST_ADDRESS, gCkptSvr->ckptIdlHdl,0);
            VDECL_VER(clCkptDeputyCkptOpenClientAsync, 4, 0, 0)(gCkptSvr->ckptIdlHdl,
                                                                storedDBHdl, clientHdl, localAddr,
                                                                localPort, &ckptVersion,
                                                                NULL,NULL);
        }    
        /* 
         *  Inform about application Info to all the replicas of this checkpoint
         */
        clCkptAppInfoReplicaNotify(pStoredData, storedDBHdl, localAddr, localPort);
        /*
         * If the checkpoint flags is having CL_CKPT_DISTRIBUTED/ COLLOCATED    
         * or CL_CKPT_ALL_OPEN_ARE_REPLICAS, then replicate the info on this local node 
         * also for hotstanby and 1 + N redundancy requirements
         */
        if(  (pStoredData->attrib.creationFlags & CL_CKPT_DISTRIBUTED)  ||
             (CL_CKPT_IS_COLLOCATED(pStoredData->attrib.creationFlags)) ||
             (pStoredData->attrib.creationFlags & CL_CKPT_ALL_OPEN_ARE_REPLICAS)  )
             
        {
            /* 
             * Choose a node for storing the checkpoint replica.
             */
            if(localAddr != CL_CKPT_UNINIT_ADDR)
            {
                ClCntNodeHandleT node = 0;
                ClCntKeyHandleT key = (ClCntKeyHandleT)(ClWordT)localAddr;
                /*
                 * First check if the ckpt is already replicated in localAddr
                 */
                rc = clCntNodeFind(pStoredData->replicaList, key, &node);
                if(rc != CL_OK)
                {

                    rc = clCkptFirstReplicaGet(pStoredData->replicaList, &nodeAddr);
                    if( CL_OK != rc)
                    {
                        /* GAS: this code does not make sense... At its most basic, shouldn't pStoredData->refCount be set to 0 */
                        clLogError(CL_CKPT_AREA_MASTER, CL_CKPT_CTX_CKPT_OPEN, "No replicas available for this checkpoint rc [0x%x]", rc);
                        ckptCloseOpenFailure(clientHdl, localAddr);
                        --pStoredData->refCount;
                        clLogNotice("CKP","MGT","Checkpoint [%s] reference count decremented. Now [%d].", pStoredData->name.value, pStoredData->refCount);
                        rc = CL_CKPT_ERR_NO_RESOURCE;
                        goto exitOnError;
                    }
                    /* 
                     * Ask the chosen node to pull the ckptInfo 
                     * from active replica.No need of error checking,
                     * even it fails,checkpoint server can proceed this 
                     * operation.
                     */
                    clLogDebug(CL_CKPT_AREA_MASTER, CL_CKPT_CTX_CKPT_OPEN, "Notifying address [%d] to pull info for Checkpoint [%.*s] (handle [%#llX]) from [%d]", localAddr, pStoredData->name.length, pStoredData->name.value, storedDBHdl, nodeAddr);
                    clCkptReplicaCopy(localAddr, nodeAddr, storedDBHdl, clientHdl, activeAddr, pHdlInfo);
                }
                else
                {
                    clLogNotice(CL_CKPT_AREA_MASTER, CL_CKPT_CTX_CKPT_OPEN,
                                "Checkpoint [%.*s] is already replicated on opening node [%d]",
                                pStoredData->name.length, pStoredData->name.value,
                                localAddr);
                }
            }
        }
    }
    pHdlInfo->ckptMasterHdl  = clientHdl;
    pHdlInfo->activeAddr     = activeAddr;
    exitOnError:
    {
        /*
         * Unlock the master DB.
         */
        CKPT_UNLOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
        return rc;
    }
}

/*
 * This function will be invoked after the the selected replica node
 * has pulled the checkpoint from the active server.
 */ 
 
void _ckptReplicaNotifyCallback( ClIdlHandleT       ckptIdlHdl,
                                 ClVersionT         *pVersion,
                                 ClHandleT          ckptActHdl,
                                 ClIocNodeAddressT  activeAddr,
                                 ClRcT              rc,
                                 void               *pData)
{
   
    CkptMasterDBEntryT      *pMasterDBEntry = NULL;
    CkptPeerInfoT           *pPeerInfo      = NULL;
    ClCkptReplicateInfoT    *pRepInfo       = (ClCkptReplicateInfoT *) pData;
    ClIocNodeAddressT        peerAddr       = 0; 
    ClRcT                   retCode         = rc;

    if( NULL == pRepInfo )
    {
        clLogCritical(CL_CKPT_AREA_MASTER, CL_CKPT_CTX_REPL_NOTIFY, 
                "Didn't get the proper cookie value, returning...");
        return ;
    }
    peerAddr = pRepInfo->destAddr;
    clLogDebug(CL_CKPT_AREA_MASTER, CL_CKPT_CTX_REPL_NOTIFY,
               "Notify callbacks RetCode [%x] tryOtherNode [%d]"
               "peerAddr [%d]", rc, pRepInfo->tryOtherNode, peerAddr);
    if( gCkptSvr == NULL || gCkptSvr->serverUp == CL_FALSE)
    {
        clLogError(CL_CKPT_AREA_MASTER, CL_CKPT_CTX_REPL_NOTIFY,"Checkpoint Server is not up\n");
        
        /*
         * Free the cookie.
         */
        if(pRepInfo != NULL)
            clHeapFree(pRepInfo);
        return ;
    }

    if(CL_RMD_TIMEOUT_UNREACHABLE_CHECK(rc))
    {
        /*
         * Free the cookie.
         */
        if(pRepInfo != NULL)
            clHeapFree(pRepInfo);
        return ;
    }

    /*
     * Version verification.
     */
    if( (pVersion->releaseCode != '\0' && rc == CL_OK))
    {
        clLogError(CL_CKPT_AREA_MASTER, CL_CKPT_CTX_REPL_NOTIFY,"While Replicating info"
                    "version mismatch occured");
        if(pRepInfo != NULL)
            clHeapFree(pRepInfo);
        return;
    }

    /*
     * Lock the master BD.
     */
    CKPT_LOCK(gCkptSvr->masterInfo.ckptMasterDBSem);

    /*
     * Checkout the checkpoint related metadata.
     */
    rc = ckptSvrHdlCheckout( gCkptSvr->masterInfo.masterDBHdl,
                             ckptActHdl,
                             (void **)&pMasterDBEntry);
    CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
            ("Failed to add replica to replica list[0x %x]\n",rc),
            rc);
    /*
     * In case the pulling of ckpt information failed and if retry flag
     * is set, select a new replica node and initiate the replication 
     * process wih the newly selected node.
     */
    if((retCode != CL_OK) && (pRepInfo->tryOtherNode == CL_CKPT_RETRY))
    {
        /*
         * Failed to update the info in peerAddr.
         * Choose some other node from the peerList.
         */
        rc = clCkptNextReplicaGet(pMasterDBEntry->replicaList, &activeAddr);
        /*
         * Start the replication process with the newly selected node.
         */
        if( (activeAddr != CL_CKPT_UNINIT_ADDR) && (activeAddr != peerAddr) )
        {
            rc = _ckptReplicaIntimation(peerAddr, activeAddr,
                                        ckptActHdl, pRepInfo->tryOtherNode,
                                        pRepInfo->addToList); 
            if( CL_OK != rc )
            {
                clLogError(CL_CKPT_AREA_MASTER, CL_CKPT_CTX_REPL_NOTIFY,
                           "While updating replica on peerAddr [%d] from"
                           "active addr [%d] failed rc [0x %x]", peerAddr,
                           activeAddr, rc);
            }
        }
        else
        {
            clLogError(CL_CKPT_AREA_MASTER, CL_CKPT_CTX_REPL_NOTIFY, 
                       "Could not find next replica for this ckpt [%.*s]"
                       "replication to [%d] didn't happen from [%d]",
                        pMasterDBEntry->name.length,
                        pMasterDBEntry->name.value, peerAddr, activeAddr);
        }
        /*
         * Checkin the checkpoint related metadata.
         */
         clHandleCheckin( gCkptSvr->masterInfo.masterDBHdl, ckptActHdl);
        
        /*
         * Unlock the master DB.
         */
        CKPT_UNLOCK(gCkptSvr->masterInfo.ckptMasterDBSem);

        /*
         * Free the cookie.
         */
        if(pRepInfo != NULL)
            clHeapFree(pRepInfo);
        return ;
    }
    /*
     * Add the replica address to the replica list of the checkpoint. 
     */         
     clLogDebug(CL_CKPT_AREA_MASTER, CL_CKPT_CTX_REPL_NOTIFY, 
		"Checkpoint [%.*s] has been replicated on [%d]", 
		pMasterDBEntry->name.length, pMasterDBEntry->name.value, 
		peerAddr);
    rc = clCntNodeAdd( pMasterDBEntry->replicaList,
            (ClCntKeyHandleT)(ClWordT)peerAddr, 
            (ClCntDataHandleT)(ClWordT)peerAddr, NULL);
    if (CL_GET_ERROR_CODE(rc) == CL_ERR_DUPLICATE)   rc = CL_OK;

    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
            ("Failed to add replica to replica list[0x %x]\n",rc),
            rc);
            
    /*          
     * Increment the replica count.
     */
    rc = clCntDataForKeyGet( gCkptSvr->masterInfo.peerList,
            (ClCntDataHandleT)(ClWordT)peerAddr,
            (ClCntDataHandleT *)&pPeerInfo);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
            (" PeerInfo get failed rc[0x %x]\n",rc),
            rc);
    pPeerInfo->replicaCount++;
    /*
     * Update the deputy
     */
    {

         rc = ckptIdlHandleUpdate(CL_IOC_BROADCAST_ADDRESS, 
                                  gCkptSvr->ckptIdlHdl, 0);
         CKPT_ERR_CHECK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
            (" Failed to update the handle rc[0x %x]\n",rc), rc);

         memcpy( pVersion, gCkptSvr->versionDatabase.versionsSupported, 
             sizeof(ClVersionT));
 
         rc = VDECL_VER(clCkptDeputyReplicaListUpdateClientAsync, 4, 0, 0)(gCkptSvr->ckptIdlHdl,
                                        pVersion,
                                        ckptActHdl,
                                        peerAddr,
                                        pRepInfo->addToList,
                                        NULL,NULL);
         CKPT_ERR_CHECK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
                        (" Failed to update deputy  rc[0x %x]\n",rc), rc);
    }
exitOnError:
    /*
     * Checkin the checkpoint related metadata.
     */
    clHandleCheckin( gCkptSvr->masterInfo.masterDBHdl, ckptActHdl);
exitOnErrorBeforeHdlCheckout:
    /*
     * Unlock the master DB.
     */
    CKPT_UNLOCK(gCkptSvr->masterInfo.ckptMasterDBSem);

    /*
     * Free the cookie.
     */
    if(pRepInfo != NULL)
        clHeapFree(pRepInfo);
    return;
}



/*
 * This function informs the node to pull the checkpoint from the 
 * active replica.
 */
 
ClRcT _ckptReplicaIntimation(ClIocNodeAddressT destAddr,
                             ClIocNodeAddressT activeAddr,
                             ClHandleT         activeHdl,
                             ClBoolT           tryOtherNode,
                             ClUint32T         addToList)
{
   ClRcT                rc          = CL_OK;
   ClVersionT           ckptVersion = {0};
   ClCkptReplicateInfoT *pRepInfo   = NULL;
   
   CKPT_NULL_CHECK(gCkptSvr);
   
   /*
    * Update the idl handle with the destination address.
    */
   rc = ckptIdlHandleUpdate( destAddr, gCkptSvr->ckptIdlHdl, 2);
   CKPT_ERR_CHECK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
           ("ckptMasterOpen failed rc[0x %x]\n",
            rc), rc);

   /*
    * Set the supporeted version.
    */
   memcpy(&ckptVersion,
          gCkptSvr->versionDatabase.versionsSupported,
          sizeof(ClVersionT));

   /*
    * Fill the cookie to be passed to the callback.
    */
   pRepInfo = (ClCkptReplicateInfoT *) clHeapCalloc(1,
                                        sizeof(ClCkptReplicateInfoT));
   if(pRepInfo == NULL)
   {
       rc = CL_CKPT_ERR_NO_MEMORY;
       return rc;
   }
   pRepInfo->destAddr     = destAddr;
   pRepInfo->tryOtherNode = tryOtherNode;
   pRepInfo->addToList    = addToList;
   
   /*
    *  Send the RMD.
    */ 
   rc = VDECL_VER(clCkptReplicaNotifyClientAsync, 4, 0, 0)( gCkptSvr->ckptIdlHdl,
           &ckptVersion,
           activeHdl,
           activeAddr,
           _ckptReplicaNotifyCallback,
           (void *)pRepInfo);
   CKPT_ERR_CHECK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
           ("Failed to update the replica [0x %x]\n",rc),
           rc);
exitOnError:
   return rc;
}



/*
 * Function to get active replica address for a given checkpoint.
 */
 
ClRcT VDECL_VER(clCkptMasterActiveAddrGet, 4, 0, 0)(ClVersionT        *pVersion,
                                ClHandleT         masterHdl,
                                ClIocNodeAddressT *pNodeAddr)
{
    ClRcT               rc           = CL_OK;
    CkptMasterDBEntryT  *pStoredData = NULL;

    /*
     * Check whether the server is fully up or not.
     */
    CL_CKPT_SVR_EXISTENCE_CHECK;
    
    /*
     * Verify the version.
     */
    rc = clVersionVerify(&gCkptSvr->versionDatabase,pVersion);
    if(rc != CL_OK)
    {   
        return CL_OK;
    }
    
    /*
     * Lock the mutex.
     */
    CKPT_LOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
    
    memset(pVersion,'\0',sizeof(ClVersionT));
    
    /*
     * Retieve the data associated with the master handle.
     * Copy the active address. In case of ERROR copy CL_CKPT_UNINIT_ADDR.
     */
    rc = clHandleCheckout(gCkptSvr->masterInfo.masterDBHdl,
                          masterHdl, (void **)&pStoredData);
    if(rc == CL_OK)
    {
        if(pStoredData != NULL)
        {
            *pNodeAddr = pStoredData->activeRepAddr;
            clLogDebug("MAS", "ACT", "Active address for handle [%#llx] is [%#x]", 
                       masterHdl, pStoredData->activeRepAddr);
        }
        else
        {
            *pNodeAddr = CL_CKPT_UNINIT_ADDR;
        }
        clHandleCheckin(gCkptSvr->masterInfo.masterDBHdl,
                        masterHdl);
    }
    else
    {
        *pNodeAddr = CL_CKPT_UNINIT_ADDR;
    }
    
    /*
     * Unlock the mutex.
     */
    CKPT_UNLOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
    
    return rc;
}



/*
 * Server side implementation of active replica set.
 */
 
ClRcT VDECL_VER(clCkptMasterActiveReplicaSet, 4, 0, 0)(ClCkptHdlT           clientHdl,
                                   ClIocNodeAddressT    localAddr,
                                   ClVersionT           *pVersion)
{
    ClRcT                   rc              = CL_OK;         
    CkptMasterDBClientInfoT *pClientEntry   = NULL;
    ClHandleT               masterHdl       = CL_CKPT_INVALID_HDL;
    ClIocNodeAddressT       prevActiveAddr  = 0;
    CkptMasterDBEntryT      *pStoredData    = NULL;
    SaNameT                 name            = {0};
    ClCkptClientUpdInfoT    eventInfo       = {0};
    ClEventIdT              eventId         = 0;

    /*
     * Check whether the server is fully up or not.
     */
    CL_CKPT_SVR_EXISTENCE_CHECK;
    
    /*
     * Verify the version.
     */
    rc = clVersionVerify(&gCkptSvr->versionDatabase,(ClVersionT *)pVersion);
    if( rc != CL_OK)
    {
        return CL_OK; 
    }

    memset(pVersion,'\0',sizeof(ClVersionT));

    /*
     * Lock the master DB.
     */
    CKPT_LOCK(gCkptSvr->masterInfo.ckptMasterDBSem);

    clLogDebug(CL_CKPT_AREA_MASTER, CL_CKPT_CTX_ACTADDR_SET,
	       "Received notification to set active address to [%d]", 
	       localAddr);
    /*
     * Get the master handle from the client handle info.
     */
    rc = clHandleCheckout(gCkptSvr->masterInfo.clientDBHdl,
            clientHdl, 
            (void **)&pClientEntry);
    CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
            (" MasterActiveReplicaSet failed rc[0x %x]\n",rc),
            rc);
    masterHdl = pClientEntry->masterHdl;
    rc = clHandleCheckin(gCkptSvr->masterInfo.clientDBHdl,
            clientHdl);
    CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
            ("MasterActiveReplicaSet failed rc[0x %x]\n",rc),
            rc);

    /*
     * Retrieve the data associated with the master handle.
     */
    rc = clHandleCheckout(gCkptSvr->masterInfo.masterDBHdl, 
            masterHdl, (void **)&pStoredData);  
    CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
            ("MasterActiveReplicaSet failed: rc[0x %x]\n",rc),
            rc);

    /*
     * Return ERROR if checkpoint is not non collocated.
     */
    if (! CL_CKPT_IS_COLLOCATED(pStoredData->attrib.creationFlags))
    {
        rc = CL_CKPT_ERR_BAD_OPERATION;
        CKPT_ERR_CHECK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
                ("MasterActiveReplicaSet failed rc[0x %x]\n",rc),
                rc);
    }

    /*
     * In case the active replica address is same as the requesting node
     * address, just return.
     */
    if( pStoredData->activeRepAddr == localAddr)
    {
        clLogDebug(CL_CKPT_AREA_MASTER, CL_CKPT_CTX_ACTADDR_SET, 
		   "No change in address, it has aready been set [%d]", localAddr);
        clHandleCheckin(gCkptSvr->masterInfo.masterDBHdl, 
                masterHdl);
        CKPT_UNLOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
        return rc;
    }

    /*
     * Update the active address. Store the old replica address as well.
     */
    prevActiveAddr = pStoredData->activeRepAddr;
    pStoredData->prevActiveRepAddr = prevActiveAddr;
    saNameCopy(&name, &pStoredData->name);
    pStoredData->activeRepAddr = localAddr;

    /*
     * remove MasterHdl from oldAddr and add to the newAddr.
     */

    _ckptPeerListMasterHdlAdd(masterHdl, prevActiveAddr, localAddr);        

    /* 
     * Publish event to inform the clients about change in active replica.
     */
    memset(&eventInfo, 0, sizeof(ClCkptClientUpdInfoT));
    saNameCopy(&eventInfo.name, &name);
    eventInfo.name.length = htons(name.length);
    eventInfo.eventType   = htonl(CL_CKPT_ACTIVE_REP_CHG_EVENT);
    eventInfo.actAddr     = htonl(localAddr);

    clLogInfo(CL_CKPT_AREA_MASTER, CL_CKPT_CTX_ACTADDR_SET, 
	      "Changing the active address from [%d] to [%d] for checkpoint [%.*s]", 
	      prevActiveAddr, localAddr, name.length, name.value);
    rc = clEventPublish(gCkptSvr->clntUpdEvtHdl, (const void*)&eventInfo,
            sizeof(ClCkptClientUpdInfoT), &eventId);
            
    /* 
     * Update the backup/deputy/secondary master 
     */
    {
        CkptUpdateInfoT   updateInfo = {0};

        updateInfo.discriminant = CKPTUPDATEINFOTACTIVEADDR;
        updateInfo.ckptUpdateInfoT.activeAddr = localAddr;

        rc = ckptDynamicDeputyUpdate(CL_CKPT_COMM_UPDATE,
                CL_CKPT_UNINIT_VALUE,
                masterHdl,
                &updateInfo);
        CKPT_ERR_CHECK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
                ("Failed to update deputy master, rc[0x %x]\n",rc),rc);
    }
exitOnError:
    {
        /*
         * Checkin the changes made to master DB.
         */
        clHandleCheckin(gCkptSvr->masterInfo.masterDBHdl, 
                masterHdl);
    }                       
exitOnErrorBeforeHdlCheckout:
    {
        /*
         * Unlock the master DB.
         */
        CKPT_UNLOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
        return rc;
    }
}



/*
 * Server side implementation of active replica switchover.
 */
 
ClRcT VDECL_VER(clCkptMasterActiveReplicaSetSwitchOver, 4, 0, 0)(ClCkptHdlT         clientHdl,
                                             ClIocNodeAddressT  localAddr,
                                             ClVersionT         *pVersion)
{
    return VDECL_VER(clCkptMasterActiveReplicaSet, 4, 0, 0)(clientHdl, localAddr, pVersion);
    /*
     * Obseleting active replica set switchover now and falling back to active replica set
     * as with multiple SCs/deputy, we may have to update the active replica for the collocated
     * ckpt on all of them.
     */
#if 0
    ClRcT                   rc              = CL_OK;         
    CkptMasterDBClientInfoT *pClientEntry   = NULL;
    ClHandleT               masterHdl       = CL_CKPT_INVALID_HDL;
    ClIocNodeAddressT       prevActiveAddr  = {0};
    CkptMasterDBEntryT      *pStoredData    = NULL;
    SaNameT                 name            = {0};
    ClCkptClientUpdInfoT    eventInfo       = {0};
    ClEventIdT              eventId         = {0};
    ClVersionT              ckptVersion     = {0};

    /*
     * Check whether the server is fully up or not.
     */
    CL_CKPT_SVR_EXISTENCE_CHECK;
    
    /*
     * Verify the version.
     */
    rc = clVersionVerify(&gCkptSvr->versionDatabase,(ClVersionT *)pVersion);
    if( rc != CL_OK)
    {
        return CL_OK; 
    }
    
    memset(pVersion,'\0',sizeof(ClVersionT));
    
    /*
     * Lock the master DB.
     */
    CKPT_LOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
    
    clLogDebug(CL_CKPT_AREA_MASTER, CL_CKPT_CTX_ACTADDR_SET,
               "Active address switch over for open handle [%#llX]",
               clientHdl);
    /*
     * Get the master handle from the client handle info.
     */
    rc = clHandleCheckout(gCkptSvr->masterInfo.clientDBHdl,
            clientHdl, 
            (void **)&pClientEntry);
    CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
            (" MasterActiveReplicaSet failed rc[0x %x]\n",rc),
            rc);
    masterHdl = pClientEntry->masterHdl;
    rc = clHandleCheckin(gCkptSvr->masterInfo.clientDBHdl,
            clientHdl);
    CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
            ("MasterActiveReplicaSet failed rc[0x %x]\n",rc),
            rc);

    /*
     * Retieve the data associated with the master handle.
     */
    rc = clHandleCheckout(gCkptSvr->masterInfo.masterDBHdl, 
            masterHdl, (void **)&pStoredData);  
    CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
            ("MasterActiveReplicaSet failed rc[0x %x]\n",rc),
            rc);
    if(pStoredData != NULL)
    {
        /*
         * Return ERROR if checkpoint is not non collocated.
         */
        if( !CL_CKPT_IS_COLLOCATED(pStoredData->attrib.creationFlags))
            rc = CL_CKPT_ERR_OP_NOT_PERMITTED;
        CKPT_ERR_CHECK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
                ("MasterActiveReplicaSet failed rc[0x %x]\n",rc),
                rc);
                
        /*
         * Update the active address. Store the old replica address as well.
         */
        prevActiveAddr = pStoredData->activeRepAddr;
        pStoredData->prevActiveRepAddr = prevActiveAddr;
        saNameCopy(&name, &pStoredData->name);
        pStoredData->activeRepAddr = localAddr;
    }   
    
    /*
     * Tell the previous active that i am active.
     */
    if(prevActiveAddr != CL_CKPT_UNINIT_ADDR)
    {
        memcpy( &ckptVersion,
                gCkptSvr->versionDatabase.versionsSupported,
                sizeof(ClVersionT));
        rc = ckptIdlHandleUpdate(prevActiveAddr,gCkptSvr->ckptIdlHdl,0);
        CKPT_ERR_CHECK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
                ("Idl Handle update  failed rc[0x %x]\n",rc),
                rc);
        rc = VDECL_VER(clCkptActiveAddrInformClientAsync, 4, 0, 0)(gCkptSvr->ckptIdlHdl,
                ckptVersion,
                masterHdl,
                localAddr,NULL,NULL);
        if(CL_GET_ERROR_CODE(rc) == CL_IOC_ERR_COMP_UNREACHABLE || 
           CL_GET_ERROR_CODE(rc) == CL_IOC_ERR_HOST_UNREACHABLE || 
           CL_GET_ERROR_CODE(rc) == CL_ERR_TIMEOUT)
            rc = CL_OK;
        CKPT_ERR_CHECK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
          ("Failed to inform the prevActive abt active Address rc[0x %x]\n",
                 rc), rc);
        /*
         * remove MasterHdl from oldAddr and add to the newAddr.
         */
        _ckptPeerListMasterHdlAdd(masterHdl, prevActiveAddr, localAddr);
        /*
         * Remove the previous stored replica from the master handle. 
         * Just as a split brain protection, don't remove the replica if it means switching over ourselves
         */
        if(pStoredData && 
           pStoredData->replicaList && 
           prevActiveAddr != clIocLocalAddressGet())
        {
            clLogNotice("REP", "SWITCHOVER", "Deleting the replica address [%d] for checkpoint [%.*s]",
                        prevActiveAddr, pStoredData->name.length, pStoredData->name.value);
            clCntAllNodesForKeyDelete(pStoredData->replicaList, (ClCntKeyHandleT)(ClWordT)prevActiveAddr);
        }
    }
    
    /* 
     * Publish event to inform the clients.
     */
    memset(&eventInfo, 0, sizeof(ClCkptClientUpdInfoT));
    saNameCopy(&eventInfo.name, &name);
    eventInfo.name.length = htons(name.length);
    eventInfo.eventType   = htonl(CL_CKPT_ACTIVE_REP_CHG_EVENT);
    eventInfo.actAddr     = htonl(localAddr);
    clLogTrace("REP","SWITCHOVER","\n Ckpt: Publishing an event for"
                " change in active replica address\n");
    rc = clEventPublish(gCkptSvr->clntUpdEvtHdl, (const void*)&eventInfo,
            sizeof(ClCkptClientUpdInfoT), &eventId);
exitOnError:            
    {
        clHandleCheckin(gCkptSvr->masterInfo.masterDBHdl, 
                masterHdl);
    }            
exitOnErrorBeforeHdlCheckout:
    {
        /*
         * Unlock the master DB.
         */
        CKPT_UNLOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
        return rc;
    }
#endif
}

/*
 * Function for going through the list of opens/handles associated with a node
 * and deleting the passed handle.
 */
ClRcT _ckptPeerListWalk(ClCntKeyHandleT    userKey,
        ClCntDataHandleT   hashTable,
        ClCntArgHandleT    userArg,
        ClUint32T          dataLength)
{
    CkptPeerInfoT  *pPeerInfo = (CkptPeerInfoT*) hashTable;
    ClRcT          rc         = CL_OK;   

    if ( CL_OK != (rc = clCntAllNodesForKeyDelete(pPeerInfo->ckptList,
                                        userArg)))
    {
        CKPT_DEBUG_E(("ClientHdl %p Delete from list failed", userArg));
    }
    return CL_OK;
}



/*
 * Function to walk through the checkpoint's masterHdl list and delete
 * the given master handle.
 */
 
ClRcT _ckptPeerMasterHdlWalk(ClCntKeyHandleT    userKey,
                             ClCntDataHandleT   hashTable,
                             ClCntArgHandleT    userArg,
                             ClUint32T          dataLength)
{
    CkptPeerInfoT   *pPeerInfo = (CkptPeerInfoT*) hashTable;
    ClRcT           rc         = CL_OK; 
    
    /*
     * Delete the master handle from the MasterHdllist 
     */
    clLogDebug(CL_CKPT_AREA_MASTER, CL_CKPT_CTX_HDL_DEL, 
               "Deleting the handle [%#llX] from masterHdl list of node [%d]", 
               *(ClHandleT *)userArg, (ClIocNodeAddressT)(ClWordT) userKey);
    clCntAllNodesForKeyDelete(pPeerInfo->mastHdlList, userArg);
    
    return rc;
}



/*
 * Function to find any of the checkpoint replica node address.
 */

ClRcT _ckptCkptReplicaNodeAddrGet(ClIocNodeAddressT* pNodeAddr,
                                  ClCntHandleT replicaListHdl)
{
    ClRcT              rc        = CL_OK;
    ClCntNodeHandleT   nodeHdl   = 0;
    ClCntDataHandleT   dataHdl   = 0;

    /*
     * Find the first node in the checkpoint replica list.
     * If not found return CL_CKPT_UNINIT_ADDR.
     */
    rc = clCntFirstNodeGet(replicaListHdl, &nodeHdl);
    if((CL_OK == rc) && (nodeHdl != 0))
    {
        rc = clCntNodeUserDataGet(replicaListHdl, nodeHdl, &dataHdl);
        if( CL_OK != rc )
        {
            *pNodeAddr = CL_CKPT_UNINIT_ADDR;
            return CL_OK;
        }
        *pNodeAddr = (ClIocNodeAddressT)(ClWordT)dataHdl;
    }
    else
        *pNodeAddr = CL_CKPT_UNINIT_ADDR;

    return CL_OK;
}



/* 
 * Server side implementation of checkpoint close functionality.
 */
 
ClRcT VDECL_VER(clCkptMasterCkptClose, 4, 0, 0)(ClCkptHdlT        clientHdl,
                            ClIocNodeAddressT localAddr,
                            ClVersionT        *pVersion)
{
    ClRcT    rc = CL_OK;
    /* 
     * Check whether service is fully up or not.
     */
    CL_CKPT_SVR_EXISTENCE_CHECK;
    
    /*
     * Verify the version.
     */
    rc = clVersionVerify(&gCkptSvr->versionDatabase,(ClVersionT *)pVersion);
    if( rc != CL_OK)
    {
        return CL_OK; 
    }
        memset(pVersion,'\0',sizeof(ClVersionT));
    rc = _clCkptMasterClose(clientHdl, localAddr, CL_CKPT_MASTER_HDL);

    return rc;
}



/*
 * Fucntion to destroy a checkpoint's metadata when it is deleted.
 */
 
void clCkptDelCallback( ClIdlHandleT   ckptIdlHdl,
                        ClVersionT     version,
                        ClHandleT      ckptActHdl,
                        ClRcT          retCode,
                        void           *pData)
{                        
    ClHandleT  *pStoredDBHdl  = pData;
    ClRcT      rc           = CL_OK;

    if(gCkptSvr == NULL)
    {
        rc = CL_CKPT_ERR_NULL_POINTER;
        clHeapFree(pStoredDBHdl);
        clLogError(CL_CKPT_AREA_MASTER, CL_CKPT_CTX_CKPT_DEL,"Checkpoint server is finalized"
                                        "already [0x %x]",rc);
        return;
    }        
    
    /*
     * Lock the master DB.
     */
    CKPT_LOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
    
    /*
     * Delete the corresponding entry from peerList->masterHdl list 
     */
    clCntWalk(gCkptSvr->masterInfo.peerList, _ckptPeerMasterHdlWalk,
            pStoredDBHdl , sizeof(*pStoredDBHdl));
    
    /*
     * Destroy the masterHdl.
     */
    clHandleDestroy( gCkptSvr->masterInfo.masterDBHdl,
            *pStoredDBHdl);
            clLogDebug(CL_CKPT_AREA_MASTER, CL_CKPT_CTX_CKPT_DEL, 
                       "Master handle [%#llX] has been deleted", 
                       *pStoredDBHdl);

     clHeapFree(pStoredDBHdl);

    /*
     * Decrement the masterHdl count.
     */
    gCkptSvr->masterInfo.masterHdlCount--;        
    
    /*
     * Unlock the master DB.
     */
    CKPT_UNLOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
    return;
}



/*
 * Function to send checkpoint delete request from master to active replica.
 */
 
ClRcT ckptDeleteActiveReplicaIntimate(ClIocNodeAddressT activeAddr,
             ClHandleT                                  ckptActiveHdl,
             CkptEoClCkptActiveCkptDeleteAsyncCallbackT callback,
             ClHandleT                                  *pPassedHdl)          
{
    ClRcT      rc          = CL_OK;
    ClVersionT ckptVersion = {0};
    ClHandleT  *pCookie    = NULL;

    /*
     * Update the idl handle with destination as active address.
     */
     
    rc = ckptIdlHandleUpdate(activeAddr, gCkptSvr->ckptIdlHdl,0);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
            ("Updating IDL handle is failed rc[0x %x]\n",rc),rc);
    
    /*
     * Set the supported version.
     */
     
    memcpy( &ckptVersion,gCkptSvr->versionDatabase.versionsSupported,
            sizeof(ClVersionT));
    
    pCookie = clHeapCalloc(1, sizeof(ClHandleT));
    if( NULL == pCookie )
    {
        clLogCritical(CL_CKPT_AREA_MASTER, CL_CKPT_CTX_HDL_DEL,
                      "Failed to allocate memory while deleting the handle");
        goto exitOnError;
    }
    *pCookie = *pPassedHdl;
    /*
     * Send the call to the active replica.
     */
    rc = VDECL_VER(clCkptActiveCkptDeleteClientAsync, 4, 0, 0)(
            gCkptSvr->ckptIdlHdl,
            ckptVersion,
            ckptActiveHdl,
            callback,
            pCookie);
    if( CL_OK != rc ) clHeapFree(pCookie);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
            (" Call to active to delete the ckpt failed"
             "rc[0x %x]\n",rc),rc);
exitOnError:
    return rc;
}



/*
 * Function to walk through the checkpoint's replica list and 
 * decrement the replica count.
 */
 
ClRcT ckptReplicaListWalk(ClCntKeyHandleT  key,
                          ClCntDataHandleT data,
                          ClCntArgHandleT  arg,
                          ClUint32T        argSize)
{
    ClRcT          rc          = CL_OK;
    CkptPeerInfoT  *pPeerInfo  = NULL;

    rc = clCntDataForKeyGet(gCkptSvr->masterInfo.peerList,
            (ClCntKeyHandleT)key,
            (ClCntDataHandleT*)&pPeerInfo);
    if( rc == CL_OK && pPeerInfo != NULL)
    {
        /*
         * Decrement the replica count.
         */
        pPeerInfo->replicaCount--;
    }
    return CL_OK;
}



/*
 * Function to delete the entry from name-tranalation table.
 */
     
ClRcT ckptDeleteEntryFromXlationTable(SaNameT  *pName)
{
    ClCntNodeHandleT    pNodeHandle = 0;
    ClUint32T           cksum       = 0;
    CkptXlationLookupT  lookup      = {0};
    ClRcT               rc          = 0;
    
    CKPT_DEBUG_T(("pName: %s\n", pName->value));
    memset(&lookup, 0, sizeof(lookup));
    
    /*
     * Checksum of the chckpoint name is the key for lookup.
     */
    clCksm32bitCompute ((ClUint8T *)pName->value, pName->length, &cksum);
    lookup.cksum = cksum;
    
    saNameCopy(&lookup.name, pName);
    
    /*
     * Find the entry and delete it.
     */
    rc = clCntNodeFind((ClCntHandleT)
            gCkptSvr->masterInfo.nameXlationDBHdl,
            (ClCntKeyHandleT)&lookup, 
            &pNodeHandle);
    if(rc == CL_OK)
        clCntNodeDelete(gCkptSvr->masterInfo.nameXlationDBHdl,
                pNodeHandle);
    return rc;             
}          



/*
 * Deleting the checkpoint info locally and updating info in 
 * active server also.
 */

ClRcT ckptMasterLocalCkptDelete(ClHandleT masterHdl)
{
    ClRcT                rc          = CL_OK;
    CkptMasterDBEntryT  *pStoredData = NULL;
    ClIocNodeAddressT    tempAddr    = 0;
    /*
     * Retrieve the information associated with the master handle.
     */
    rc = ckptSvrHdlCheckout(gCkptSvr->masterInfo.masterDBHdl,
            masterHdl, (void **)&pStoredData);
    CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR, CL_LOG_SEV_ERROR,
            (" Handle Checkout Failed rc[0x %x]\n", rc), rc);

    /*
     * In case of collocated ckpt if no active replica
     * exists, get any replica adderss for the checkpoint.
     */
    if(pStoredData->activeRepAddr == CL_CKPT_UNINIT_ADDR)
    {
        _ckptCkptReplicaNodeAddrGet(&tempAddr, 
                pStoredData->replicaList);
    }
    else
    {
        tempAddr = pStoredData->activeRepAddr;
    }

    clLogDebug(CL_CKPT_AREA_MASTER, CL_CKPT_CTX_CKPT_DEL, 
            "Chosen address [%d] to notify ckpt deletion", 
            tempAddr);
    if(tempAddr != CL_CKPT_UNINIT_ADDR)
    {
        /* 
         * Decrement the replica count at the replica nodes.
         */
        clCntWalk(pStoredData->replicaList,
                ckptReplicaListWalk,NULL,0);
        /*
         * Ask the active replica to delete the checkpoint.
         * Active replica will inform all other replicas.
         * Just removed optimization call to local guy to delete 
         * checkpoint. as it was causing problem from PeerDown path 
         * to delete the checkpoint on the local server with deadlock mutex.
         */
        rc = ckptDeleteActiveReplicaIntimate(tempAddr,
                pStoredData->activeRepHdl, clCkptDelCallback, &masterHdl);
        CKPT_ERR_CHECK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
                (" Call to active to delete the ckpt failed"
                 "rc[0x %x]\n",rc),rc);
    }
    else
    {
        clCntWalk(gCkptSvr->masterInfo.peerList,
                _ckptPeerMasterHdlWalk,
                &masterHdl ,
                sizeof(masterHdl));
        clHandleDestroy( gCkptSvr->masterInfo.masterDBHdl,
                masterHdl);
        gCkptSvr->masterInfo.masterHdlCount--;        
    }
    clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
            "MasterCheckpointClose retention timer \
            creation failed, rc = %x", rc);

    clDbgResourceNotify(clDbgCheckpointResource, clDbgRelease, 1, gCkptSvr->masterInfo.masterDBHdl, ("Deleting Checkpoint %s",pStoredData->name.value));

    /*
     * Delete the replica list.
     */
    clCntDelete(pStoredData->replicaList);
    pStoredData->replicaList = 0;
    rc = clHandleCheckin( gCkptSvr->masterInfo.masterDBHdl,
            masterHdl);
    return rc;
exitOnError:
    clHandleCheckin(gCkptSvr->masterInfo.masterDBHdl, masterHdl);
exitOnErrorBeforeHdlCheckout:
    return rc;
}

void
VDECL_VER(clCkptCheckpointReplicaRemoveCallback, 4, 0, 0)(ClIdlHandleT       idlHdl,
                                      ClCkptHdlT         masterHdl,
                                      ClIocNodeAddressT  replicaAddr, 
                                      ClRcT              retCode,
                                      ClPtrT             pCookie)
{
    ClRcT               rc              = CL_OK;
    CkptMasterDBEntryT  *pMasterDBEntry = NULL;
    CkptPeerInfoT       *pPeerInfo      = NULL;
    ClVersionT          version         = {0};

    clLogDebug(CL_CKPT_AREA_MASTER, CL_CKPT_CTX_REPL_NOTIFY,
               "Remove Notify callbacks RetCode [%x]"
               "peerAddr [%d]", retCode, replicaAddr);
    if( gCkptSvr == NULL || gCkptSvr->serverUp == CL_FALSE)
    {
        clLogWarning(CL_CKPT_AREA_MASTER, CL_CKPT_CTX_REPL_NOTIFY, 
                     "Checkpoint server is not ready to provide service");
        /*
         * return 
         */
        return ;
    }

    if(CL_RMD_TIMEOUT_UNREACHABLE_CHECK(rc))
    {
        /* 
         *  If the error is timeout, just going not retrying
         */
        return ;
    }

    /*
     * Lock the master BD.
     */
    CKPT_LOCK(gCkptSvr->masterInfo.ckptMasterDBSem);

    /*
     * Checkout the checkpoint related metadata.
     */
    rc = ckptSvrHdlCheckout( gCkptSvr->masterInfo.masterDBHdl,
                             masterHdl,
                             (void **) &pMasterDBEntry);
    if( CL_OK != rc )
    {
        clLogError(CL_CKPT_AREA_MASTER, CL_CKPT_CTX_REPL_NOTIFY, 
                   "Checkpoint [%#llX] does not exist, this might have been"
                   "deleted while call is on the replica side", 
                   masterHdl);
        goto unlockNExit;
    }

    /*
     * Add the replica address to the replica list of the checkpoint. 
     */         
     clLogDebug(CL_CKPT_AREA_MASTER, CL_CKPT_CTX_REPL_NOTIFY, 
		"Checkpoint [%.*s] has been removed from address [%d]", 
		pMasterDBEntry->name.length, pMasterDBEntry->name.value, 
		replicaAddr);
    rc = clCntAllNodesForKeyDelete(pMasterDBEntry->replicaList,
            (ClCntKeyHandleT)(ClWordT) replicaAddr);
    if( CL_OK != rc )
    {
        clLogError(CL_CKPT_AREA_MASTER, CL_CKPT_CTX_REPL_NOTIFY, 
                   "Replica address [%d] has been deleted from replica list",
                   replicaAddr);
        goto hdlCheckinNexit;
    } 
            
    /*          
     * Increment the replica count.
     */
    rc = clCntDataForKeyGet( gCkptSvr->masterInfo.peerList,
            (ClCntDataHandleT)(ClWordT) replicaAddr,
            (ClCntDataHandleT *) &pPeerInfo);
    if( CL_OK != rc )
    {
        clLogError(CL_CKPT_AREA_MASTER, CL_CKPT_CTX_REPL_NOTIFY, 
                   "Failed to get info about peerAddr [%d] from peerlist",
                    replicaAddr);
        goto hdlCheckinNexit;
    }
    pPeerInfo->replicaCount--;
    /*
     * Update the deputy
     */
    {
         rc = ckptIdlHandleUpdate(CL_IOC_BROADCAST_ADDRESS, 
                                  gCkptSvr->ckptIdlHdl, 0);
         if( CL_OK != rc )
         {
             clLogError(CL_CKPT_AREA_MASTER, CL_CKPT_CTX_REPL_NOTIFY,
                     "Idl handle updation failed rc[0x %x]", rc);
             goto hdlCheckinNexit;
         }

         memcpy( &version, gCkptSvr->versionDatabase.versionsSupported, 
             sizeof(ClVersionT));
 
         rc = VDECL_VER(clCkptDeputyReplicaListUpdateClientAsync, 4, 0, 0)(gCkptSvr->ckptIdlHdl,
                                        &version,
                                        masterHdl,
                                        replicaAddr,
                                        CL_CKPT_REPLICA_DEL, 
                                        NULL,NULL);
         if( CL_OK != rc )
         {
             clLogError(CL_CKPT_AREA_MASTER, CL_CKPT_CTX_REPL_NOTIFY, 
                        "Deputy replica list updation failed rc[0x %x]",
                        rc);
         }
    }
hdlCheckinNexit:
    /*
     * Checkin the checkpoint related metadata.
     */
    clHandleCheckin( gCkptSvr->masterInfo.masterDBHdl, masterHdl);
unlockNExit:
    /*
     * Unlock the master DB.
     */
    CKPT_UNLOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
    return;
}

ClRcT
clCkptMasterReplicaRemoveUpdate(ClCkptHdlT         masterHdl,
                                ClIocNodeAddressT  replicaAddr)
{
    ClRcT  rc = CL_OK;

    rc = ckptIdlHandleUpdate(replicaAddr, gCkptSvr->ckptIdlHdl, 0);
    if( CL_OK != rc )
    {
        clLogDebug(CL_CKPT_AREA_MASTER, CL_CKPT_CTX_CKPT_DEL, 
                   "Checkpoint idl handle updation failed rc[0x %x]", rc);
        /* explicit return CL_OK, this updation is not mandatory */
        return CL_OK;
    }

    clLogDebug(CL_CKPT_AREA_MASTER, CL_CKPT_CTX_CKPT_DEL, 
               "Ask server [%d] to remove the DISTRIBUTED replica for"
               "checkpoint handle [%#llX]", replicaAddr, masterHdl);
    rc = VDECL_VER(clCkptCheckpointReplicaRemoveClientAsync, 4, 0, 0)(gCkptSvr->ckptIdlHdl, 
                                                  masterHdl, 
                                                  replicaAddr, 
                                                  VDECL_VER(clCkptCheckpointReplicaRemoveCallback, 4, 0, 0),
                                                  NULL);
    if( CL_OK != rc )
    {
        clLogError(CL_CKPT_AREA_MASTER, CL_CKPT_CTX_CKPT_DEL, 
              "Failed to remove the replica from address [%d] with rc[0x %x]",
                    replicaAddr, rc);
    }
    /*
     * Just logging error, but returning CL_OK, as this fail call doesn't
     * affect the flow.
     */
    return CL_OK;
}

/*
 * Core logic for checkpoint close functionality.
 * Should be called with master db lock held.
 */
ClRcT _clCkptMasterCloseNoLock(ClHandleT         clientHdl, 
                               ClIocNodeAddressT localAddr,
                               ClUint32T         flag)
{
    ClRcT                   rc              = CL_OK;
    CkptMasterDBClientInfoT *pClientEntry   = NULL;
    ClHandleT               masterHdl       = CL_CKPT_INVALID_HDL;
    CkptMasterDBEntryT      *pStoredData    = NULL;
    ClTimerTimeOutT         timeOut         = {0};
    ClTimeT                 retenTime       = 0;
    CkptDynamicSyncupT      tag             = CL_CKPT_CKPT_CLOSE;
    CkptPeerInfoT           *pPeerInfo      = NULL;
    ClHandleT               *pTimerArg      = NULL;

    CKPT_NULL_CHECK(gCkptSvr);

    /* 
     * Retrieve the information associated with the client handle.
     */
    rc = clHandleCheckout(gCkptSvr->masterInfo.clientDBHdl,
                          clientHdl,
                          (void **)&pClientEntry);
    CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
                                  (" MasterCheckpointClose failed rc[0x %x]\n",rc),
                                  rc);

    /*
     * Get the associated master handle.
     */
    masterHdl = pClientEntry->masterHdl;

    rc = clHandleCheckin(gCkptSvr->masterInfo.clientDBHdl, 
                         clientHdl);
    CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
                                  (" MasterCheckpointClose failed rc[0x %x]\n",rc),
                                  rc);

    /* 
     * Delete the entry from clientDB database 
     */
    rc = clHandleDestroy(gCkptSvr->masterInfo.clientDBHdl,
                         clientHdl); 
    CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
                                  (" MasterCheckpointClose failed rc[0x %x]\n",rc),
                                  rc);

    /*
     * Decrement the client handle count.
     */
    gCkptSvr->masterInfo.clientHdlCount--;        

    /*
     * Delete the client handle from the node's ckptHdl list.
     */
    rc = clCntDataForKeyGet(gCkptSvr->masterInfo.peerList, (ClPtrT)(ClWordT)localAddr,
                            (ClCntDataHandleT *)&pPeerInfo); 
    CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
                                  ("Failed to get info of addr %d in peerList rc[0x %x]\n",
                                   localAddr, rc), rc);
    if ( CL_OK != (rc = clCntAllNodesForKeyDelete(pPeerInfo->ckptList,
                                                  (ClPtrT)(ClWordT)clientHdl)))
    {
        CKPT_DEBUG_E(("ClientHdl %#llX Delete from list failed", clientHdl));
    }
                    
    /*
     * Retrieve the metadata associated with the master handle.
     */
    rc = ckptSvrHdlCheckout(gCkptSvr->masterInfo.masterDBHdl,
                            masterHdl, (void **)&pStoredData);
    CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
                                  (" MasterCheckpointClose failed rc[0x %x]\n",rc),
                                  rc);
            
    CL_ASSERT(pStoredData != NULL);

    /*
     * In case of sync checkpoint and collocated checkpoint(if active replica
     * is the local node), if the close has been called by the user and not
     * via event, remove th eentry from the node's masterHdl list.
     */
    if((flag == CL_CKPT_MASTER_HDL) && 
       (CL_CKPT_IS_SYNCHRONOUS(pStoredData->attrib.creationFlags) ||
        (CL_CKPT_IS_COLLOCATED(pStoredData->attrib.creationFlags) &&
         (pStoredData->activeRepAddr == localAddr))))
    {
        _ckptPeerListMasterHdlAdd(masterHdl, CL_CKPT_UNINIT_ADDR,
                                  CL_CKPT_UNINIT_ADDR);
    }                                             
     
    /* 
     * Decrement the checkpoint reference count.
     */
    pStoredData->refCount--;
    clLogInfo("CKP","MGT","Checkpoint [%s] reference count decremented.  Now [%d].",
              pStoredData->name.value, pStoredData->refCount);

    if(pStoredData->refCount == 0)
    {
        /*
         * reference count for the checkpoint is 0.
         * If Unlink had already been called, delete the checkpoint.
         * Else start the retention timer.
         */
        if(pStoredData->markedDelete == 1) 
        {
            /* 
             * Update active replica.
             */
            tag = CL_CKPT_CLOSE_MARKED_DELETE; 

            /*
             * Delete the retention timer.
             */
            if( pStoredData->retenTimerHdl)
                clTimerDelete(&pStoredData->retenTimerHdl);

            /*
             * Delete the checkpoint locally and inform active replica .
             */
            rc = ckptMasterLocalCkptDelete(masterHdl);
              
            CKPT_ERR_CHECK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
                           ("Failed to delete the checkpoint, rc[0x %x]\n",rc),
                           rc);
        }
        else 
        {
            /* 
             * Unlink not yet called. Start the retention related timer for non distributed checkpoints.
             */
            if( (pStoredData->attrib.creationFlags & CL_CKPT_DISTRIBUTED) )
                retenTime = 0;
            else
                retenTime = pStoredData->attrib.retentionDuration;

            if(!retenTime)
                goto out_delete;

            if(retenTime != CL_TIME_END)
            {
                memset(&timeOut, 0, sizeof(ClTimerTimeOutT));
                timeOut.tsMilliSec = (retenTime/CL_CKPT_NANO_TO_MILLI);
                tag = CL_CKPT_START_TIMER;
                if( NULL != (pTimerArg = clHeapCalloc(1, sizeof(ClHandleT) )) )
                {
                    *pTimerArg = masterHdl;
                }
                rc = clTimerCreateAndStart(timeOut,
                                           CL_TIMER_ONE_SHOT,
                                           CL_TIMER_SEPARATE_CONTEXT,
                                           _ckptRetentionTimerExpiry,
                                           (ClPtrT) pTimerArg, 
                                           &pStoredData->retenTimerHdl);

                /*
                 * In case retention timer is 0 or retention timer start 
                 * failed delete the checkpoint from all replicas and update
                 * the master metadata.
                 */
                if(rc != CL_OK)
                {
                    clHeapFree(pTimerArg);
                    clLogError("CKP","MGT","Checkpoint [%s] retention timer [%lldms] expired due to timer failure [0x%x].", pStoredData->name.value, retenTime/CL_CKPT_NANO_TO_MILLI,rc);

                    out_delete:
                    tag = CL_CKPT_CLOSE_MARKED_DELETE; 
                
                    clLogWarning("CKP","MGT","Checkpoint [%s] deleted due to retention timer [%llums] expiry.", pStoredData->name.value, retenTime/CL_CKPT_NANO_TO_MILLI);
                    /*
                     * Delete the entry from name-tranalation table.
                     */
                    rc = ckptDeleteEntryFromXlationTable(&pStoredData->name);
                
                    /*
                     * Delete the checkpoint locally and inform active replica .
                     */
                    rc = ckptMasterLocalCkptDelete(masterHdl);
                    if( CL_OK != NULL)
                    {
                        CKPT_DEBUG_E(("Failed to delete the checkpoint"));
                    }    
                }
            }                            
        }
    }
    else
    {
        /* If its not the last close call, go and remove the replica on this
         * node, if the checkpoint flag is CL_CKPT_DISTRIBUTED
         */
        if( pStoredData->attrib.creationFlags & CL_CKPT_DISTRIBUTED )
        {
            /* Commenting this, because count should be maintained for
             * different application, we can't just go and delete for one
             * close
             */
            //rc = clCkptMasterReplicaRemoveUpdate(masterHdl, localAddr);
        }
    }

    /* 
     * Update the deputy master.
     */
    {
        CkptUpdateInfoT   updateInfo = {0};

        /* 
         * Passing dummy values as in this case it is not needed.
         */
        updateInfo.discriminant = CKPTUPDATEINFOTREPLICAADDR; 
        updateInfo.ckptUpdateInfoT.replicaAddr= localAddr;

        rc = ckptDynamicDeputyUpdate(tag,
                                     clientHdl,
                                     masterHdl,
                                     &updateInfo);
        CKPT_ERR_CHECK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
                       ("Failed to update deputy master, rc[0x %x]\n",rc),
                       rc);
    } 
    exitOnError:
    clHandleCheckin(gCkptSvr->masterInfo.masterDBHdl,masterHdl);
    exitOnErrorBeforeHdlCheckout:
    {
        return rc;
    }
}

ClRcT _clCkptMasterClose(ClHandleT         clientHdl, 
                         ClIocNodeAddressT localAddr,
                         ClUint32T         flag)
{
    ClRcT rc = CL_OK;
    /* 
     * Lock the master DB.
     */
    CKPT_LOCK(gCkptSvr->masterInfo.ckptMasterDBSem);

    rc = _clCkptMasterCloseNoLock(clientHdl, localAddr, flag);

    CKPT_UNLOCK(gCkptSvr->masterInfo.ckptMasterDBSem);

    return rc;
}



/*
 * Server implementation of checkpoint delete functionality.
 */
 
ClRcT VDECL_VER(clCkptMasterCkptUnlink, 4, 0, 0)(SaNameT           *pName,
                             ClIocNodeAddressT localAddr,
                             ClVersionT        *pVersion)
{
    ClRcT                   rc              = CL_OK;
    CkptMasterDBEntryT      *pStoredData    = NULL;
    ClHandleT               storedDBHdl     = CL_CKPT_INVALID_HDL;
    ClUint32T               cksum           = 0;
    CkptXlationLookupT      lookup          = {0};
    CkptXlationDBEntryT     *pStoredDBEntry = NULL;
    CkptDynamicSyncupT      tag             = CL_CKPT_CKPT_UNLINK;

    /*
     * Check whether the server is fully up or not.
     */
    CL_CKPT_SVR_EXISTENCE_CHECK;
    
    /*
     * Verify the version.
     */
    rc = clVersionVerify(&gCkptSvr->versionDatabase,(ClVersionT *)pVersion);
    if( rc != CL_OK)
    {
        return CL_OK; 
    }
    memset(pVersion,'\0',sizeof(ClVersionT));
    
    /*
     * Check the validity of the checkpoint name.
     */
    CL_CKPT_NAME_VALIDATE(pName);

    /*
     * Lock the master DB.
     */
    CKPT_LOCK(gCkptSvr->masterInfo.ckptMasterDBSem);

    /* 
     * Check in the name Xlation table, whether the meta data for the 
     * given ckpt exists or not.
     */
    memset(&lookup, 0, sizeof(lookup));
    clCksm32bitCompute ((ClUint8T *)pName->value, pName->length, &cksum);
    lookup.cksum = cksum;
    saNameCopy(&lookup.name, pName);
    rc = clCntDataForKeyGet(
            (ClCntHandleT)gCkptSvr->masterInfo.nameXlationDBHdl,
            (ClCntKeyHandleT)&lookup,
            (ClCntDataHandleT *)&pStoredDBEntry);
    if(rc != CL_OK)
    {
        rc = CL_CKPT_RC(CL_ERR_NOT_EXIST);
        CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
                ("ckptMasterUnlink failed rc[0x %x]\n",
                 rc), rc);
    }
    
    /* 
     * Entry found.
     */
    if( NULL != pStoredDBEntry)
    {
        /*
         * Get the master handle associated with the checkpoint.
         */
        storedDBHdl = pStoredDBEntry->mastHdl;

        /*
         * Retrieve the information associated with the master handle.
         */
        rc = clHandleCheckout(gCkptSvr->masterInfo.masterDBHdl,
                storedDBHdl, (void **)&pStoredData);
        CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
                (" MasterCheckpointUnlink failed rc[0x %x]\n",rc),
                rc);
                
        /* 
         * Delete the entry from the Xlation table.
         */ 
        rc = clCntAllNodesForKeyDelete(gCkptSvr->masterInfo.nameXlationDBHdl,
                            (ClCntKeyHandleT)&lookup);
        CKPT_ERR_CHECK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
                (" MasterCheckpointUnlink failed rc[0x %x]\n",rc),
                rc);
                        
        /* 
         * Check the reference county of the checkopoint.
         * If 0 the delete the checkpoint elsemark it for deletion.
         */
        if(pStoredData->refCount == 0)
        {
            /*
             * Delete the checkpoint.
             */

             /*
              * Delete the retention timerassociated with the checkpoint.
              */
            if(pStoredData->retenTimerHdl)
                clTimerDelete(&pStoredData->retenTimerHdl);
            
            tag         = CL_CKPT_MARKED_DELETE;

            /*
             * Delete the checkpoint locally and inform active replica .
             */
             rc = ckptMasterLocalCkptDelete( storedDBHdl);
             CKPT_ERR_CHECK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
                ("Failed to delete the checkpoint, rc[0x %x]\n",rc),
                rc);
        }
        else
        {
            /*
             * Mark the checkpoint for delete.
             */
            tag = CL_CKPT_CKPT_UNLINK;
            pStoredData->markedDelete = 1;
        }
    }

    /*
     * Update the deputy master.
     */
    {
        CkptUpdateInfoT   updateInfo = {0};

        updateInfo.discriminant = CKPTUPDATEINFOTNULLINFO;
        updateInfo.ckptUpdateInfoT.nullInfo = 0;

        rc = ckptDynamicDeputyUpdate(tag,
                                     CL_CKPT_UNINIT_VALUE,
                                     storedDBHdl,
                                     &updateInfo);
        CKPT_ERR_CHECK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
                ("Failed to update deputy master, rc[0x %x]\n",rc),
                rc);
    }
exitOnError:
    {
        /*
         * Checkin the master handle related information.
         */
        clHandleCheckin(gCkptSvr->masterInfo.masterDBHdl, 
                        storedDBHdl);
    }
exitOnErrorBeforeHdlCheckout:
    {
        /*
         * Unlock the master DB.
         */
        CKPT_UNLOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
        return rc;
    }
}



/*
 * Function for dynamically updating the deputy server.
 */

ClRcT ckptDynamicDeputyUpdate(CkptDynamicSyncupT updateFlag,
                              ClHandleT          clientHdl,
                              ClHandleT          masterHdl,
                              CkptUpdateInfoT    *pUpdateInfo)
{
    ClRcT               rc          = CL_OK;    
    CkptDynamicInfoT    dynamicInfo = {0};
    ClVersionT          ckptVersion = {0};

    /*
     * Fill the dynamic info to be sent to the deputy.
     */
    dynamicInfo.clientHdl = clientHdl;
    dynamicInfo.masterHdl = masterHdl;
    dynamicInfo.data      = pUpdateInfo;

    /*
     * Set the supported version.
     */
    memcpy(&ckptVersion,gCkptSvr->versionDatabase.versionsSupported,
                        sizeof(ClVersionT));

    /*
     * Update the idl handle with the destination address.
     */
    rc = ckptIdlHandleUpdate(CL_IOC_BROADCAST_ADDRESS,
                       gCkptSvr->ckptIdlHdl,0);

    /*
     * Send the call to the deputy master.
     */
    rc = VDECL_VER(clCkptDeputyDynamicUpdateClientAsync, 4, 0, 0)(
                            gCkptSvr->ckptIdlHdl,
                            &ckptVersion,
                            updateFlag,
                            &dynamicInfo,
                            NULL,0);
    if( CL_GET_ERROR_CODE(rc) == CL_IOC_ERR_COMP_UNREACHABLE || 
        CL_GET_ERROR_CODE(rc) == CL_IOC_ERR_HOST_UNREACHABLE )
    {
       clLogWarning(CL_CKPT_AREA_PEER, CL_CKPT_CTX_REPL_UPDATE,
                   "Deputy address [%d] is not reachable rc [0x %x]", 
                    gCkptSvr->masterInfo.deputyAddr, rc); 
      rc = CL_OK ;
    }
    return rc;
}



/*
 * Server side implementation of retention timer set.
 */
 
ClRcT VDECL_VER(clCkptMasterCkptRetentionDurationSet, 4, 0, 0)(ClCkptHdlT  clientHdl,
                                           ClTimeT     retentionDuration,
                                           ClVersionT  *pVersion)
{
    ClRcT                   rc              = CL_OK;
    CkptMasterDBClientInfoT *pClientEntry   = NULL;
    ClHandleT               masterHdl       = CL_CKPT_INVALID_HDL;
    CkptMasterDBEntryT      *pStoredData    = NULL;

    /*
     * Verify the version.
     */
    rc = clVersionVerify(&gCkptSvr->versionDatabase,(ClVersionT *)pVersion);
    if( rc != CL_OK)
    {
        return CL_OK; 
    }
    
    memset(pVersion,'\0',sizeof(ClVersionT));
    
    /*
     * Check whether the server is fully up or not.
     */
    CL_CKPT_SVR_EXISTENCE_CHECK; 
    
    /*
     * Lock the master DB.
     */

    CKPT_LOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
    /*
     * Retrieve the information assocaited with the client handle.
     */
    rc = clHandleCheckout(gCkptSvr->masterInfo.clientDBHdl,
            clientHdl,
            (void **)&pClientEntry);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
            (" MasterCkptRetentionDurationSet failed rc[0x %x]\n",rc),
            rc);

    /*
     * Get the associated master handle.
     */
    masterHdl = pClientEntry->masterHdl;

    /*
     * Retrieve the information assocaited with the master handle.
     */
    rc = clHandleCheckout(gCkptSvr->masterInfo.masterDBHdl,
            masterHdl, (void **)&pStoredData);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
            (" MasterCkptRetentionDurationSet failed rc[0x %x]\n",rc),
            rc);

    /* 
     * Update the retention timer value.
     * Incase the checkpoint has already been unkinked, return ERROR.
     */
    if(pStoredData->markedDelete != 1)
    {
        pStoredData->attrib.retentionDuration = retentionDuration;
    }
    else
    {
        rc = CL_CKPT_ERR_BAD_OPERATION;
        CKPT_ERR_CHECK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
                (" MasterCkptRetentionDurationSet failed rc[0x %x]\n",rc),
                rc);
    }

    /* 
     * Update the deputy master.
     */
    {
        CkptUpdateInfoT   updateInfo = {0};

        /*
         * Update retentimer info in the deputy.
         */
        updateInfo.discriminant = CKPTUPDATEINFOTRETENTIME;
        updateInfo.ckptUpdateInfoT.retenTime = retentionDuration;

        rc = ckptDynamicDeputyUpdate(CL_CKPT_COMM_UPDATE,
                                     CL_CKPT_UNINIT_VALUE,
                                     masterHdl,
                                     &updateInfo);
        CKPT_ERR_CHECK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
                ("Failed to update deputy master, rc[0x %x]\n",rc),
                rc);
    }

exitOnError:
    {
        /* 
         * Checkin the updated stuff.
         */
        clHandleCheckin(gCkptSvr->masterInfo.masterDBHdl, 
                masterHdl);
        clHandleCheckin(gCkptSvr->masterInfo.clientDBHdl, 
                clientHdl);
                
        /*
         * Lock the master DB.
         */
        CKPT_UNLOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
        return rc;
    }
}



/*
 * Function to check whether the given node is suitable for
 * storing repliuca info or not.
 */
 
ClRcT ckptReplicaNodeGetCallback(ClCntKeyHandleT    userKey,
                                 ClCntDataHandleT   hashTable,
                                 ClCntArgHandleT    userArg,
                                 ClUint32T          dataLength)
{
    CkptPeerInfoT     *pPeerInfo = (CkptPeerInfoT *) hashTable;
    CkptPeerWalkInfoT *pInfo     = (CkptPeerWalkInfoT*) userArg;
    ClIocNodeAddressT addr = (ClWordT)userKey;

    if((pInfo->inAddr != addr) && (pInfo->prevAddr != addr) &&
       (pPeerInfo->available == CL_CKPT_NODE_AVAIL))
    {
        if(pInfo->count > pPeerInfo->replicaCount)
        {
            pInfo->outAddr   = addr;
            pInfo->count     = pPeerInfo->replicaCount;
        }
    }
    return CL_OK;
}



/*
 * Function to walk through the ckpt's replica list and get
 * a replica node,
 */
 
ClRcT ckptReplicaNodeGet( ClIocNodeAddressT peerAddr,
                          ClIocNodeAddressT *pNodeAddr)
{
    ClRcT              rc  = CL_OK;
    CkptPeerWalkInfoT  info = {0}; 
    ClBoolT            flag = CL_FALSE;
    
    rc = _ckptIsReplicationPossible(&flag);
    if((rc != CL_OK) || (flag == CL_FALSE))
    {
        *pNodeAddr = CL_CKPT_UNINIT_ADDR;
        return CL_OK;
    }   
    info.outAddr   = CL_CKPT_UNINIT_ADDR;
    info.prevAddr  = peerAddr;
    info.inAddr    = *pNodeAddr;
    info.count     = CL_CKPT_UNINIT_ADDR;

    /*
     * Walk throught the replica list.
     */
    rc = clCntWalk(gCkptSvr->masterInfo.peerList, ckptReplicaNodeGetCallback,
            (ClCntArgHandleT) &info,
            sizeof(CkptPeerWalkInfoT));
    CL_ASSERT(rc == CL_OK);                  
    
    *pNodeAddr = info.outAddr;
    
    return CL_OK;              
}



/*
 * Retention timer expity callback function.
 */
 
ClRcT _ckptRetentionTimerExpiry (void *pRetenInfo)
{
    ClRcT                   rc            = CL_OK;
    CkptMasterDBEntryT      *pStoredData  = NULL;
    ClHandleT               masterHdl     = CL_HANDLE_INVALID_VALUE; 

    if( NULL != pRetenInfo )
    {
        masterHdl = *(ClHandleT *) pRetenInfo;
    }
    /*
     * Check whether the server is fully up or not.
     */
    CL_CKPT_SVR_EXISTENCE_CHECK; 
    /*
     * Lock the master DB.
     */
    CKPT_LOCK(gCkptSvr->masterInfo.ckptMasterDBSem);

    /*
     * Set the execution context.
     */
    clEoMyEoObjectSet(gCkptSvr->eoHdl);
    clEoMyEoIocPortSet(gCkptSvr->eoPort);

    /*
     * Retrieve the information associated with the master handle.
     */
    rc = clHandleCheckout(gCkptSvr->masterInfo.masterDBHdl,
            masterHdl, (void **)&pStoredData);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
            (" Check point delete failed rc[0x %x]\n",rc),
            rc);
    /*
     * Delete the entry from name-tranalation table.
     */
    rc = ckptDeleteEntryFromXlationTable(&pStoredData->name);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
            (" Check point delete failed rc[0x %x]\n",rc),
            rc);

    /* 
     * In case retention timer expiry has been triggered in deputy,
     * return OK.
     */
    if(gCkptSvr->masterInfo.masterAddr != clIocLocalAddressGet())
    {
        /*
         * Delete the corresponding entry from peerList->masterHdl list
         */
        clCntWalk(gCkptSvr->masterInfo.peerList, _ckptPeerMasterHdlWalk,
                  pRetenInfo , sizeof(ClHandleT));

        /*
         * Destroy the masterHdl.
         */
        clHandleDestroy( gCkptSvr->masterInfo.masterDBHdl,
                         masterHdl);

        /*
         * Decrement the masterHdl count.
         */
        gCkptSvr->masterInfo.masterHdlCount--;

        rc = clHandleCheckin(gCkptSvr->masterInfo.masterDBHdl, 
                             masterHdl);
        CKPT_UNLOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
        return  CL_OK; 
    }

    /*
     * Delete the checkpoint locally and inform active replica .
     */
    rc = ckptMasterLocalCkptDelete(masterHdl);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
            (" Check point delete failed rc[0x %x]\n",rc),
            rc);
    rc = clHandleCheckin(gCkptSvr->masterInfo.masterDBHdl, 
            masterHdl);
exitOnError:
    {
        /*
         * Unlock the master DB.
         */
        CKPT_UNLOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
        clHeapFree(pRetenInfo);
        return rc;
    }
}



/*
 * Function to get the checkpoint status from the master.
 */
 
ClRcT VDECL_VER(clCkptMasterStatusInfoGet, 4, 0, 0)(ClHandleT         hdl,
                                ClTimeT           *pTime,
                                ClIocNodeAddressT *pActiveAddr,
                                ClUint32T         *pRefCount,
                                ClUint8T          *pDeleteFlag)
{
    ClRcT                rc              = CL_OK;
    CkptMasterDBEntryT   *pMasterDBEntry = NULL;

    CKPT_LOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
    rc = clHandleCheckout(gCkptSvr->masterInfo.masterDBHdl,
            hdl,(void **) &pMasterDBEntry);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
            ("clMasterStatusInfoGet failed rc[0x %x]\n",rc),
            rc);
    *pTime = pMasterDBEntry->attrib.retentionDuration;
    *pActiveAddr = pMasterDBEntry->activeRepAddr;
    *pRefCount   = pMasterDBEntry->refCount;
    *pDeleteFlag = pMasterDBEntry->markedDelete;
    rc = clHandleCheckin(gCkptSvr->masterInfo.masterDBHdl, 
            hdl);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
            ("clMasterStatusInfoGet failed rc[0x %x]\n",rc),
            rc);
exitOnError:
    {
        CKPT_UNLOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
        return rc;
    }
}


/*************************************************************************/
/********************** Syncup related functions *************************/
/*************************************************************************/

/*
 * Funtion for packing name to master address translation table.
 */
 
ClRcT _ckptMasterXlationTablePack(ClCntKeyHandleT    userKey,
                                  ClCntDataHandleT   hashTable,
                                  ClCntArgHandleT    userArg,
                                  ClUint32T          dataLength)
{
    ClRcT                   rc          = CL_OK;
    CkptXlationDBEntryT    *pInXlation  = (CkptXlationDBEntryT *)hashTable;
    ClCkptMasterSyncupWalkT *pWalkArgs = userArg;
    CkptXlationDBEntryT    *pOutXlation = (CkptXlationDBEntryT *)pWalkArgs->pData;
    CKPT_NULL_CHECK(pInXlation);

    if(pWalkArgs->index < pWalkArgs->count)
    {
        pOutXlation = pOutXlation + pWalkArgs->index;
        ++pWalkArgs->index;
        CKPT_NULL_CHECK(pOutXlation);
        pOutXlation->cksum  = pInXlation->cksum;
        pOutXlation->mastHdl = pInXlation->mastHdl;
        saNameCopy(&pOutXlation->name, &pInXlation->name);
    }
    return rc;
}



/*
 * Function for packing replica list.
 */
 
ClRcT _ckptMasterReplicaListPack(ClCntKeyHandleT    userKey,
                                 ClCntDataHandleT   hashTable,
                                 ClCntArgHandleT    userArg,
                                 ClUint32T          dataLength)
{
    ClUint32T         *replicaInfo  = (ClUint32T *)userArg; 
    
    replicaInfo = replicaInfo + gRepListCnt;
    CKPT_NULL_CHECK(replicaInfo);
    gRepListCnt++;
    *replicaInfo = (ClIocNodeAddressT)(ClWordT)userKey; 
    
    return CL_OK;
}



/*
 * Function for packing master handles.
 */
 
ClRcT _ckptMasterHdlPack(ClCntKeyHandleT    userKey,
                         ClCntDataHandleT   hashTable,
                         ClCntArgHandleT    userArg,
                         ClUint32T          dataLength)
{ 
    ClRcT               rc        = CL_OK;
    ClCkptMasterSyncupWalkT *pWalkArgs = userArg;
    ClHandleT          *pNodeInfo = (ClHandleT *)pWalkArgs->pData;
    
    if(pWalkArgs->index < pWalkArgs->count)
    {
        pNodeInfo = pNodeInfo + pWalkArgs->index;
        ++pWalkArgs->index;
        CKPT_NULL_CHECK(pNodeInfo);
        (*pNodeInfo) = *(ClHandleT *)userKey;
    }
    return rc;
}



/*
 * Function for packing client and master handles in peer list.
 */
 
ClRcT _ckptMasterPeerListHdlPack(ClCntKeyHandleT    userKey,
                                 ClCntDataHandleT   hashTable,
                                 ClCntArgHandleT    userArg,
                                 ClUint32T          dataLength)
{ 
    ClRcT               rc         = CL_OK;
    CkptNodeListInfoT   *pData     = (CkptNodeListInfoT *)hashTable;
    ClCkptMasterSyncupWalkT *pWalkArgs = userArg;
    CkptNodeListInfoT   *pNodeInfo = (CkptNodeListInfoT *)pWalkArgs->pData;

    if(pWalkArgs->index < pWalkArgs->count)
    {
        pNodeInfo = pNodeInfo + pWalkArgs->index;
        ++pWalkArgs->index;
    
        CKPT_NULL_CHECK(pData);
        CKPT_NULL_CHECK(pNodeInfo);
    
        pNodeInfo->clientHdl  = pData->clientHdl;
        pNodeInfo->appAddr    = pData->appAddr;
        pNodeInfo->appPortNum = pData->appPortNum; 
    }    
    return rc;
}



/*
 * Function for packing peer list.
 */
 
ClRcT _ckptMasterPeerListPack(ClCntKeyHandleT    userKey,
                              ClCntDataHandleT   hashTable,
                              ClCntArgHandleT    userArg,
                              ClUint32T          dataLength)
{
    ClRcT               rc             = CL_OK;
    CkptPeerInfoT       *pPeerInfo     = (CkptPeerInfoT *) hashTable;
    ClCkptMasterSyncupWalkT *pWalkArgs = userArg;
    CkptPeerListInfoT   *pPeerListInfo = (CkptPeerListInfoT *)pWalkArgs->pData;

    if(pWalkArgs->index < pWalkArgs->count)
    {
        ClCkptMasterSyncupWalkT walkArgs = {0};
        pPeerListInfo = pPeerListInfo + pWalkArgs->index;
        ++pWalkArgs->index;
        CKPT_NULL_CHECK(pPeerListInfo);
        CKPT_NULL_CHECK(pPeerInfo);

        pPeerListInfo->nodeAddr    = pPeerInfo->addr;
        pPeerListInfo->credential  = pPeerInfo->credential;
        pPeerListInfo->available   = pPeerInfo->available;
    
        /* 
         * Pack the no. of handle per peer first.
         */
        if(pPeerInfo->ckptList != 0)
        {
            rc = clCntSizeGet(pPeerInfo->ckptList, &pPeerListInfo->numOfOpens);
            CKPT_ERR_CHECK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
                           ("Ckpt: Failed to get the size of ckptList rc[0x %x]\n", rc), rc);
          
            pPeerListInfo->nodeListInfo = (CkptNodeListInfoT *)clHeapCalloc(1, 
                                                                            sizeof(CkptNodeListInfoT) *
                                                                            pPeerListInfo->numOfOpens);
            if(pPeerListInfo->nodeListInfo == NULL)
            {
                rc = CL_CKPT_ERR_NO_MEMORY;
                CKPT_ERR_CHECK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
                               ("Ckpt: Failed to allocate the memory rc[0x %x]\n", rc), rc);
            }

            /* 
             * Pack the handles.
             */
            gClntHdlCnt = 0;
            walkArgs.count = pPeerListInfo->numOfOpens;
            walkArgs.index = 0;
            walkArgs.pData = (ClPtrT)pPeerListInfo->nodeListInfo;
            rc = clCntWalk(pPeerInfo->ckptList, _ckptMasterPeerListHdlPack,
                           (ClCntArgHandleT) &walkArgs,
                           (ClUint32T)sizeof(walkArgs));
            gClntHdlCnt = 0;
        }
        else
        {
            pPeerListInfo->numOfOpens = 0;
            pPeerListInfo->nodeListInfo = NULL;
        }

        if(pPeerInfo->mastHdlList != 0)
        {
            rc = clCntSizeGet(pPeerInfo->mastHdlList, &pPeerListInfo->numOfHdl);
            CKPT_ERR_CHECK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
                           ("Ckpt: Failed to get the size of mastHdlList rc[0x %x]\n", rc), rc);
        
            pPeerListInfo->mastHdlInfo = (ClHandleT *)clHeapCalloc(1, 
                                                                   sizeof(ClHandleT) *
                                                                   pPeerListInfo->numOfHdl);
            if(pPeerListInfo->mastHdlInfo == NULL)
            {
                rc = CL_CKPT_ERR_NO_MEMORY;
                CKPT_ERR_CHECK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
                               ("Ckpt: Failed to allocate the memory rc[0x %x]\n", rc), rc);
            }        

            gMastHdlCnt = 0;
            walkArgs.index = 0;
            walkArgs.count = pPeerListInfo->numOfHdl;
            walkArgs.pData = (ClPtrT)pPeerListInfo->mastHdlInfo;
            rc = clCntWalk(pPeerInfo->mastHdlList, _ckptMasterHdlPack,
                           (ClCntArgHandleT) &walkArgs,
                           (ClUint32T)sizeof(walkArgs));
            gMastHdlCnt = 0;
        }
        else
        {
            pPeerListInfo->numOfHdl = 0;
            pPeerListInfo->mastHdlInfo = NULL;
        }
    }
    exitOnError:
    return rc;
}



/*
 * Function for packing client DB entry.
 */
 
ClRcT   ckptClientDBEntryPack(ClHandleDatabaseHandleT databaseHandle,
                              ClHandleT               handle, 
                              void                    *pCookie)
{
    ClCkptMasterSyncupWalkT *pWalkArgs = pCookie;
    CkptMasterDBClientInfoT *pClientInfo = (CkptMasterDBClientInfoT *)pWalkArgs->pData;
    CkptMasterDBClientInfoT *pClientData = NULL;
    ClRcT                    rc          = CL_OK;

    if(pWalkArgs->index < pWalkArgs->count)
    {
        pClientInfo = pClientInfo + pWalkArgs->index;
        ++pWalkArgs->index;
        rc = clHandleCheckout( gCkptSvr->masterInfo.clientDBHdl,
                               handle,(void **)&pClientData);
        CKPT_ERR_CHECK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
                       ("Ckpt: Failed to allocate the memory rc[0x %x]\n", rc), rc);
            
        if(pClientData == NULL || pClientInfo == NULL)
        {
            rc = CL_CKPT_ERR_INVALID_STATE;
            clHandleCheckin(gCkptSvr->masterInfo.clientDBHdl,handle);
            CKPT_ERR_CHECK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
                           ("Ckpt:Proper data is not there rc[0x %x]\n", rc), rc);
        }
    
        pClientInfo->clientHdl = handle;        
        pClientInfo->masterHdl = pClientData->masterHdl;        
        clLogDebug(CL_CKPT_AREA_MASTER, CL_CKPT_CTX_DEP_SYNCUP, 
                   "Packing open handle [%#llX] for masterHandle [%#llX]", 
                   handle, pClientInfo->masterHdl);
        clHandleCheckin(gCkptSvr->masterInfo.clientDBHdl, handle);
    }
    exitOnError:
    return rc;
}



/*
 * Function for packing a given checkpoint's metadata .
 */
 
ClRcT  ckptMasterDBEntryCopy(ClHandleT             handle,
                             CkptMasterDBEntryIDLT *pMasterEntry)
{
    ClRcT                  rc               = CL_OK;
    CkptMasterDBEntryT     *pStoredData     = NULL;
    ClUint32T              *replicaListInfo = NULL;
    ClCntNodeHandleT       nodeHdl          = 0;
    ClCntDataHandleT       dataHdl          = 0;
    ClIocNodeAddressT      nodeAddr         = 0;

    /*
     * Retrieve the info associated with the master handle.
     */
    rc = clHandleCheckout(gCkptSvr->masterInfo.masterDBHdl,
            handle,(void **)&pStoredData);
    CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
            ("Ckpt: Failed to allocate the memory rc[0x %x]\n", rc), rc);
            
    if(pStoredData == NULL || pMasterEntry == NULL)
    {
       rc = CL_CKPT_ERR_INVALID_STATE;
       CKPT_ERR_CHECK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
            ("Ckpt:Proper data is not there rc[0x %x]\n", rc), rc);
    }
    
    /*
     * Copy the data.
     */
    saNameCopy(&pMasterEntry->name, &pStoredData->name);
    memcpy(&pMasterEntry->attrib, &pStoredData->attrib,
            sizeof(ClCkptCheckpointCreationAttributesT));
    pMasterEntry->ckptMasterHdl     = handle;
    pMasterEntry->refCount          = pStoredData->refCount;
    pMasterEntry->markedDelete      = pStoredData->markedDelete;
    pMasterEntry->activeRepHdl      = pStoredData->activeRepHdl;
    pMasterEntry->activeRepAddr     = pStoredData->activeRepAddr;
    pMasterEntry->prevActiveRepAddr = pStoredData->prevActiveRepAddr;
    
    /* 
     * Pack the no. size of replica list first.
     */
    if(pStoredData->replicaList != 0)
    {
        ClUint32T replicaCount = 0;
        rc = clCntSizeGet(pStoredData->replicaList,
                        &pMasterEntry->replicaCount);
        replicaListInfo = (ClUint32T *)clHeapAllocate(
                             sizeof(ClUint32T) * pMasterEntry->replicaCount);
        if(replicaListInfo == NULL)
        {
             rc = CL_CKPT_ERR_NO_MEMORY;
             CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
            ("Ckpt: Failed to allocate the memory rc[0x %x]\n", rc), rc);
        }   
        pMasterEntry->replicaListInfo = replicaListInfo;
        clCntFirstNodeGet(pStoredData->replicaList,&nodeHdl);
        while(nodeHdl != 0 
              && 
              replicaCount++ < pMasterEntry->replicaCount)
        {
            rc = clCntNodeUserDataGet(pStoredData->replicaList,nodeHdl, &dataHdl);
            CL_ASSERT(CL_OK == rc);
            nodeAddr = (ClIocNodeAddressT)(ClWordT) dataHdl;
            clCntNextNodeGet(pStoredData->replicaList,nodeHdl,&nodeHdl);  
            *replicaListInfo = nodeAddr;               
             replicaListInfo++;
       }
       CKPT_ERR_CHECK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
            ("Ckpt:Proper data is not there rc[0x %x]\n", rc), rc);
    }
exitOnError:
    clHandleCheckin(gCkptSvr->masterInfo.masterDBHdl, handle);
exitOnErrorBeforeHdlCheckout:
    return rc;
}



/*
 * Function for packing master DB entry.
 */
 
ClRcT _ckptMasterDBEntryPack(ClHandleDatabaseHandleT databaseHandle,
                             ClHandleT               handle, 
                             void                    *pCookie)
{
    ClRcT                  rc              = CL_OK;
    ClCkptMasterSyncupWalkT *pWalkArgs     = pCookie;
    CkptMasterDBEntryIDLT  *pMasterEntry   = (CkptMasterDBEntryIDLT *)pWalkArgs->pData;

    if(pWalkArgs->index < pWalkArgs->count)
    {
        pMasterEntry = pMasterEntry + pWalkArgs->index;
        rc = ckptMasterDBEntryCopy(handle,pMasterEntry);
        ++pWalkArgs->index;
    }
    else
    {
        pMasterEntry = clHeapRealloc(pMasterEntry, sizeof(*pMasterEntry) * (pWalkArgs->count + 1));
        CL_ASSERT(pMasterEntry != NULL);
        pMasterEntry = pMasterEntry + pWalkArgs->index;
        memset(pMasterEntry, 0, sizeof(*pMasterEntry));
        ++pWalkArgs->index;
        ++pWalkArgs->count;
        rc = ckptMasterDBEntryCopy(handle, pMasterEntry);
        clLogNotice("MASTER", "PACK", "Master db extended to index [%d]", pWalkArgs->index);
    }
    return rc;
}



/*
 * Function to pack the metadata to send it across to the requesting node.
 */
 
ClRcT VDECL_VER(clCkptDeputyMasterInfoSyncup, 4, 0, 0)(ClVersionT              *pVersion,
                                   ClUint32T               *pCkptCount,
                                   CkptXlationDBEntryT     **ppXlationEntry,
                                   CkptMasterDBInfoIDLT    *pMasterInfo,
                                   ClUint32T               *pMastHdlCount,
                                   CkptMasterDBEntryIDLT   **ppMasterDBInfo,
                                   ClUint32T               *pPeerCount,
                                   CkptPeerListInfoT       **ppPeerListInfo,
                                   ClUint32T               *pClntHdlCount,
                                   CkptMasterDBClientInfoT **ppClientDBInfo)
{
    ClRcT   rc = CL_OK; 
    ClCkptMasterSyncupWalkT walkArgs = { 0 };

    /*
     * Chech whether the server is up or not.
     */
    CL_CKPT_SVR_EXISTENCE_CHECK;
    
    /*
     * Lock the master DB.
     */
    CKPT_LOCK(gCkptSvr->masterInfo.ckptMasterDBSem);

    /*
     * Verify the version.
     */
    rc = clVersionVerify(&gCkptSvr->versionDatabase,pVersion);
    CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
            ("Ckpt: MasterDatabasePack failed, "
            "versionMismatch rc[0x %x]\n", rc), rc);
            
    /*
     * Pack the name translation table.
     */        
    rc = clCntSizeGet(gCkptSvr->masterInfo.nameXlationDBHdl,pCkptCount);
    CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
       ("Ckpt: Failed to get the size of Xlation Table rc[0x %x]\n", rc), rc);
    *ppXlationEntry = (CkptXlationDBEntryT *)clHeapAllocate(
                              sizeof(CkptXlationDBEntryT) *(*pCkptCount));  
     if(*ppXlationEntry == NULL)
     {
        rc = CL_CKPT_ERR_NO_MEMORY;
        CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
            ("Ckpt: Failed to allocate the memory rc[0x %x]\n", rc), rc);
     }
     gXlationCnt = 0;
     walkArgs.pData = (ClPtrT)*ppXlationEntry;
     walkArgs.index = 0;
     walkArgs.count = *pCkptCount;
     rc = clCntWalk( gCkptSvr->masterInfo.nameXlationDBHdl,
                     _ckptMasterXlationTablePack,
                     (ClCntArgHandleT)&walkArgs,
                     (ClUint32T)sizeof(walkArgs));

    gXlationCnt = 0;
    CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
            ("Ckpt: Failed to pack the Xlation Table rc[0x %x]\n", rc), rc);
            
    /*
     * Pack the master Related Entries.
     */        
    pMasterInfo->masterAddr    = gCkptSvr->masterInfo.masterAddr;
    pMasterInfo->deputyAddr    = gCkptSvr->masterInfo.deputyAddr;
    pMasterInfo->compId        = gCkptSvr->masterInfo.compId;
    
    /*
     * Pack Master DB entries.
     */
    *pMastHdlCount  = gCkptSvr->masterInfo.masterHdlCount; 
    *ppMasterDBInfo = (CkptMasterDBEntryIDLT *)clHeapAllocate(
                                           sizeof(CkptMasterDBEntryIDLT) *
                                        gCkptSvr->masterInfo.masterHdlCount);
     if(*ppMasterDBInfo == NULL)
     {
        rc = CL_CKPT_ERR_NO_MEMORY;
        CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
            ("Ckpt: Failed to allocate the memory rc[0x %x]\n", rc), rc);
     }
     gMstEntryCnt= 0;
     walkArgs.pData = (ClPtrT)*ppMasterDBInfo;
     walkArgs.index = 0;
     walkArgs.count = gCkptSvr->masterInfo.masterHdlCount;
     rc = clHandleWalk( gCkptSvr->masterInfo.masterDBHdl,
                       _ckptMasterDBEntryPack,
                        (ClPtrT)&walkArgs);
     if(walkArgs.count > gCkptSvr->masterInfo.masterHdlCount)
     {
         gCkptSvr->masterInfo.masterHdlCount = *pMastHdlCount = walkArgs.count;
     }
     gMstEntryCnt = 0;
     CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
            ("Ckpt: Failed to allocate the memory rc[0x %x]\n", rc), rc);
            
     /*
      * Pack the peerList Entries.
      */       
     rc = clCntSizeGet(gCkptSvr->masterInfo.peerList,pPeerCount); 
     CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
            ("Ckpt: Failed to allocate the memory rc[0x %x]\n", rc), rc);
     *ppPeerListInfo = (CkptPeerListInfoT *)clHeapAllocate(
                         sizeof(CkptPeerListInfoT) * (*pPeerCount));
     if(*ppPeerListInfo == NULL)
     {
        rc = CL_CKPT_ERR_NO_MEMORY;
        CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
            ("Ckpt: Failed to allocate the memory rc[0x %x]\n", rc), rc);
     }
     gPeerListCnt = 0;
     walkArgs.count = *pPeerCount;
     walkArgs.index = 0;
     walkArgs.pData = (ClPtrT)*ppPeerListInfo;
     rc = clCntWalk(gCkptSvr->masterInfo.peerList, _ckptMasterPeerListPack,
                    (ClCntArgHandleT)&walkArgs, 
                    (ClUint32T)sizeof(walkArgs));
     gPeerListCnt = 0;
     CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
            ("Ckpt: Failed to pack the PeerInfo rc[0x %x]\n", rc), rc);

     /*
      * Pack the client DB and the Entries.
      */       
     *pClntHdlCount = gCkptSvr->masterInfo.clientHdlCount;
     clLogDebug(CL_CKPT_AREA_MASTER, CL_CKPT_CTX_DEP_SYNCUP, 
             "Num of opened handles [%d]", *pClntHdlCount);
     *ppClientDBInfo = (CkptMasterDBClientInfoT *)clHeapCalloc(1,
        sizeof(CkptMasterDBClientInfoT) * 
        gCkptSvr->masterInfo.clientHdlCount);
     if(*ppClientDBInfo == NULL)
     {
        rc = CL_CKPT_ERR_NO_MEMORY;
        CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
            ("Ckpt: Failed to allocate the memory rc[0x %x]\n", rc), rc);
     }
     gCltEntryCnt = 0;
     walkArgs.index = 0;
     walkArgs.count = gCkptSvr->masterInfo.clientHdlCount;
     walkArgs.pData = (ClPtrT)*ppClientDBInfo;
     rc = clHandleWalk(gCkptSvr->masterInfo.clientDBHdl,
                       ckptClientDBEntryPack,
                       (ClPtrT)&walkArgs);
     gCltEntryCnt = 0;
     CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
          ("Ckpt: Failed to pack the client DB Entries rc[0x %x]\n", rc), rc);
exitOnErrorBeforeHdlCheckout:
    /*
     * Unlock the master DB.
     */
    CKPT_UNLOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
    return rc;
}                                   



/*
 * Function for deleting the node from checkpoint's replica list.
 */
 
ClRcT ckptMasterReplicaListDelete(ClCntKeyHandleT    userKey,
                                  ClCntDataHandleT   hashTable,
                                  ClCntArgHandleT    userArg,
                                  ClUint32T          dataLength)
{
    ClRcT                   rc            = CL_OK;
    CkptXlationDBEntryT     *pXlation     = (CkptXlationDBEntryT *)hashTable;
    ClIocNodeAddressT       peerAddr      = *(ClIocNodeAddressT *)userArg; 
    CkptMasterDBEntryT      *pStoredData  = NULL;
    ClCntNodeHandleT        pNodeHandle   = 0;

    if(pXlation != NULL)
    {
        /*
         * Retrieve the information associated with the master hdl.
         */
        rc = clHandleCheckout(gCkptSvr->masterInfo.masterDBHdl,
                pXlation->mastHdl, (void **)&pStoredData);
        if( CL_OK == rc && pStoredData != NULL)
        {
            rc = clCntNodeFind(pStoredData->replicaList, (ClPtrT)(ClWordT)peerAddr,
                               &pNodeHandle);
            if(rc == CL_OK)
            {
                /*
                 * Delete the entry.
                 */
                clLogDebug("REP", "DELETE", "Deleting the replica address [%d] for checkpoint [%.*s]",
                            peerAddr, pXlation->name.length, pXlation->name.value);
                clCntNodeDelete(pStoredData->replicaList, pNodeHandle);
            }   
            rc = clHandleCheckin(gCkptSvr->masterInfo.masterDBHdl, 
                                 pXlation->mastHdl);
        }    
    }
    return rc;
}   



/*
 * For all the checkpoints where replica is stored in the passed 
 * node adddress, delete the node address from the checkpoints' 
 * replica list.
 */
 
ClRcT clCkptMasterReplicaListUpdateNoLock(ClIocNodeAddressT peerAddr)
{

    /*
     * Walk through the name Xlation table and delete teh node from
     * checkpoint's replica list.
     */
    clCntWalkFailSafe(gCkptSvr->masterInfo.nameXlationDBHdl, 
                      ckptMasterReplicaListDelete,
                      &peerAddr,sizeof(ClIocNodeAddressT));
              
    return CL_OK;
}

ClRcT clCkptMasterReplicaListUpdate(ClIocNodeAddressT peerAddr)
{
    ClRcT rc = CL_OK;
    CKPT_LOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
    rc = clCkptMasterReplicaListUpdateNoLock(peerAddr);
    CKPT_UNLOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
    return rc;
}

/*
 * Function gets called whenever the master receives "hello" from other
 * checkpoint servers when they come up as well as master restart.
 * It checks whether the passed checkpoint was of COLLOCATED  
 * in nature that the passed node was active replica for the 
 * collocated checkpoint. If yes that node is again made the active replica
 * and an event is raised for the same to inform the checkpoint clients.
 * Aslo the passed address asked to pull the given checkpoint from the 
 * replica node.
 */
 
ClRcT
_ckptMastHdlListWalk(ClCntKeyHandleT   userKey,
                     ClCntDataHandleT  pData,
                     ClCntArgHandleT   userArg,
                     ClUint32T         size)
                  
{
    ClHandleT            masterHdl    = *(ClHandleT *)userKey;
    CkptMasterDBEntryT   *pStoredData = NULL;
    ClIocNodeAddressT    replicaAddr  = CL_CKPT_UNINIT_ADDR;
    ClIocNodeAddressT    peerAddr     = (ClIocNodeAddressT)(ClWordT)userArg;
    ClCntNodeHandleT     nodeHdl      = 0;
    ClCntDataHandleT     dataHdl      = 0;
    ClRcT                rc           = CL_OK;
    ClCkptClientUpdInfoT eventInfo    = {0};
    ClEventIdT           eventId      = 0;

    /*
     * Checkout the checkpoint's metadata from the master DB.
     */
    rc = ckptSvrHdlCheckout(gCkptSvr->masterInfo.masterDBHdl,
            masterHdl,
            (void **)&pStoredData);
    CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
            ("Failed to sync up the info  rc[%#x]\n", rc), rc);
    if( (CL_CKPT_IS_COLLOCATED(pStoredData->attrib.creationFlags)) &&
            (pStoredData->activeRepAddr == CL_CKPT_UNINIT_ADDR) &&
            (pStoredData->prevActiveRepAddr == peerAddr))
    {
        /*
         * Make the node the active replica for the checkpoint.
         */
        pStoredData->prevActiveRepAddr = CL_CKPT_UNINIT_ADDR;
        pStoredData->activeRepAddr     = peerAddr;

        /*
         * Add the masterHdl to the node's peerlist.
         */
        _ckptPeerListMasterHdlAdd(masterHdl, pStoredData->prevActiveRepAddr,
                peerAddr);
                
        /*
         * Publish an event to inform client about the active 
         * address change.
         */
        memset(&eventInfo, 0, sizeof(ClCkptClientUpdInfoT));
        eventInfo.eventType = htonl(CL_CKPT_ACTIVE_REP_CHG_EVENT);
        eventInfo.actAddr   = htonl(pStoredData->activeRepAddr);
        saNameCopy(&eventInfo.name, &pStoredData->name);
        eventInfo.name.length = htons(pStoredData->name.length);
	clLogInfo(CL_CKPT_AREA_MASTER, CL_CKPT_CTX_ACTADDR_SET,
		  "Changing the active address from [%d] to [%d] for checkpoint [%.*s]",
		  pStoredData->prevActiveRepAddr, pStoredData->activeRepAddr, pStoredData->name.length,
	          pStoredData->name.value);
        rc = clEventPublish(gCkptSvr->clntUpdEvtHdl, 
                (const void*)&eventInfo,
                sizeof(ClCkptClientUpdInfoT), &eventId);

        /* 
         * Inform the deputy master as well.
         */
        {
            CkptUpdateInfoT   updateInfo = {0};

            /*
             * Update the active address in the master.
             */
            updateInfo.discriminant = CKPTUPDATEINFOTACTIVEADDR;
            updateInfo.ckptUpdateInfoT.activeAddr 
                                    = pStoredData->activeRepAddr;

            rc = ckptDynamicDeputyUpdate(CL_CKPT_COMM_UPDATE,
                    CL_CKPT_UNINIT_VALUE,
                    masterHdl,
                    &updateInfo);
        }    
    }

    /*
     * Ckpt info needs to be pulled in case of COLLOCATED ckpts only
     */
    if (CL_CKPT_IS_COLLOCATED(pStoredData->attrib.creationFlags) ||
       (pStoredData->attrib.creationFlags & CL_CKPT_DISTRIBUTED) )
    {
        /* 
         * Get the replica node.
         */
        ClUint32T size = 0;
        clCntSizeGet(pStoredData->replicaList, &size);
        clCntFirstNodeGet(pStoredData->replicaList, &nodeHdl);
        while(nodeHdl != 0)
        {
            rc = clCntNodeUserDataGet(pStoredData->replicaList,nodeHdl, &dataHdl);
            CL_ASSERT(CL_OK == rc);
            replicaAddr = (ClIocNodeAddressT)(ClWordT)dataHdl;
            if(replicaAddr == peerAddr)
            {
                if(CL_OK != (rc = clCntNextNodeGet(pStoredData->replicaList,
                                            nodeHdl, &nodeHdl)))
                {
                    replicaAddr = CL_CKPT_UNINIT_ADDR;
                    break;
                }
            }
            else
            {
                break;
            }
        }   
        
        if(replicaAddr != CL_CKPT_UNINIT_ADDR)
        {
            /*
             * Ask the node to pull the checkpoint 
             * from the replica node.
             */
            rc = _ckptReplicaIntimation( peerAddr,
                    replicaAddr, masterHdl, 
                    CL_CKPT_DONT_RETRY,
                    CL_CKPT_ADD_TO_MASTHDL_LIST);
            CKPT_ERR_CHECK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
                    ("Failed to add ckptInfo to replica" 
                     "[0x %x]\n",rc),rc);
        }        
    }
exitOnError:
    /*
     * Checkin the checkpoint's metadata.
     */
    clHandleCheckin(gCkptSvr->masterInfo.masterDBHdl,masterHdl);
exitOnErrorBeforeHdlCheckout:    
    return rc;

}



/*
 * Timer expiry function that will go through the list of checkpoint and 
 * see whether they need to be replicated or not and take appropriate action.
 * This timer is scheduled a few seconds after a ckpt server has contacted
 * the master.
 */

ClRcT _ckptMasterCkptsReplicateTimerExpiry(void *pArg)
{
    ClCkptReplicateTimerArgsT *pTimerArg = pArg;
    ClIocNodeAddressT addr = CL_IOC_RESERVED_ADDRESS;

    CL_ASSERT(pTimerArg != NULL);

    addr = pTimerArg->nodeAddress;

    clHeapFree(pTimerArg);

    if( (gCkptSvr == NULL) || ( (gCkptSvr != NULL) && (gCkptSvr->serverUp ==
                    CL_FALSE)) )
    {
        goto exitOnError;
    }
     CKPT_LOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
     clLogDebug(CL_CKPT_AREA_MASTER, CL_CKPT_CTX_REPL_NOTIFY,
		"Timer expired for balancing the checkpoints for address [%d]", 
		 addr);
     clHandleWalk(gCkptSvr->masterInfo.masterDBHdl,
                  _ckptMasterCkptsReplicate, &addr);
     CKPT_UNLOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
exitOnError:
     return CL_OK;
}



/* 
 * Function received by master (only) whenever a checkpoint server
 * on any node comes up.
 */
 
ClRcT    VDECL_VER(clCkptRemSvrWelcome, 4, 0, 0)(ClVersionT         *pVersion,
                                                 ClIocNodeAddressT  peerAddr,
                                                 ClUint8T           credential)
{
    ClRcT           rc         = CL_OK;
    CkptPeerInfoT   *pPeerInfo = NULL;
    ClVersionT      version    = {0};
    ClTimerTimeOutT timeOut    = {0}; 
    ClCkptReplicateTimerArgsT *pTimerArgs = NULL;
    ClIocNodeAddressT masterAddr = gCkptSvr->masterInfo.masterAddr;

    /*
     * Version verification.
     */
    rc = clVersionVerify(&gCkptSvr->versionDatabase,pVersion);
    if(rc != CL_OK)
    {
        /*TODO nack send has to be uncommented*/
        //       rc = clCkptNackSend(*pVersion,CKPT_PEER_HELLO);
        rc = CL_OK;
        return rc;
    }
    
    /* 
     * Ensure that this function is only executed by master only.
     */
    if( masterAddr == gCkptSvr->localAddr)
    {
        clLogNotice(CL_CKPT_AREA_MASTER, CL_CKPT_CTX_PEER_WELCOME, 
                    "Welcoming ckpt server [%d] ...", peerAddr);
        /*
         * Update the master's peerlist. Mark the entry as
         * AVAILABLE.
         */
        rc = clCkptMasterPeerUpdate(0, CL_CKPT_SERVER_UP,
                                    peerAddr,
                                    credential); 

        if(rc != CL_OK)
        {
            clLogError("PEER", "UPD", "Updating peer [%d] during welcome failed with [%#x]",
                       peerAddr, rc);
            return rc;
        }

        /*
         * Update the deputy about the new peer node that has come up.
         */
        clLogDebug(CL_CKPT_AREA_MASTER, CL_CKPT_CTX_PEER_WELCOME,
                   "Updating all the deputy's peer list");

        rc = ckptIdlHandleUpdate(CL_IOC_BROADCAST_ADDRESS,
                                 gCkptSvr->ckptIdlHdl, 0);
 
        memcpy(&version, gCkptSvr->versionDatabase.versionsSupported,
               sizeof(ClVersionT));
        rc = VDECL_VER(clCkptDeputyPeerListUpdateClientAsync, 4, 0, 0)(gCkptSvr->ckptIdlHdl, 
                                                                       version, peerAddr, credential,
                                                                       NULL, NULL);
        /* 
         * Go through the masterHdl list associated with server coming
         * up. In case that server was active replica of any collocated 
         * ckpt ask that server to pull the related checkpoint information 
         * from replica nodes.
         */
        CKPT_LOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
                
        rc = clCntDataForKeyGet(gCkptSvr->masterInfo.peerList,
                                (ClPtrT)(ClWordT)peerAddr, (ClCntDataHandleT *)&pPeerInfo);    
        CKPT_ERR_CHECK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
                       ("Ckpt: peerAddr %d does not exist in peerList. rc[0x %x]\n",
                        peerAddr, rc), rc);
      
        if(pPeerInfo->mastHdlList != 0)
        {
    	    clLogDebug(CL_CKPT_AREA_MASTER, CL_CKPT_CTX_PEER_WELCOME, 
                       "Going through the master handle list for address [%d]", 
                       peerAddr);
            clCntWalk(pPeerInfo->mastHdlList, _ckptMastHdlListWalk, 
                      (ClPtrT)(ClWordT)peerAddr, sizeof(peerAddr));  
        }            

        /*
         * Check whether any checkpoint needs to be replicated.
         * It is possible that when a checkpoint was created no 
         * other node capable for replicating the info was present.
         * The other scenario is that the node holding the replica 
         * info has gone down. Now that a node is available, replicate
         * the checkpoint is needed.
         */
        pTimerArgs = clHeapCalloc(1, sizeof(*pTimerArgs));
        if(!pTimerArgs)
        {
            rc = CL_CKPT_RC(CL_ERR_NO_MEMORY);
        }
        CKPT_ERR_CHECK(CL_CKPT_SVR, CL_LOG_SEV_ERROR,
                       ("Failed to allocate memory\n"),
                       rc);
        pTimerArgs->nodeAddress = peerAddr;
    	clLogDebug(CL_CKPT_AREA_MASTER, CL_CKPT_CTX_PEER_WELCOME, 
                   "Starting the timer to balance the checkpoints for address [%d]", 
                   peerAddr);
    	memset(&timeOut, 0, sizeof(ClTimerTimeOutT));
	    timeOut.tsSec = 2;
    	rc = clTimerCreateAndStart(timeOut,
                                   CL_TIMER_VOLATILE,
                                   CL_TIMER_SEPARATE_CONTEXT,
                                   _ckptMasterCkptsReplicateTimerExpiry,
                                   (ClPtrT)pTimerArgs,
                                   &gClPeerReplicateTimer);
        gClPeerReplicateTimer = 0;
    }       
    else
    {
        /*TODO : throw ckptError,so that sender has to handle resend to correct guy */
    }
    exitOnError:
    /*
     * Unlock the master DB.
     */
    if(masterAddr == gCkptSvr->localAddr)
        CKPT_UNLOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
    return rc;
}                              

ClRcT VDECL_VER(clCkptRemSvrWelcome, 5, 1, 0)(ClVersionT         *pVersion,
                                              ClIocNodeAddressT  peerAddr,
                                              ClUint8T           credential)
{
    return VDECL_VER(clCkptRemSvrWelcome, 4, 0, 0)(pVersion, peerAddr, credential);
}

/*
 * Function checks whether replication is needed for the checkpoint or not.
 * If yes the ckpt server running on the passed node address pulls the 
 * checkpoint from the active replica.
 */
 
ClRcT _ckptMasterCkptsReplicate(ClHandleDatabaseHandleT databaseHandle,
                                ClHandleT               masterHdl, 
                                void                    *pData)
{
    ClIocNodeAddressT    peerAddr      = *(ClIocNodeAddressT *)pData;
    ClUint32T            replicaCount  = 0;
    ClRcT                rc            = CL_OK;
    CkptMasterDBEntryT   *pStoredData  = NULL; 
    ClCntNodeHandleT     nodeHdl       = 0;
    ClCntDataHandleT     dataHdl       = 0;
    ClIocNodeAddressT    tempAddr      = 0;


    /*
     * Checkout the checkpoint's metadata from the master DB.
     */
    rc = ckptSvrHdlCheckout(gCkptSvr->masterInfo.masterDBHdl,
                          masterHdl,
                          (void **)&pStoredData);
    CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
                ("Failed to sync up the info  rc[0x %x]\n", rc), rc);

    /*
     * Skip replicating collocated and distributed checkpoints since they are
     * replicated explicitly on open. For collocated, we use the global replicate on open
     * indicator before we decide to skip.
     */
    if( (pStoredData->attrib.creationFlags & CL_CKPT_DISTRIBUTED)
        ||
        (CKPT_COLLOCATE_REPLICA_ON_OPEN(gCkptSvr->replicationFlags) 
         && 
         CL_CKPT_IS_COLLOCATED(pStoredData->attrib.creationFlags)))
    {
        clLogNotice("REP", "BALANCE", "Skipping balancing of checkpoint [%.*s]. "
                    "Deferring replica allocation to ckpt open", 
                    pStoredData->name.length, pStoredData->name.value);
        goto exitOnError;
    }
                
    /*
     * Replicate the checkpoints that are not replicated. This is determined 
     * by looking at the no. of elements in the replica list of the ckpt.
     * no. of elements < 2 means no replica is present and hence replicate the
     * checkpoint.
     */ 
    rc = clCntSizeGet(pStoredData->replicaList, &replicaCount);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
                ("Replica list in not yet created  rc[0x %x]\n", rc), rc);

    if((replicaCount) && (replicaCount < 2))
    {
          /* 
           * Replication is required.
           */
           
          clCntFirstNodeGet(pStoredData->replicaList, &nodeHdl);
          rc = clCntNodeUserKeyGet(pStoredData->replicaList, nodeHdl, &dataHdl);
          if( CL_OK != rc )
          {
              /* No node available to replicated */
              clLogError(CL_CKPT_AREA_MASTER, CL_CKPT_CTX_PEER_WELCOME, 
                      "Data for node get failed rc[0x %x]", rc);
              goto exitOnError;
          }
          tempAddr = (ClIocNodeAddressT)(ClWordT)dataHdl;

          if((tempAddr != peerAddr) &&
             (tempAddr != CL_CKPT_UNINIT_ADDR))
          {
                /*
                 * Ask the node to pull the checkpoint 
                 * from the active replica.
                 */
	         	clLogDebug(CL_CKPT_AREA_MASTER, CL_CKPT_CTX_REPL_NOTIFY, 
			   "Notifing the new server [%d] to pull info from [%d]", 
			   peerAddr, tempAddr);
                rc = _ckptReplicaIntimation( peerAddr,
                            tempAddr,
                            masterHdl, 
                            CL_CKPT_DONT_RETRY,
                            !CL_CKPT_ADD_TO_MASTHDL_LIST);
                CKPT_ERR_CHECK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
                        ("Failed to add ckptInfo to replica" 
                         "[0x %x]\n",rc),rc);
           }

    }    
exitOnError:
    /*
     * Checkin the checkpoint's metadata.
     */
    clHandleCheckin(gCkptSvr->masterInfo.masterDBHdl,masterHdl);
exitOnErrorBeforeHdlCheckout:    
    return rc;
}


/*
 * Function to update the deputy and master addresses.
 */
 
ClRcT VDECL_VER(clCkptLeaderAddrUpdate, 4, 0, 0)(ClIocNodeAddressT masterAddr,
                                                 ClIocNodeAddressT deputyAddr)
{
    ClRcT rc = CL_OK;
    
    /*
     * Check whether the server is fully up or not.
     */
    CL_CKPT_SVR_EXISTENCE_CHECK; 

    /*
     * Lock the master DB.
     */
    clOsalMutexLock(&gCkptSvr->ckptClusterSem);
    CKPT_LOCK(gCkptSvr->masterInfo.ckptMasterDBSem);

    /*
     * Update the addresses.
     */
    gCkptSvr->masterInfo.deputyAddr = deputyAddr; 
    if( gCkptSvr->masterInfo.masterAddr != masterAddr )
    {
        gCkptSvr->masterInfo.prevMasterAddr = gCkptSvr->masterInfo.masterAddr;
        gCkptSvr->masterInfo.masterAddr = masterAddr;
    }
    
    /*
     * Unock the master DB.
     */
    CKPT_UNLOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
    clOsalMutexUnlock(&gCkptSvr->ckptClusterSem);

    return rc;
}


ClRcT _ckptMasterCkptsLoadBalance(ClHandleDatabaseHandleT databaseHandle,
                                  ClHandleT               masterHdl,
                                  void                    *pData)
{
    ClUint32T            replicaCount  = 0;
    ClRcT                rc            = CL_OK;
    CkptMasterDBEntryT   *pStoredData  = NULL;
    ClCntNodeHandleT     nodeHdl       = 0;
    ClCntNodeHandleT     dataHdl       = 0;
    ClIocNodeAddressT    tempAddr      = 0;
    ClIocNodeAddressT    replicaAddr   = 0;

    /*
     * Checkout the checkpoint's metadata from the master DB.
     */
    rc = ckptSvrHdlCheckout(gCkptSvr->masterInfo.masterDBHdl,
            masterHdl,
            (void **)&pStoredData);
    CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
            ("Failed to sync up the info  rc[0x %x]\n", rc), rc);

    /*
     * Skip load balancing collocated and distributed checkpoints since they are
     * replicated explicitly on open. For collocated, we use the global replicate on open
     * indicator before we decide to skip.
     */
    if( (pStoredData->attrib.creationFlags & CL_CKPT_DISTRIBUTED)
        ||
        (CKPT_COLLOCATE_REPLICA_ON_OPEN(gCkptSvr->replicationFlags) 
         && 
         CL_CKPT_IS_COLLOCATED(pStoredData->attrib.creationFlags)))
    {
        clLogNotice("LOAD", "BALANCE", "Skipping balancing of checkpoint [%.*s]. "
                    "Deferring replica allocation to ckpt open", 
                    pStoredData->name.length, pStoredData->name.value);
        goto exitOnError;
    }

    /*
     * Replicate the checkpoints that are not replicated. This is determined 
     * by looking at the no. of elements in the replica list of the ckpt.
     * no. of elements < 2 means no replica is present and hence replicate the
     * checkpoint.
     */ 
    rc = clCntSizeGet(pStoredData->replicaList, &replicaCount);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_LOG_SEV_ERROR,
            ("Replica list in not yet created  rc[0x %x]\n", rc), rc);

    if(replicaCount && replicaCount < 2)
    {
        rc = clCntFirstNodeGet(pStoredData->replicaList, &nodeHdl);
        if(nodeHdl != 0)
        {
            rc = clCntNodeUserDataGet(pStoredData->replicaList, nodeHdl, &dataHdl);
            CL_ASSERT(CL_OK == rc);
            replicaAddr = (ClIocNodeAddressT)(ClWordT)dataHdl;
        }
        tempAddr = replicaAddr;

        rc = ckptReplicaNodeGet(CL_CKPT_UNINIT_ADDR, &replicaAddr);

        if(replicaAddr != CL_CKPT_UNINIT_ADDR)
        {
            /*
             * Ask the chosen node to pull the ckptInfo
             * from active replica.No need of error checking,
             * even it fails,checkpoint server can proceed this
             * operation.
             */
            _ckptReplicaIntimation(replicaAddr, tempAddr,
                    masterHdl, CL_CKPT_DONT_RETRY,
                    !CL_CKPT_ADD_TO_MASTHDL_LIST);
        }
    }
exitOnError:
    /*
     * Checkin the updated stuff.
     */
    clHandleCheckin(gCkptSvr->masterInfo.masterDBHdl, masterHdl);
exitOnErrorBeforeHdlCheckout:    
    return rc;
}



ClRcT _ckptMasterLoadBalancingTimerExpiry(void *pData)
{
    ClTimerHandleT  timerHdl = *(ClTimerHandleT *) pData;

    if( (gCkptSvr == NULL) || ( (gCkptSvr != NULL) && (gCkptSvr->serverUp ==
                    CL_FALSE)) )
    {
        goto exitOnError;
    }
    CKPT_LOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
    clHandleWalk(gCkptSvr->masterInfo.masterDBHdl,
            _ckptMasterCkptsLoadBalance, NULL);
    CKPT_UNLOCK(gCkptSvr->masterInfo.ckptMasterDBSem);

exitOnError:    
    clTimerDelete(&timerHdl);
    clHeapFree(pData);
    return CL_OK;
}

ClRcT _ckptCheckpointLoadBalancing()
{
    ClRcT           rc         = CL_OK;
    ClTimerTimeOutT timeOut    = {0};
    ClTimerHandleT  timerHdl   = NULL;
    ClTimerHandleT  *pTimerArg = NULL;

    /*
     * This is for storing the timer handle and thatwill be deleted in the
     * callback 
     */
    pTimerArg = clHeapCalloc(1, sizeof(ClTimerHandleT));
    if( NULL == pTimerArg )
    {
        clLogCritical(CL_CKPT_AREA_MASTER, CL_CKPT_CTX_REPL_NOTIFY, 
                      "Failed to allocate memory");
        return CL_CKPT_ERR_NO_MEMORY;
    }
    memset(&timeOut, 0, sizeof(ClTimerTimeOutT));
    timeOut.tsSec = 2;
    rc = clTimerCreate(timeOut,
            CL_TIMER_ONE_SHOT,
            CL_TIMER_SEPARATE_CONTEXT,
            _ckptMasterLoadBalancingTimerExpiry,
            (ClPtrT)(ClWordT)pTimerArg,
            &timerHdl);
    if( CL_OK != rc )
    {
        clHeapFree(pTimerArg);
        clLogError(CL_CKPT_AREA_MASTER, CL_CKPT_CTX_REPL_NOTIFY, 
                      "Failed to create balancing timer rc[0x %x]", rc);
        return rc;
    }
    *pTimerArg = timerHdl;
    rc = clTimerStart(timerHdl);
    if( CL_OK != rc )
    {
        clTimerDelete(&timerHdl);
        clHeapFree(pTimerArg);
        clLogError(CL_CKPT_AREA_MASTER, CL_CKPT_CTX_REPL_NOTIFY, 
                      "Failed to start balancing timer rc[0x %x]", rc);
        return rc;
    }
            
    return rc;            
}


/*
 * Function to check whether ckpt replication is possible or not.
 */
 
ClRcT _ckptIsReplicationPossible(ClBoolT* pFlag)
{
    ClRcT     rc    = CL_OK;
    ClUint32T count = 0;

    *pFlag = CL_FALSE;
    
    rc = clCntSizeGet(gCkptSvr->masterInfo.peerList, &count);
    if(count <= 1)
    {
        return rc;
    }
    *pFlag = CL_TRUE;
    return rc;
}

ClRcT
clCkptAppInfoReplicaNotify(CkptMasterDBEntryT  *pMasterData,
                           ClHandleT           masterHdl,
                           ClIocNodeAddressT   appAddr,
                           ClIocPortT          appPortId)
{
    ClRcT              rc       = CL_OK;
    ClCntNodeHandleT   node     = CL_HANDLE_INVALID_VALUE;
    ClCntKeyHandleT    key      = CL_HANDLE_INVALID_VALUE;
    ClIocNodeAddressT  peerAddr = 0;

    rc = clCntFirstNodeGet(pMasterData->replicaList, &node); 
    if( CL_OK != rc )
    {
        clLogError(CL_CKPT_AREA_MASTER, CL_CKPT_CTX_CKPT_OPEN,
                   "No replica for checkpoint [%.*s] rc[0x %x]",
                   pMasterData->name.length, pMasterData->name.value, rc);
        return rc;
    }
    while( node != 0 )
    {
        rc = clCntNodeUserKeyGet(pMasterData->replicaList, node,
                                 (ClCntKeyHandleT *) &key);
        if( CL_OK != rc )
        {
            clLogError(CL_CKPT_AREA_MASTER, CL_CKPT_CTX_CKPT_OPEN,
             "Not able to get the key for node from replica list rc [0x %x]", 
             rc);
            return rc;
        }
        peerAddr = (ClIocNodeAddressT)(ClWordT)key;
        rc = ckptIdlHandleUpdate(peerAddr, gCkptSvr->ckptIdlHdl, 0); 
        if( CL_OK != rc )
        {
            clLogError(CL_CKPT_AREA_MASTER, CL_CKPT_CTX_CKPT_OPEN,
             "Idl handle update failed rc [0x %x]", 
             rc);
            return rc;
        }
        clLogDebug(CL_CKPT_AREA_MASTER, CL_CKPT_CTX_CKPT_OPEN, 
                   "Updating application info [%d:%d] on replica addr [%d]", 
                    appAddr, appPortId, peerAddr); 
        rc = VDECL_VER(clCkptReplicaAppInfoNotifyClientAsync, 4, 0, 0)(gCkptSvr->ckptIdlHdl, 
                                                   masterHdl, appAddr,
                                                   appPortId, NULL, NULL);
        if( CL_OK != rc )
        {
            clLogError(CL_CKPT_AREA_MASTER, CL_CKPT_CTX_CKPT_OPEN,
             "Idl call to clCkptAppInfoReplicaUpdate failed rc [0x %x]", 
             rc);
            return rc;
        }
        rc = clCntNextNodeGet(pMasterData->replicaList, node, &node);
        if( CL_OK != rc )
        {
            break;
        }
    }

    return CL_OK;
}

/*
 * This function will be invoked after the the selected replica node
 * has pulled the checkpoint from the active server.
 */ 
 
void clCkptReplicaCopyCallback( ClIdlHandleT       ckptIdlHdl,
                                ClVersionT         *pVersion,
                                ClHandleT          ckptActHdl,
                                ClIocNodeAddressT  activeAddr,
                                ClRcT              rc,
                                void               *pData)
{
    CkptMasterDBEntryT      *pMasterDBEntry = NULL;
    CkptPeerInfoT           *pPeerInfo      = NULL;
    ClCkptReplicateInfoT    *pRepInfo       = (ClCkptReplicateInfoT *) pData;
    ClIocNodeAddressT        peerAddr       = 0; 
    CkptHdlDbT              hdlDbInfo       = {0};
    ClRcT                   retCode         = rc;

    if( NULL == pRepInfo ) 
    {
        clLogCritical(CL_CKPT_AREA_MASTER, CL_CKPT_CTX_REPL_NOTIFY, 
                      "Cookie is NULL in replica copy callback rc [0x %x]", rc);
        CL_ASSERT(CL_FALSE);
        return;
    }
    peerAddr = pRepInfo->destAddr;
    clLogDebug(CL_CKPT_AREA_MASTER, CL_CKPT_CTX_REPL_NOTIFY,
               "Replica copy callback RetCode [%x] tryOtherNode [%d]"
               "peerAddr [%d]", rc, pRepInfo->tryOtherNode, peerAddr);
    hdlDbInfo.ckptMasterHdl = pRepInfo->clientHdl;
    hdlDbInfo.activeAddr    = pRepInfo->activeAddr;
    hdlDbInfo.ckptActiveHdl    = pRepInfo->activeHdl;
    hdlDbInfo.creationFlag = pRepInfo->creationFlags;
    if( (pVersion->releaseCode != '\0')  && (CL_OK == retCode) )
    {
        clLogError(CL_CKPT_AREA_MASTER, CL_CKPT_CTX_REPL_NOTIFY, 
                   "Version mismatch happened at server [%d] rc [0x %x]", 
                   peerAddr, rc); 
    }
    else if( (gCkptSvr != NULL) && (gCkptSvr->serverUp != CL_FALSE) )
    {
        /*
         * Lock the master BD.
         */
        CKPT_LOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
        /*
         * Checkout the checkpoint related metadata.
         */
        rc = ckptSvrHdlCheckout( gCkptSvr->masterInfo.masterDBHdl,
                                 ckptActHdl,
                                 (void **)&pMasterDBEntry);
        if( CL_OK != rc )
        {
            clLogError(CL_CKPT_AREA_MASTER, CL_CKPT_CTX_REPL_NOTIFY, 
                       "CkptHdl [%#llX] does not exist, rc [0x %x]",
                       ckptActHdl, rc);
            goto exitOnErrorBeforeHdlCheckout;
        }

        if( CL_OK == retCode )
        {
            /*
             * Add the replica address to the replica list of the checkpoint. 
             */         
            clLogDebug(CL_CKPT_AREA_MASTER, CL_CKPT_CTX_REPL_NOTIFY, 
                       "Checkpoint [%.*s] has been replicated on [%d]", 
                       pMasterDBEntry->name.length, pMasterDBEntry->name.value, 
                       peerAddr);
            rc = clCntNodeAdd( pMasterDBEntry->replicaList,
                               (ClCntKeyHandleT)(ClWordT)peerAddr, 
                               (ClCntDataHandleT)(ClWordT)peerAddr, NULL);
            if (CL_GET_ERROR_CODE(rc) == CL_ERR_DUPLICATE)   rc = CL_OK;
            if( CL_OK != rc )
            {
                clLogError(CL_CKPT_AREA_MASTER, CL_CKPT_CTX_REPL_NOTIFY, 
                           "While updating replica list for checkpoint handle [%#llX]"
                           "rc [0x %x]", ckptActHdl, rc);
                goto exitOnError;
            }

            /*          
             * Increment the replica count.
             */
            rc = clCntDataForKeyGet( gCkptSvr->masterInfo.peerList,
                                     (ClCntDataHandleT)(ClWordT)peerAddr,
                                     (ClCntDataHandleT *)&pPeerInfo);
            if( CL_OK != rc )
            {
                clLogError(CL_CKPT_AREA_MASTER, CL_CKPT_CTX_REPL_NOTIFY, 
                           "While updating replica list for checkpoint handle [%#llX]"
                           "rc [0x %x]", ckptActHdl, rc);
                goto exitOnError;
            }
            pPeerInfo->replicaCount++;
            /*
             * Update the deputy
             */
            {
                ClVersionT version;
                rc = ckptIdlHandleUpdate(CL_IOC_BROADCAST_ADDRESS, 
                        gCkptSvr->ckptIdlHdl, 0);
                if( CL_OK != rc )
                {
                    clLogError(CL_CKPT_AREA_MASTER, CL_CKPT_CTX_REPL_NOTIFY, 
                               "Idl handle update failed for handle [%#llX]"
                               "rc [0x %x]", ckptActHdl, rc);
                    goto exitOnError;
                }

                memcpy( &version, gCkptSvr->versionDatabase.versionsSupported, 
                        sizeof(ClVersionT));

                rc = VDECL_VER(clCkptDeputyReplicaListUpdateClientAsync, 4, 0, 0)(gCkptSvr->ckptIdlHdl,
                                                                                  &version,
                                                                                  ckptActHdl,
                                                                                  peerAddr,
                                                                                  pRepInfo->addToList,
                                                                                  NULL,NULL);
                if( CL_OK != rc )
                {
                    clLogError(CL_CKPT_AREA_MASTER, CL_CKPT_CTX_REPL_NOTIFY, 
                               "Deputy update for handle [%#llX]"
                               "rc [0x %x]", ckptActHdl, rc);
                    goto exitOnError;
                }
            }
        }
        else
        {

            if(pRepInfo->tryOtherNode == CL_CKPT_RETRY)
            {
                /*
                 * Failed to update the info in peerAddr.
                 * Choose some other node from the peerList.
                 */
                rc = clCkptNextReplicaGet(pMasterDBEntry->replicaList, &activeAddr);
                /*
                 * Start the replication process with the newly selected node.
                 */
                if( (activeAddr != CL_CKPT_UNINIT_ADDR) && (activeAddr != peerAddr) )
                {
                    clLogNotice(CL_CKPT_AREA_MASTER, CL_CKPT_CTX_CKPT_OPEN, 
                                "[Try other node] Notifying address [%d] to pull info from [%d] for handle [%#llX]", 
                                peerAddr, activeAddr, ckptActHdl);

                    rc = clCkptReplicaCopyWithContext(peerAddr, activeAddr, hdlDbInfo.activeAddr, pRepInfo, pVersion); 
                    clHandleCheckin(gCkptSvr->masterInfo.masterDBHdl, ckptActHdl);
                    CKPT_UNLOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
                    if( CL_OK != rc )
                    {
                        retCode = rc;
                        clLogError(CL_CKPT_AREA_MASTER, CL_CKPT_CTX_REPL_NOTIFY,
                                   "While updating replica on peerAddr [%d] from"
                                   "active addr [%d] failed rc [0x %x]", peerAddr,
                                   activeAddr, rc);
                        goto out_respond;
                    }
                    return;
                }
                goto out_close;
            }
            else
            {
                out_close:
                clHandleCheckin( gCkptSvr->masterInfo.masterDBHdl, ckptActHdl);
                CKPT_UNLOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
                rc = _clCkptMasterClose(hdlDbInfo.ckptMasterHdl, peerAddr, 
                                        !CL_CKPT_MASTER_HDL);
                goto out_respond;
            }
        }
        /*
         * Checkin the checkpoint related metadata.
         */
        clHandleCheckin( gCkptSvr->masterInfo.masterDBHdl, ckptActHdl);
        /*
         * Unlock the master DB.
         */
        CKPT_UNLOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
    }

    out_respond:
    clLogDebug(CL_CKPT_AREA_MASTER, CL_CKPT_CTX_REPL_NOTIFY, 
               "Responding to client, clientHdl [%#llX] activeAddr [%d]", 
               hdlDbInfo.ckptMasterHdl, hdlDbInfo.activeAddr);
    VDECL_VER(clCkptMasterCkptOpenResponseSend, 4, 0, 0)(pRepInfo->idlHdl, retCode, *pVersion,
                                                         hdlDbInfo);
    /*
     * Free the cookie.
     */
    if(pRepInfo != NULL)
        clHeapFree(pRepInfo);
    return;
    exitOnError:
    /*
     * Checkin the checkpoint related metadata.
     */
    clHandleCheckin( gCkptSvr->masterInfo.masterDBHdl, ckptActHdl);
    exitOnErrorBeforeHdlCheckout:
    /*
     * Unlock the master DB.
     */
    CKPT_UNLOCK(gCkptSvr->masterInfo.ckptMasterDBSem);

    /*
     * Free the cookie.
     */
    VDECL_VER(clCkptMasterCkptOpenResponseSend, 4, 0, 0)(pRepInfo->idlHdl, rc, *pVersion,
                                                         hdlDbInfo);
    if(pRepInfo != NULL)
        clHeapFree(pRepInfo);
    return;
}

/*
 * This function informs the node to pull the checkpoint from the 
 * active replica.
 */

ClRcT clCkptReplicaCopyWithContext(ClIocNodeAddressT destAddr,
                                   ClIocNodeAddressT replicaAddr,
                                   ClIocNodeAddressT activeAddr,
                                   ClCkptReplicateInfoT *pRepInfo,
                                   ClVersionT *pVersion)
{
    ClRcT                rc          = CL_OK;
    ClVersionT           ckptVersion = {0};
   
    if(!pVersion) pVersion = &ckptVersion;

    /*
     * Update the idl handle with the destination address.
     */
    rc = ckptIdlHandleUpdate( destAddr, gCkptSvr->ckptIdlHdl, 2);
    if( CL_OK != rc )
    {
        clLogError(CL_CKPT_AREA_MASTER, CL_CKPT_CTX_CKPT_OPEN,
                   "Failed to update the idl handle [%#llX] rc[%#x]",
                   gCkptSvr->ckptIdlHdl, rc);
        return rc;
    }
    /*
     * Set the supported version.
     */
    memcpy(pVersion,
           gCkptSvr->versionDatabase.versionsSupported,
           sizeof(ClVersionT));

    pRepInfo->destAddr     = destAddr;
    pRepInfo->activeAddr   = activeAddr;
    pRepInfo->tryOtherNode = CL_TRUE;

    /*
     *  Send the RMD.
     */ 
    rc = VDECL_VER(clCkptReplicaNotifyClientAsync, 4, 0, 0)( gCkptSvr->ckptIdlHdl,
                                                             pVersion,
                                                             pRepInfo->activeHdl,
                                                             replicaAddr,
                                                             clCkptReplicaCopyCallback,
                                                             (void *)pRepInfo);
    if( CL_OK != rc )
    {
        clLogError(CL_CKPT_AREA_MASTER, CL_CKPT_CTX_CKPT_OPEN, 
                   "Replica copy to peerAddr [%d] failed rc[0x %x]", destAddr, rc);
        return rc;
    }
    return CL_OK;
}
 
ClRcT clCkptReplicaCopy(ClIocNodeAddressT destAddr,
                        ClIocNodeAddressT replicaAddr,
                        ClHandleT         activeHdl, 
                        ClHandleT         clientHdl,
                        ClIocNodeAddressT activeAddr,
                        CkptHdlDbT        *pHdlInfo)
{
    ClRcT                rc          = CL_OK;
    ClVersionT           ckptVersion = {0};
    ClCkptReplicateInfoT *pRepInfo   = NULL;
    CkptHdlDbT           hdlDbInfo   = {0};
   
    /*
     * Fill the cookie to be passed to the callback.
     */
    pRepInfo = (ClCkptReplicateInfoT *) clHeapCalloc(1,
                                                     sizeof(ClCkptReplicateInfoT));
    if(pRepInfo == NULL)
    {
        clLogCritical(CL_CKPT_AREA_MASTER, CL_CKPT_CTX_CKPT_OPEN, 
                      "Failed to allocated while trying to update the replica");
        rc = CL_CKPT_ERR_NO_MEMORY;
        return rc;
    }
    rc = clCkptEoIdlSyncDefer(&pRepInfo->idlHdl);
    if( CL_OK != rc )
    {
        clLogError(CL_CKPT_AREA_MASTER, CL_CKPT_CTX_CKPT_OPEN, 
                   "Idl Defer failed rc[0x %x]", rc);
        clHeapFree(pRepInfo);
        return rc;
    }
    pRepInfo->clientHdl    = clientHdl;
    pRepInfo->activeHdl    = pHdlInfo->ckptActiveHdl;
    pRepInfo->creationFlags = pHdlInfo->creationFlag;

    rc = clCkptReplicaCopyWithContext(destAddr, replicaAddr, activeAddr, pRepInfo, &ckptVersion);

    if( CL_OK != rc )
    {
        VDECL_VER(clCkptMasterCkptOpenResponseSend, 4, 0, 0)(pRepInfo->idlHdl, rc, ckptVersion,
                                                             hdlDbInfo);
        clHeapFree(pRepInfo);
        return rc;
    }
    return CL_OK;
}
