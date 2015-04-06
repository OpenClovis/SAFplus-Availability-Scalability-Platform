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
File        : clCkptDeputy.c
        clCkptDeputy.c $
 ****************************************************************************/

/****************************************************************************
 * Description :                                                                
 *
 *  This file contains CKPT deputy node functionality.
 *
 *
 ****************************************************************************/
#include <string.h> 
#include <netinet/in.h> 

#include "clCommon.h"
#include <clCommonErrors.h>
#include <clVersion.h>

#include "clCkptSvr.h"
#include <clCkptLog.h>
#include "clCkptSvrIpi.h"
#include "clCkptUtils.h"
#include "clCkptMasterUtils.h"
#include "clCkptMaster.h"
#include <clCpmExtApi.h>

#include "ckptEockptServerMasterDeputyClient.h"
#include "ckptEockptServerMasterActiveServer.h"
extern CkptSvrCbT  *gCkptSvr;
extern ClInt32T ckptReplicaListKeyComp(ClCntKeyHandleT key1,
        ClCntKeyHandleT key2);
extern void ckptReplicaListDeleteCallback(ClCntKeyHandleT  userKey,
        ClCntDataHandleT userData);
        
/*
 * Function for requesting the checkpoint metadata and other info
 * from the master. This is usually called by deputy when it comes up.
 * In case master is restarted, and deputy is there, master calls this
 * function to request in info from the deputy.
 */
 
ClRcT ckptMasterDatabaseSyncup(ClIocNodeAddressT dest)
{
    ClRcT                    rc             = CL_OK;
    CkptXlationDBEntryT      *pXlationInfo  = NULL;
    CkptMasterDBInfoIDLT     *pMasterInfo   = NULL;
    ClUint32T                mastHdlCount   = 0;
    CkptMasterDBEntryIDLT    *pMasterDBInfo = NULL;
    ClUint32T                peerCount      = 0;
    CkptPeerListInfoT        *pPeerListInfo = NULL;
    ClUint32T                clntHdlCount   = 0;
    CkptMasterDBClientInfoT  *pClientDBInfo = NULL;
    ClVersionT               ckptVersion    = {0};
    ClUint32T                ckptCount      = 0;

    CKPT_NULL_CHECK(gCkptSvr);

    memcpy(&ckptVersion,gCkptSvr->versionDatabase.versionsSupported, sizeof(ClVersionT));
    pMasterInfo = (CkptMasterDBInfoIDLT *) clHeapCalloc(1,
                           sizeof(CkptMasterDBInfoIDLT));       
    if(pMasterInfo == NULL)
    {
        rc = CL_CKPT_ERR_NO_MEMORY;
        CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Ckpt: Failed to allocate the memory rc[0x %x]\n", rc), rc);
    }

 
    rc = ckptIdlHandleUpdate(dest, gCkptSvr->ckptIdlHdl, 0);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Ckpt: Failed to update the Idl handle rc[0x %x]\n", rc), rc);

    /*
     * Request the metadata and the related info from the deputy.
     */
    rc =  VDECL_VER(clCkptDeputyMasterInfoSyncupClientSync, 4, 0, 0)(gCkptSvr->ckptIdlHdl,
                &ckptVersion,
                &ckptCount,
                &pXlationInfo,
                pMasterInfo,
                &mastHdlCount,
                &pMasterDBInfo,
                &peerCount,
                &pPeerListInfo,
                &clntHdlCount,
                &pClientDBInfo);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Ckpt: Failed to get the data from Master rc[0x %x]\n", rc), rc);
            
    /*
     * Populate All data structures in deputy.
     */        
     
    /* 
     * Unpack the Name xlation Entries
     */
    rc = ckptXlatioEntryUnpack(ckptCount,pXlationInfo);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR, ("Ckpt: Failed to unpack the Xlation Entry rc[0x %x]\n", rc), rc);
    gCkptSvr->masterInfo.masterAddr = pMasterInfo->masterAddr;
    gCkptSvr->masterInfo.deputyAddr = pMasterInfo->deputyAddr;
    gCkptSvr->masterInfo.compId     = pMasterInfo->compId;
    
    /* 
     * Unpack the peerList.
     */
    rc = ckptPeerListInfoUnpack(peerCount,pPeerListInfo);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,("Ckpt: Failed to unpack the PeerList info rc[0x %x]\n", rc), rc);

    /*
     * Unpack the master DB info.
     */
    rc = ckptMasterDBInfoUnpack(mastHdlCount,pMasterDBInfo);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,("Ckpt: Failed to unpack the MasterInfo rc[0x %x]\n", rc), rc);
            
    clLogDebug(CL_CKPT_AREA_DEPUTY, CL_CKPT_CTX_DEP_SYNCUP,"Received open handle [%d]", clntHdlCount);
    /*
     * Unpack the client DB info.
     */
    rc = ckptClientDBInfoUnpack(clntHdlCount,pClientDBInfo);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,("Ckpt: Failed to unpack the ClientDB Info rc[0x %x]\n", rc), rc);
    gCkptSvr->isSynced = CL_TRUE;

exitOnError:
    /*
     * Clean up the allocated resources.
     */
    if(pMasterDBInfo != NULL)
    {
        if( NULL != pMasterDBInfo->replicaListInfo )
        {
            clHeapFree(pMasterDBInfo->replicaListInfo);
            pMasterDBInfo->replicaListInfo = NULL;
        }
        clHeapFree(pMasterDBInfo);
    }
    if(pXlationInfo != NULL) clHeapFree(pXlationInfo);
    if(pPeerListInfo != NULL)
    {
        if( NULL != pPeerListInfo->nodeListInfo )
        {
            clHeapFree(pPeerListInfo->nodeListInfo);
            pPeerListInfo->nodeListInfo = NULL;
        }
        if( NULL != pPeerListInfo->mastHdlInfo )
        {
            clHeapFree(pPeerListInfo->mastHdlInfo);
            pPeerListInfo->mastHdlInfo = NULL;
        }
        clHeapFree(pPeerListInfo);
    }
    if(pClientDBInfo != NULL) clHeapFree(pClientDBInfo);
    return rc;
} 



/*
 * Function for unpacking the name xlation table entries.
 */
 
ClRcT ckptXlatioEntryUnpack( ClUint32T           ckptCount, CkptXlationDBEntryT *pXlationInfo)
{
    CkptXlationDBEntryT  *pXlationEntry = NULL;
    ClUint32T             count         = 0;
    ClRcT                 rc            = CL_OK;

    /*
     * Validate the input parameter.
     */
    CKPT_NULL_CHECK(pXlationInfo);
    
    /*
     * Prepare the Xlation table database.
     */
    for( count = 0; count < ckptCount; count++)
    {
        /*
         * Allocate memory and copy the unpacked data.
         */
        pXlationEntry = (CkptXlationDBEntryT *)clHeapCalloc(1, sizeof(CkptXlationDBEntryT));
        if(pXlationEntry == NULL)
        {
            rc = CL_CKPT_ERR_NO_MEMORY;
            CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                    ("Ckpt: Failed to allocate memory rc[0x %x]\n", rc), rc);
        }
        clNameCopy(&pXlationEntry->name, &pXlationInfo->name);
        pXlationEntry->cksum   = pXlationInfo->cksum;
        pXlationEntry->mastHdl = pXlationInfo->mastHdl;
        rc = clCntNodeAdd(gCkptSvr->masterInfo.nameXlationDBHdl,
                (ClCntKeyHandleT ) pXlationEntry,
                (ClCntDataHandleT ) pXlationEntry, NULL);
        if(CL_GET_ERROR_CODE(rc) == CL_ERR_DUPLICATE)
        {
            CkptXlationDBEntryT  *pStoredXlation = NULL;
            rc = clCntDataForKeyGet(gCkptSvr->masterInfo.nameXlationDBHdl, (ClCntKeyHandleT) pXlationEntry, (ClCntDataHandleT*)&pStoredXlation);
            if(rc == CL_OK)
            {
                /*
                 * Update the master handle in the db. from the received pack
                 */
                clLogWarning("DEP", "XLATION", "Duplicate ckpt [%.*s] with master handle [%#llx] found while unpacking ckpt entry [%.*s] with master hdl [%#llx], sum [%#x]",
                             pStoredXlation->name.length, pStoredXlation->name.value, 
                             pStoredXlation->mastHdl, pXlationEntry->name.length, 
                             pXlationEntry->name.value, pXlationEntry->mastHdl,
                             pXlationEntry->cksum);
                pStoredXlation->mastHdl = pXlationEntry->mastHdl;
                clHeapFree(pXlationEntry);
                pXlationEntry = pStoredXlation;
            }
            else 
            {
                rc = CL_OK;
            }
        }

        /* GAS: was CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,("Ckpt: Failed to add to the XlationEntry rc[0x%x]\n", rc), rc);
           but I think we should continue to add the other entries
        */
        if (rc != CL_OK)
        {
        clLogWarning(CL_CKPT_AREA_DEPUTY, CL_CKPT_CTX_DEP_SYNCUP,"Checkpoint [%.*s] with master handle [%#llx], cksum [%#x] cannot be added to the translation table, rc [%x]",pXlationEntry->name.length, pXlationEntry->name.value, pXlationEntry->mastHdl,pXlationEntry->cksum,rc);
        }
        else
        {        
        clLogDebug(CL_CKPT_AREA_DEPUTY, CL_CKPT_CTX_DEP_SYNCUP, "Checkpoint [%.*s] handle info has been replicated here. Master handle [%#llx], cksum [%#x]", pXlationEntry->name.length, pXlationEntry->name.value, pXlationEntry->mastHdl, pXlationEntry->cksum);
        }
        
        pXlationInfo++;      
    }
exitOnError:    
    return rc;
}

/*
 * Function for unpacking the peer list data.
 */
 
ClRcT ckptPeerListInfoUnpack(ClUint32T          peerCount,
                             CkptPeerListInfoT  *pPeerListInfo)
{
    ClRcT             rc              = CL_OK;
    CkptPeerInfoT     *pPeerInfo      = NULL;
    ClUint32T         count           = 0;
    ClUint32T         openCount       = 0;
    ClHandleT         *pMasterHdlInfo = NULL;
    ClHandleT         *pMasterHandle  = NULL;
    CkptNodeListInfoT *pNodeTempInfo  = NULL;
    
    CKPT_DEBUG_T(("PeerCount : %d\n",peerCount));
    
    /*
     * Prepare the peer list database.
     */
    for( count  = 0 ; count < peerCount; count++)
    {
       /*
        * Check whether the entry already exists in the requesting node's
        * peer list. If not, then allocate memory for the peer.
        */
       rc =  clCntDataForKeyGet(gCkptSvr->masterInfo.peerList,
                                (ClPtrT)(ClWordT)pPeerListInfo->nodeAddr,
                               (ClCntDataHandleT  *)&pPeerInfo);
       if( CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST)
       {
           rc = _ckptMasterPeerListInfoCreate(pPeerListInfo->nodeAddr,
                                              pPeerListInfo->credential,
                                              pPeerListInfo->replicaCount);
           rc =  clCntDataForKeyGet(gCkptSvr->masterInfo.peerList,
                          (ClPtrT)(ClWordT)pPeerListInfo->nodeAddr,
                          (ClCntDataHandleT  *)&pPeerInfo);
           clLogDebug(CL_CKPT_AREA_DEPUTY, CL_CKPT_CTX_DEP_SYNCUP, 
                      "Peer list has been updated for address [%d]", 
                      pPeerListInfo->nodeAddr);
       }
       CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Ckpt: Failed to find the entry rc[0x %x]\n", rc), rc);
       /*
        * Prepare the client Hdl list associated with the peer.
        */
       pNodeTempInfo = pPeerListInfo->nodeListInfo;
       
       for(openCount = 0; openCount < pPeerListInfo->numOfOpens; openCount++)
       {
          CkptNodeListInfoT *pNodeInfo = NULL;
          pNodeInfo = (CkptNodeListInfoT *)clHeapCalloc(1,
                            sizeof(CkptNodeListInfoT));
          if(pNodeInfo == NULL)
          {
                   rc = CL_CKPT_ERR_NO_MEMORY;
                   CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                    ("Ckpt: Failed to allocate memory rc[0x %x]\n", rc), rc);
          }
          pNodeInfo->clientHdl   =  pNodeTempInfo->clientHdl;
          pNodeInfo->appPortNum  =  pNodeTempInfo->appPortNum;
          pNodeInfo->appAddr     =  pNodeTempInfo->appAddr;
          rc = clCntNodeAdd( pPeerInfo->ckptList,
                             (ClPtrT)(ClWordT)pNodeInfo->clientHdl,
                             (ClCntDataHandleT)pNodeInfo, NULL);
          if( CL_GET_ERROR_CODE(rc) == CL_ERR_DUPLICATE)
                rc = CL_OK;
          CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                    ("ckptMasterDatabaseUnpack failed rc[0x %x]\n",rc),rc);
           clLogDebug(CL_CKPT_AREA_DEPUTY, CL_CKPT_CTX_DEP_SYNCUP, 
                      "Open handle %#llX has been added to address [%d]", 
                      pNodeInfo->clientHdl, pPeerListInfo->nodeAddr);
          pNodeTempInfo++;          
       }

       /*
        * Prepare the master Hdl list associated with the peer.
        */
       pMasterHdlInfo = pPeerListInfo->mastHdlInfo;
       for(openCount = 0; openCount < pPeerListInfo->numOfHdl; openCount++)
       {
           /*
            * Allocate memory for the 64bit handle.
            */
           pMasterHandle = clHeapAllocate(sizeof(*pMasterHandle)); // Free on Error
           if(NULL == pMasterHandle)
           {
               CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Memory Allocation Failed\n"));
               goto exitOnError;
           }
           *pMasterHandle = *pMasterHdlInfo;
          rc = clCntNodeAdd( pPeerInfo->mastHdlList,
                             pMasterHandle,
                             0, NULL);
          if( rc == CL_OK ) pPeerInfo->replicaCount++;
          if( CL_GET_ERROR_CODE(rc) == CL_ERR_DUPLICATE)
                rc = CL_OK;
          CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                    ("ckptMasterDatabaseUnpack failed rc[0x %x]\n",rc),rc);
           clLogDebug(CL_CKPT_AREA_DEPUTY, CL_CKPT_CTX_DEP_SYNCUP, 
                      "Master handle %#llX has been added to address [%d]", 
                       *pMasterHandle, pPeerListInfo->nodeAddr);
          pMasterHdlInfo++;                    
       }
       pPeerListInfo++;
    }
 exitOnError:
    CKPT_DEBUG_T(("Exit [0x %x]\n", rc));
    return rc;
}



/*
 * Function for unpacking the master DB Info database.
 */
 
ClRcT ckptMasterDBInfoUnpack(ClUint32T             mastHdlCount,
                             CkptMasterDBEntryIDLT *pMasterDBInfo)
{
    ClRcT                 rc               = CL_OK;
    ClUint32T             count            = 0;
    ClUint32T             replicaCount     = 0;
    CkptMasterDBEntryT    *pMasterDBEntry  = NULL;
    ClIocNodeAddressT     *pReplicaListInfo = NULL;

    /*
     * Validate the input parameter.
     */
    CKPT_NULL_CHECK(pMasterDBInfo);
    
    /*
     * Prepare the master DB Info database.
     */
    for( count = 0; count < mastHdlCount; count++)
    {
        if (pMasterDBInfo->ckptMasterHdl == CL_HANDLE_INVALID_VALUE)
        {
            clLogWarning(CL_CKPT_AREA_DEPUTY, CL_CKPT_CTX_DEP_SYNCUP, "Skipping invalid 0 handle received during masterdb unpack");
            ++pMasterDBInfo;
            continue;
        }

        /*
         * Create a handle and copy the unpacked info.
         */
        rc = clHandleCreateSpecifiedHandle(
                gCkptSvr->masterInfo.masterDBHdl,
                sizeof(CkptMasterDBEntryT),
                pMasterDBInfo->ckptMasterHdl);
        if(CL_GET_ERROR_CODE(rc) == CL_ERR_ALREADY_EXIST) 
        {
            clLogDebug("INFO", "SYNCUP",
                        "Master db handle [%#llx] already exists and in sync. "
                        "Skipping masterdb info syncup",
                        pMasterDBInfo->ckptMasterHdl);
            ++pMasterDBInfo;
            rc = CL_OK;     
            continue;
        }
        CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_DEBUG_ERROR, ("ckptMasterDatabaseUnpack failed rc[0x %x]\n",rc),rc);
                
        rc = clHandleCheckout( gCkptSvr->masterInfo.masterDBHdl, pMasterDBInfo->ckptMasterHdl, (void **)&pMasterDBEntry);
        CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_DEBUG_ERROR, ("ckptMasterDatabaseUnpack failed rc[0x %x]\n",rc),rc);
        
        /*
         * Increment the masterHdl count.
         */
        gCkptSvr->masterInfo.masterHdlCount++;
        
        memcpy(&pMasterDBEntry->attrib, &pMasterDBInfo->attrib, sizeof(ClCkptCheckpointCreationAttributesT));
        clNameCopy(&pMasterDBEntry->name, &pMasterDBInfo->name);
        pMasterDBEntry->markedDelete  = pMasterDBInfo->markedDelete;
        pMasterDBEntry->refCount      = pMasterDBInfo->refCount;
            
        pMasterDBEntry->retenTimerHdl = 0;
        pMasterDBEntry->activeRepAddr = pMasterDBInfo->activeRepAddr;
        pMasterDBEntry->prevActiveRepAddr = pMasterDBInfo->prevActiveRepAddr;
        pMasterDBEntry->activeRepHdl  = pMasterDBInfo->activeRepHdl;

        clLogInfo(CL_CKPT_AREA_DEPUTY, CL_CKPT_CTX_DEP_SYNCUP, "Replicated checkpoint [%.*s] reference count [%d] active replica [%d]", pMasterDBEntry->name.length, pMasterDBEntry->name.value,pMasterDBEntry->refCount,pMasterDBEntry->activeRepAddr);
       
        /*
         * TODO : Check the following flow.
         */
        if((pMasterDBEntry->activeRepAddr == gCkptSvr->localAddr) &&
                ( CL_CKPT_CHECKPOINT_COLLOCATED == (
                                      pMasterDBEntry->attrib.creationFlags &
                                      CL_CKPT_CHECKPOINT_COLLOCATED)) &&
                ((gCkptSvr->masterInfo.deputyAddr == CL_CKPT_UNINIT_ADDR) ||
                 (gCkptSvr->masterInfo.deputyAddr == -1)))
        {
            ClCkptClientUpdInfoT    eventInfo;
            ClEventIdT              eventId;

            /* 
             * Publish event to inform the clients.
             */
            memset(&eventInfo, 0, sizeof(ClCkptClientUpdInfoT));
            clNameCopy(&eventInfo.name, &pMasterDBEntry->name);
            eventInfo.name.length = htons(pMasterDBEntry->name.length);
            eventInfo.eventType = htonl(CL_CKPT_ACTIVE_REP_CHG_EVENT);
            eventInfo.actAddr   = htonl(pMasterDBEntry->activeRepAddr);
            clLogInfo(CL_CKPT_AREA_DEPUTY, CL_CKPT_CTX_DEP_SYNCUP, 
                    "Publishing event for address change from [%d] to [%d]"
                    "for [%.*s]", pMasterDBEntry->prevActiveRepAddr,
                    pMasterDBEntry->activeRepAddr, pMasterDBEntry->name.length,
                    pMasterDBEntry->name.value); 
            rc = clEventPublish( gCkptSvr->clntUpdEvtHdl,
                    (const void*)&eventInfo,
                    sizeof(ClCkptClientUpdInfoT), &eventId);
        }

        /*
         * Create and populate the replica list associated with the
         * checkpoint.
         */
        rc = clCntLlistCreate(ckptReplicaListKeyComp,
                ckptReplicaListDeleteCallback,
                ckptReplicaListDeleteCallback,
                CL_CNT_UNIQUE_KEY,
                &pMasterDBEntry->replicaList);
        CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                ("Ckpt: Faile to unpack master Entries rc[0x %x]\n",rc),rc);
        /* Stroing the replicalistinfo, to restore it */
        pReplicaListInfo = pMasterDBInfo->replicaListInfo;
        for( replicaCount = 0; replicaCount < pMasterDBInfo->replicaCount;
                replicaCount++)
        {
            if( pReplicaListInfo != NULL)
            {
                rc = clCntNodeAdd( pMasterDBEntry->replicaList, 
                        (ClCntKeyHandleT)(ClWordT)*(pReplicaListInfo),
                        (ClCntKeyHandleT)(ClWordT)*(pReplicaListInfo),
                        NULL);
                if( CL_GET_ERROR_CODE(rc) == CL_ERR_DUPLICATE)
                    rc = CL_OK;         
                CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                   ("ckptMasterDatabaseUnpack failed rc[0x %x]\n",rc),rc);
            } 
            else
            {
                rc = CL_CKPT_ERR_INVALID_STATE;
                CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                      ("ckptMasterDatabaseUnpack failed rc[0x %x]\n",rc),rc);
            }
            pReplicaListInfo++;
        }                                
        clHandleCheckin( gCkptSvr->masterInfo.masterDBHdl, pMasterDBInfo->ckptMasterHdl);
        clLogDebug(CL_CKPT_AREA_DEPUTY, CL_CKPT_CTX_DEP_SYNCUP, "Master handle [%#llX] has been created in database [%#lx %p]", pMasterDBInfo->ckptMasterHdl, clHandleGetDatabaseId(gCkptSvr->masterInfo.masterDBHdl), gCkptSvr->masterInfo.masterDBHdl);
        pMasterDBInfo++;
    }                       
    return rc;
exitOnError:
    clHandleCheckin( gCkptSvr->masterInfo.masterDBHdl,
            pMasterDBInfo->ckptMasterHdl);
exitOnErrorBeforeHdlCheckout:
    return rc;
}


/*
 * Function for unpacking the client DB Info database.
 */
 
ClRcT ckptClientDBInfoUnpack(ClUint32T               clientHdlCount,
                             CkptMasterDBClientInfoT *pClientDBInfo)
{
    ClRcT                   rc              = CL_OK;
    ClUint32T               count           = 0;
    CkptMasterDBClientInfoT *pClientEntry   = NULL;

    /*
     * Validate the input parameter.
     */
    CKPT_NULL_CHECK(pClientDBInfo);
    
    /*
     * Prepare the client DB Info database.
     */
    for( count = 0; count < clientHdlCount; count++)
    {

        if (pClientDBInfo->clientHdl == CL_HANDLE_INVALID_VALUE)
        {
            clLogWarning(CL_CKPT_AREA_DEPUTY, CL_CKPT_CTX_DEP_SYNCUP,
                         "Skipping invalid 0 handle received during clientdb unpack");
            ++pClientDBInfo;
            continue;
        }
        
        /*
         * Create a handle and copy the unpacked info.
         */
        rc = clHandleCreateSpecifiedHandle(
                gCkptSvr->masterInfo.clientDBHdl,
                sizeof(CkptMasterDBClientInfoT),
                pClientDBInfo->clientHdl);
        if(CL_GET_ERROR_CODE(rc) == CL_ERR_ALREADY_EXIST) 
        {
            clLogNotice("INFO", "SYNCUP", "Client handle [%#llx] already exists."
                        "Skipping unpack for master handle [%#llx]", 
                        pClientDBInfo->clientHdl, pClientDBInfo->masterHdl);
            pClientDBInfo++;
            rc = CL_OK;
            continue;
        }
        CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                ("ckptMasterDatabaseUnpack failed rc[0x %x]\n",rc),rc);
        rc = clHandleCheckout( gCkptSvr->masterInfo.clientDBHdl,
                pClientDBInfo->clientHdl,
                (void **)&pClientEntry);
        CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                ("ckptMasterDatabaseUnpack failed rc[0x %x]\n",rc),rc);
                
        /*
         * Increment the clientHdl count.
         */
        gCkptSvr->masterInfo.clientHdlCount++; 
        pClientEntry->clientHdl = pClientDBInfo->clientHdl;        
        pClientEntry->masterHdl = pClientDBInfo->masterHdl;        
        clHandleCheckin(gCkptSvr->masterInfo.clientDBHdl,
                        pClientDBInfo->clientHdl);
        clLogDebug(CL_CKPT_AREA_DEPUTY, CL_CKPT_CTX_DEP_SYNCUP, 
                   "Open Handle [%#llX] has been created", 
                   pClientDBInfo->clientHdl);
        pClientDBInfo++;                
    }                    
exitOnErrorBeforeHdlCheckout:                       
   return rc;
}

/*
 * Function invoked at deputy whenever a checkpoint is closed
 * at master.
 */
 
ClRcT ckptCheckpointClose(ClHandleT clientHdl,ClHandleT masterHdl,
                          ClIocNodeAddressT localAddr)
{
    ClRcT                   rc           = CL_OK;
    CkptMasterDBEntryT      *pStoredData = NULL;
    CkptPeerInfoT           *pPeerInfo   = NULL;   
 
    /*
     * Check if the deputy is up or not.
     */
    CKPT_NULL_CHECK(gCkptSvr);

    /*
     * Lock the master DB.
     */
    CKPT_LOCK(gCkptSvr->masterInfo.ckptMasterDBSem);

    /*
     * Destroy the client hdl.
     */
    rc = clHandleDestroy(gCkptSvr->masterInfo.clientDBHdl,
            clientHdl); 
    CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_LOG_DEBUG,
            (" MasterCheckpointClose failed rc[0x %x]\n",rc),
            rc);
    clLogDebug(CL_CKPT_AREA_DEPUTY, CL_CKPT_CTX_CKPT_CLOSE,
               "Open handle [%#llX] has been closed at deputy", 
                clientHdl);
    /* 
     * Decrement the client handle count.
     */
    gCkptSvr->masterInfo.clientHdlCount--;        

    /*
     * Remove the entry from peer's client hdl list.
     */
    rc = clCntDataForKeyGet(gCkptSvr->masterInfo.peerList, (ClPtrT)(ClWordT)localAddr,
                            (ClCntDataHandleT *)&pPeerInfo); 
    if( CL_OK != rc )
    {
        clLogWarning(CL_CKPT_AREA_DEPUTY, CL_CKPT_CTX_CKPT_CLOSE, 
                "Failed to find address [%d] in the peer list", localAddr);
    }
    else
    {
        if ( CL_OK != (rc = clCntAllNodesForKeyDelete(pPeerInfo->ckptList,
                        (ClPtrT)(ClWordT)clientHdl)))
        {
            CKPT_DEBUG_E(("ClientHdl %#llX Delete from list failed", clientHdl));
        }
    }

    /*
     * Decrement the checkpoint's  reference count.
     */
    rc = clHandleCheckout(gCkptSvr->masterInfo.masterDBHdl,
            masterHdl, (void **)&pStoredData);
    CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            (" MasterCheckpointClose failed rc[0x %x]\n",rc),
            rc);
    if(pStoredData != NULL)
    {
        pStoredData->refCount--;
    }    
    rc = clHandleCheckin(gCkptSvr->masterInfo.masterDBHdl,
                         masterHdl);
                         
exitOnErrorBeforeHdlCheckout:
    /*
     * Unlock the master DB.
     */
    CKPT_UNLOCK(gCkptSvr->masterInfo.ckptMasterDBSem);

    /* Client hdl already destroyed at client app/comp finalize step */
    if (CL_GET_ERROR_CODE(rc) == CL_ERR_INVALID_HANDLE)
      {
        rc = CL_OK;
      }
    return rc;
}



/*
 * Function invoked at deputy whenever a checkpoint's active
 * address changes.
 */
 
ClRcT ckptCheckpointActiveAddrChange(ClHandleT         masterHdl,
                                     ClIocNodeAddressT activeAddr)
{
    ClRcT                rc            = CL_OK;
    CkptMasterDBEntryT   *pStoredData  = NULL;

    /*
     * Check if the deputy is up or not.
     */
    CKPT_NULL_CHECK(gCkptSvr);
    
    /*
     * Lock the master DB.
     */
    CKPT_LOCK(gCkptSvr->masterInfo.ckptMasterDBSem);

    /*
     * Update the checkpoint's active address.
     */
    rc = clHandleCheckout(gCkptSvr->masterInfo.masterDBHdl,
            masterHdl, (void **)&pStoredData);
    CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            (" MasterCheckpointClose failed rc[0x %x]\n",rc),
            rc);
    if(pStoredData != NULL)
    {
        /*
         * Update the masterHdl list. Remove the hasterHdl from old active's 
         * list and add it to new active's list.
         */
        pStoredData->prevActiveRepAddr = pStoredData->activeRepAddr;
        _ckptPeerListMasterHdlAdd(masterHdl, pStoredData->activeRepAddr, 
                activeAddr);
        pStoredData->activeRepAddr = activeAddr;
        clLogDebug(CL_CKPT_AREA_DEPUTY, CL_CKPT_CTX_CKPT_CLOSE,
                "Active address changed from [%d] to [%d] for ckpt [%.*s]",
                pStoredData->prevActiveRepAddr, activeAddr, 
                pStoredData->name.length, pStoredData->name.value);
    }    
    rc = clHandleCheckin(gCkptSvr->masterInfo.masterDBHdl,
                         masterHdl);
                         
exitOnErrorBeforeHdlCheckout:
    /*
     * Unock the master DB.
     */
    CKPT_UNLOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
    return rc;
}



/*
 * Function invoked at deputy whenever a checkpoint's retention
 * timer is changed.
 */
 
ClRcT ckptCheckpointRetenTimerUpdate(ClHandleT masterHdl,
                                     ClTimeT   retenTime)
{
    ClRcT                   rc              = CL_OK;
    CkptMasterDBEntryT      *pStoredData    = NULL;

    /*
     * Check if the deputy is up or not.
     */
    CKPT_NULL_CHECK(gCkptSvr);
    
    /*
     * Lock the master DB.
     */
    CKPT_LOCK(gCkptSvr->masterInfo.ckptMasterDBSem);

    /*
     * Update the retention timer value.
     */
    rc = clHandleCheckout(gCkptSvr->masterInfo.masterDBHdl,
            masterHdl, (void **)&pStoredData);
    CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            (" MasterCheckpointClose failed rc[0x %x]\n",rc),
            rc);
    if(pStoredData != NULL)
    {
        if(pStoredData->markedDelete != CL_TRUE)
            pStoredData->attrib.retentionDuration = retenTime;
        else
        {
            rc = CL_CKPT_ERR_OP_NOT_PERMITTED;    
        }    
    }    
    clHandleCheckin(gCkptSvr->masterInfo.masterDBHdl,
                         masterHdl);
                         
exitOnErrorBeforeHdlCheckout:
    /*
     * Unock the master DB.
     */
    CKPT_UNLOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
    return rc;
}



/*
 * Function invoked at deputy whenever a checkpoint is closed.
 */
 
ClRcT ckptCheckpointDelete(ClHandleT          clientHdl,
                           ClHandleT          masterHdl,
                           ClIocNodeAddressT  localAddr,
                           CkptDynamicSyncupT updateFlag)
{
    ClRcT                   rc              = CL_OK;
    CkptMasterDBEntryT      *pStoredData    = NULL;
    CkptXlationLookupT      lookup          = {0};
    ClUint32T               cksum           = 0;
    CkptPeerInfoT           *pPeerInfo      = NULL;

    /*
     * Check if the deputy is up or not.
     */
    CKPT_NULL_CHECK(gCkptSvr);

    /*
     * Lock the master DB.
     */
    CKPT_LOCK(gCkptSvr->masterInfo.ckptMasterDBSem);

    if(updateFlag == CL_CKPT_CLOSE_MARKED_DELETE)
    {
        /*
         * Delete the clientHdl Entries.
         */ 
        rc = clHandleDestroy(gCkptSvr->masterInfo.clientDBHdl,
                clientHdl); 
        CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                (" MasterCheckpointClose failed rc[0x %x]\n",rc),
                rc);
        clLogDebug(CL_CKPT_AREA_DEPUTY, CL_CKPT_CTX_CKPT_DEL, 
                "Open handle [%#llX] has been deleted", 
                clientHdl);
        gCkptSvr->masterInfo.clientHdlCount--;        

        rc = clCntDataForKeyGet(gCkptSvr->masterInfo.peerList, (ClPtrT)(ClWordT)localAddr,
                (ClCntDataHandleT  *)&pPeerInfo); 
        if(rc != CL_OK)
        {
            clLogWarning(CL_CKPT_AREA_DEPUTY, CL_CKPT_CTX_CKPT_DEL,
                         "Failed to get info of addr %d in peerList rc[0x%x]",
                         localAddr, rc);
        }
        else if ( CL_OK != (rc = clCntAllNodesForKeyDelete(pPeerInfo->ckptList,
                                                           (ClPtrT)(ClWordT)clientHdl)))
        {
            CKPT_DEBUG_E(("ClientHdl %#llX Delete from list failed", clientHdl));
        }
    }
    rc = clHandleCheckout(gCkptSvr->masterInfo.masterDBHdl,
            masterHdl, (void **)&pStoredData);
    CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            (" MasterCheckpointClose failed rc[0x %x]\n",rc),
            rc);
    if(pStoredData != NULL)
    {
        clDbgResourceNotify(clDbgCheckpointResource, clDbgRelease, 1, gCkptSvr->masterInfo.masterDBHdl, ("Deleting Checkpoint %s",pStoredData->name.value));
        pStoredData->markedDelete = CL_TRUE;
        memset(&lookup, 0, sizeof(lookup));
        clCksm32bitCompute ((ClUint8T *)pStoredData->name.value, pStoredData->name.length,
                &cksum);
        lookup.cksum = cksum;
        clNameCopy(&lookup.name, &pStoredData->name);
        clCntDelete(pStoredData->replicaList);
        pStoredData->replicaList = 0;
        if(pStoredData->retenTimerHdl)
            clTimerDelete(&pStoredData->retenTimerHdl);
    }    
    rc = clHandleCheckin(gCkptSvr->masterInfo.masterDBHdl,
            masterHdl);
    /* 
     * No need of checking rc. 
     * It might have been deleted during call unlink.
     */
    clCntAllNodesForKeyDelete(gCkptSvr->masterInfo.nameXlationDBHdl,
            (ClCntKeyHandleT)&lookup);
    clLogInfo(CL_CKPT_AREA_DEPUTY, CL_CKPT_CTX_CKPT_DEL, 
            "Ckpt [%.*s] has been deleted at deputy", 
            lookup.name.length, lookup.name.value);
    rc = clCntWalk( gCkptSvr->masterInfo.peerList,
            _ckptPeerMasterHdlWalk,
            &masterHdl,
            sizeof(masterHdl));
    CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Deputy update failed rc[0x %x]\n",rc),
            rc);
    rc = clHandleDestroy(gCkptSvr->masterInfo.masterDBHdl,
            masterHdl);
    clLogDebug(CL_CKPT_AREA_DEPUTY, CL_CKPT_CTX_CKPT_DEL, 
            "Master handle [%#llX] has been deleted", 
            masterHdl);
    gCkptSvr->masterInfo.masterHdlCount--;        

exitOnErrorBeforeHdlCheckout:
    /*
     * Unock the master DB.
     */
    CKPT_UNLOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
    return rc;
}


/*
 * Function invoked at deputy whenever a checkpoint is unlinked
 * at master.
 */
ClRcT ckptCheckpointCalltoDelete(ClHandleT masterHdl)
{
    ClRcT                   rc              = CL_OK;
    CkptMasterDBEntryT      *pStoredData    = NULL;
    CkptXlationLookupT      lookup          = {0};
    ClUint32T               cksum           = 0;

    /*
     * Check if the deputy is up or not.
     */
    CKPT_NULL_CHECK(gCkptSvr);
    
    /*
     * Lock the master DB.
     */
    CKPT_LOCK(gCkptSvr->masterInfo.ckptMasterDBSem);

    /*
     * Locate and delete the entry from the name Xlation table.
     */
    rc = clHandleCheckout(gCkptSvr->masterInfo.masterDBHdl,
            masterHdl, (void **)&pStoredData);
    CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            (" MasterCheckpointClose failed rc[0x %x]\n",rc),
            rc);
    if(pStoredData != NULL)
    {
        pStoredData->markedDelete = CL_TRUE;
        memset(&lookup, 0, sizeof(lookup));
        clCksm32bitCompute ((ClUint8T *)pStoredData->name.value, pStoredData->name.length,
                &cksum);
        lookup.cksum = cksum;
        clNameCopy(&lookup.name, &pStoredData->name);
    }    
    rc = clHandleCheckin(gCkptSvr->masterInfo.masterDBHdl,
                         masterHdl);
    rc = clCntAllNodesForKeyDelete(gCkptSvr->masterInfo.nameXlationDBHdl,
                            (ClCntKeyHandleT)&lookup);
    clLogInfo(CL_CKPT_AREA_DEPUTY, CL_CKPT_CTX_CKPT_DEL, 
            "Ckpt [%.*s] has been deleted at deputy", 
            lookup.name.length, lookup.name.value);
                            
exitOnErrorBeforeHdlCheckout:
    /*
     * Unock the master DB.
     */
    CKPT_UNLOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
    return rc;
}



/*
 * Function invoked at deputy whenever a checkpoint's retention 
 * timer is started.
 */
 
ClRcT ckptCheckpointRetenTimerStart(ClHandleT clientHdl,ClHandleT  masterHdl,
                                    ClIocNodeAddressT localAddr)
{
    ClRcT                rc            = CL_OK;
    CkptMasterDBEntryT   *pStoredData  = NULL;
    ClTimerTimeOutT      timeOut       = {0};
    ClTimeT              retenTime     = 0;
    CkptPeerInfoT        *pPeerInfo    = NULL;
    ClHandleT            *pTimerArg    = NULL;

    /*
     * Check if the deputy is up or not.
     */
    CKPT_NULL_CHECK(gCkptSvr);
    
    /*
     * Lock the master DB.
     */
    CKPT_LOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
    
    rc = clHandleDestroy(gCkptSvr->masterInfo.clientDBHdl,
                         clientHdl); 
    CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                                  (" MasterCheckpointClose failed rc[0x %x]\n",rc),
                                  rc);
    gCkptSvr->masterInfo.clientHdlCount--;        

    rc = clCntDataForKeyGet(gCkptSvr->masterInfo.peerList, (ClPtrT)(ClWordT)localAddr,
                            (ClCntDataHandleT *)&pPeerInfo); 
    CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                                  ("Failed to get info of addr %d in peerList rc[0x %x]\n",
                                   localAddr, rc), rc);
    if ( CL_OK != (rc = clCntAllNodesForKeyDelete(pPeerInfo->ckptList,
                                                  (ClPtrT)(ClWordT)clientHdl)))
    {
        CKPT_DEBUG_E(("ClientHdl %#llX Delete from list failed", clientHdl));
    }
    rc = clHandleCheckout(gCkptSvr->masterInfo.masterDBHdl,
                          masterHdl, (void **)&pStoredData);
    CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                                  (" MasterCheckpointClose failed rc[0x %x]\n",rc),
                                  rc);
    if(pStoredData != NULL)
    {
        memset(&timeOut, 0, sizeof(ClTimerTimeOutT));
        retenTime = pStoredData->attrib.retentionDuration;
        timeOut.tsMilliSec = (retenTime/
                              (CL_CKPT_NANO_TO_MICRO * CL_CKPT_NANO_TO_MICRO));
        if( NULL != (pTimerArg = clHeapCalloc(1, sizeof(ClHandleT) )) )
        {
            *pTimerArg = masterHdl;
        }

        rc = clTimerCreateAndStart(timeOut,
                                   CL_TIMER_ONE_SHOT,
                                   CL_TIMER_SEPARATE_CONTEXT,
                                   _ckptRetentionTimerExpiry,
                                   pTimerArg, 
                                   &pStoredData->retenTimerHdl);
        clLogDebug(CL_CKPT_AREA_DEPUTY, CL_CKPT_CTX_CKPT_OPEN, 
                   "Retention timer has been started for [%d] sec [%d] millsec",
                   timeOut.tsSec, timeOut.tsMilliSec);
    }    
    clHandleCheckin(gCkptSvr->masterInfo.masterDBHdl,
                    masterHdl);
            
    exitOnErrorBeforeHdlCheckout:
    /*
     * Unock the master DB.
     */
    CKPT_UNLOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
    return rc;
}



/*
 * Function called by the master to update the deputy at run time
 * whenever there is a change in master's meta data.
 */
 
ClRcT VDECL_VER(clCkptDeputyDynamicUpdate, 4, 0, 0)(ClVersionT          *pVersion,
                                CkptDynamicSyncupT  updateFlag,
                                CkptDynamicInfoT    *pDynamicInfo)
{
    ClRcT    rc = CL_OK;
    ClIocPhysicalAddressT srcAddr;

    rc = clRmdSourceAddressGet(&srcAddr);
    if( CL_OK != rc )
    {
        clLogError(CL_CKPT_AREA_DEPUTY, "UPD",
                   "Failed to get the src address rc[0x %x]", rc);
        return rc;
    }

     /*
     * Check if the deputy is up or not.
     */
    CKPT_NULL_CHECK(gCkptSvr);

    if(!clCpmIsSCCapable())
    {
        return CL_OK;
    }

    /*
     * Lock the master DB.
     */
    CKPT_LOCK(gCkptSvr->masterInfo.ckptMasterDBSem);

    /*
     * Skip for the master or sender
     */
    if((gCkptSvr->masterInfo.masterAddr == gCkptSvr->localAddr) ||
        (srcAddr.nodeAddress == gCkptSvr->localAddr))
    {
        CKPT_UNLOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
        return CL_OK;
    }

    CKPT_UNLOCK(gCkptSvr->masterInfo.ckptMasterDBSem);

    /*
     * Based on the update flag, take appropriate action.
     */
    switch(updateFlag)
    {
        case CL_CKPT_CKPT_CLOSE:
            rc = ckptCheckpointClose(pDynamicInfo->clientHdl,
                             pDynamicInfo->masterHdl,
                             pDynamicInfo->data->ckptUpdateInfoT.replicaAddr);
            break;
        case CL_CKPT_CLOSE_MARKED_DELETE:
        case CL_CKPT_MARKED_DELETE:
            rc = ckptCheckpointDelete(pDynamicInfo->clientHdl,
                    pDynamicInfo->masterHdl,
                    pDynamicInfo->data->ckptUpdateInfoT.replicaAddr,
                    updateFlag);
            break;
        case CL_CKPT_START_TIMER:
            rc = ckptCheckpointRetenTimerStart(pDynamicInfo->clientHdl,
                                pDynamicInfo->masterHdl,
                            pDynamicInfo->data->ckptUpdateInfoT.replicaAddr);
            break;
        case CL_CKPT_CKPT_UNLINK:
            rc  = ckptCheckpointCalltoDelete(pDynamicInfo->masterHdl);
            break;
        case CL_CKPT_COMM_UPDATE:
            switch(pDynamicInfo->data->discriminant)
            {
                case CKPTUPDATEINFOTRETENTIME:
                   rc = ckptCheckpointRetenTimerUpdate(
                             pDynamicInfo->masterHdl,
                             pDynamicInfo->data->ckptUpdateInfoT.retenTime);
                    break;
                case CKPTUPDATEINFOTACTIVEADDR:
                    rc = ckptCheckpointActiveAddrChange(
                            pDynamicInfo->masterHdl,
                            pDynamicInfo->data->ckptUpdateInfoT.activeAddr);
                    break;
                default:
                    break;
            }
            break;
        default:
            rc = CL_CKPT_ERR_INVALID_STATE;
            break;
    }
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Ckpt: Dynamic update failed with Tag [%x] rc[0x %x]\n", 
             updateFlag,rc), rc);
exitOnError:
    return rc;
}



/*
 * Function called at deputy whenever a checkpoint is replicated.
 */
 
ClRcT VDECL_VER(clCkptDeputyReplicaListUpdate, 4, 0, 0)( ClVersionT          *pVersion,
                                     ClHandleT           masterHdl,
                                     ClIocNodeAddressT   peerAddr,
                                     ClUint32T           addToList)
{
    CkptMasterDBEntryT      *pMasterDBEntry = NULL;
    CkptPeerInfoT           *pPeerInfo      = NULL;
    ClRcT                    rc             = CL_OK;
    ClIocPhysicalAddressT    srcAddr;

    rc = clRmdSourceAddressGet(&srcAddr);
    if( CL_OK != rc )
    {
        clLogError(CL_CKPT_AREA_DEPUTY, "UPD",
                   "Failed to get the src address rc[0x %x]", rc);
        return rc;
    }
    /*
     * Check if the deputy is up or not.
     */
    CL_CKPT_SVR_EXISTENCE_CHECK;

    if(!clCpmIsSCCapable())
    {
        return CL_OK;
    }

    /*
     * Lock the master DB.
     */
    CKPT_LOCK(gCkptSvr->masterInfo.ckptMasterDBSem);

    /*
     * Skip for the master or sender.
     */
    if((gCkptSvr->masterInfo.masterAddr == gCkptSvr->localAddr) ||
        (srcAddr.nodeAddress == gCkptSvr->localAddr))
    {
        CKPT_UNLOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
        return CL_OK;
    }

    /*
     * Verify th eversion.
     */
    rc = clVersionVerify(&gCkptSvr->versionDatabase,pVersion);
    CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Ckpt: Version mismatch rc[0x %x]\n", rc), rc);

    /*
     * Update the checkpoint's replica list.
     */
    rc = ckptSvrHdlCheckout( gCkptSvr->masterInfo.masterDBHdl,
                             masterHdl,
                             (void **)&pMasterDBEntry);
    CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Failed to add replica to replica list[0x %x]\n",rc),
            rc);

    if( addToList !=  CL_CKPT_REPLICA_DEL )
    {
        rc = clCntNodeAdd( pMasterDBEntry->replicaList,
            (ClCntKeyHandleT)(ClWordT)peerAddr, 
            (ClCntDataHandleT)(ClWordT)peerAddr, NULL);
        if (CL_GET_ERROR_CODE(rc) == CL_ERR_DUPLICATE)   rc = CL_OK;
        CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Failed to add replica to replica list[0x %x]\n",rc),
            rc);
        clLogDebug(CL_CKPT_AREA_PEER, CL_CKPT_CTX_CKPT_DEL, 
                   "Replica address [%d] has been added", peerAddr);
    }
    else
    {
        rc = clCntAllNodesForKeyDelete(pMasterDBEntry->replicaList, 
                                       (ClCntKeyHandleT)(ClWordT)peerAddr);
        CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Failed to remove replica to replica list[0x %x]\n", rc),
            rc);
        clLogDebug(CL_CKPT_AREA_PEER, CL_CKPT_CTX_CKPT_DEL, 
                   "Replica address [%d] has been removed", peerAddr);
    }
    
    rc = clCntDataForKeyGet( gCkptSvr->masterInfo.peerList,
            (ClCntDataHandleT)(ClWordT)peerAddr,
            (ClCntDataHandleT *)&pPeerInfo);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            (" PeerInfo get failed rc[0x %x]\n",rc),
            rc);
    
    if( addToList != CL_CKPT_REPLICA_DEL )
    {
        /*
         * Increment the replica count associated with th epeer.
         */
        pPeerInfo->replicaCount++;
        /*
         * Add the master handle to address list 
         */
        if(addToList && 
                CL_CKPT_IS_COLLOCATED(pMasterDBEntry->attrib.creationFlags))
        {
            _ckptPeerListMasterHdlAdd(masterHdl, CL_CKPT_UNINIT_ADDR, peerAddr);
        }    
    }
    else
    {
        pPeerInfo->replicaCount--;
    }
exitOnError:
    clHandleCheckin( gCkptSvr->masterInfo.masterDBHdl, masterHdl); 
exitOnErrorBeforeHdlCheckout:
    CKPT_UNLOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
    return rc;
}



/*
 * Function called at deputy whenever a checkpoint has been created.
 */
 
ClRcT 
VDECL_VER(clCkptDeputyCkptCreate, 4, 0, 0)(ClHandleT                           masterHdl,
                       ClHandleT                           clientHdl,
                       ClNameT                             *pName,
                       ClCkptCheckpointCreationAttributesT *pCreateAttr,
                       ClIocNodeAddressT                   localAddr,
                       ClIocPortT                          localPort,
                       ClVersionT                          *pVersion)
{
    ClRcT               rc         = CL_OK;
    ClUint32T           cksum      = 0;
    CkptXlationLookupT  lookup     = {0};
    ClIocNodeAddressT   activeAddr = {0};
    ClIocPhysicalAddressT srcAddr;

    rc = clRmdSourceAddressGet(&srcAddr);
    if( CL_OK != rc )
    {
        clLogError(CL_CKPT_AREA_DEPUTY, "CRE",
                   "Failed to get the src address rc[0x %x]", rc);
        return rc;
    }

    CKPT_DEBUG_T(("MasterHdl: %#llX Name: %s, localAddr : %d,"
                  "localPort: %d\n", masterHdl, pName->value,
                   localAddr, localPort));
                   
    /*
     * Check if the deputy is up or not.
     */
    CL_CKPT_SVR_EXISTENCE_CHECK;

    if(!clCpmIsSCCapable())
    {
        return CL_OK;
    }

    /*
     * Lock the master DB.
     */
    CKPT_LOCK(gCkptSvr->masterInfo.ckptMasterDBSem);

    /*
     * Skip for the master or sender
     */
    if((gCkptSvr->masterInfo.masterAddr == gCkptSvr->localAddr) ||
            (srcAddr.nodeAddress == gCkptSvr->localAddr))
    {
        CKPT_UNLOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
        return CL_OK;
    }

    clCksm32bitCompute ((ClUint8T *)pName->value, pName->length, &cksum);
    lookup.cksum = cksum;
    
    clNameCopy(&lookup.name, pName);

    /*
     * Update the checkpoint metadata.
     */
    if (CL_OK != 
           (rc = _ckptMasterHdlInfoFill(masterHdl, pName,
                                        pCreateAttr,
                                        localAddr, CL_CKPT_SOURCE_DEPUTY,
                                        &activeAddr)))
    {
        CKPT_UNLOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
        return rc;
    }
    
    /* 
     * Add corresponding entry to Xlation table.
     */
    if (CL_OK != (rc = _ckptMasterXlationDBEntryAdd(pName, cksum, masterHdl)))
    {
        clHandleDestroy(gCkptSvr->masterInfo.masterDBHdl,masterHdl);
        CKPT_UNLOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
        return rc;
    }
    /* 
     * Create the client handle and add the client info.
     */
    if (CL_OK != (rc = _ckptClientHdlInfoFill(masterHdl, clientHdl, 
                                              CL_CKPT_SOURCE_DEPUTY)))
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
    if (CL_OK != (rc = _ckptMasterPeerListHdlsAdd(clientHdl, masterHdl,
                                              localAddr, localPort,
                                              CL_CKPT_CREAT,
                                              pCreateAttr->creationFlags)))
    {
        clHandleDestroy(gCkptSvr->masterInfo.clientDBHdl,
                clientHdl);
        clCntAllNodesForKeyDelete(
                gCkptSvr->masterInfo.nameXlationDBHdl,
                (ClCntKeyHandleT)&lookup);
        clHandleDestroy(gCkptSvr->masterInfo.masterDBHdl,masterHdl);
    }
    clLogDebug(CL_CKPT_AREA_DEPUTY, CL_CKPT_CTX_CKPT_OPEN, 
               "Deputy updated for creation, ckptName [%.*s] "
               "mastHdl [%#llX] OpenHdl [%#llX]", lookup.name.length, 
               lookup.name.value, masterHdl, clientHdl);
    
    /*
     * Unlock the master DB.
     */
    CKPT_UNLOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
    return rc;
}



/*
 * Function called at deputy whenever a checkpoint has been opened.
 */
 
ClRcT 
VDECL_VER(clCkptDeputyCkptOpen, 4, 0, 0)(ClHandleT         storedDBHdl,
                     ClHandleT         clientHdl,
                     ClIocNodeAddressT localAddr,
                     ClIocPortT        localPort,
                     ClVersionT        *pVersion)
{
    ClRcT                rc           = CL_OK;
    CkptMasterDBEntryT  *pStoredData  = NULL;
    ClIocPhysicalAddressT srcAddr;

    rc = clRmdSourceAddressGet(&srcAddr);
    if( CL_OK != rc )
    {
        clLogError(CL_CKPT_AREA_DEPUTY, "OPE",
                   "Failed to get the src address rc[0x %x]", rc);
        return rc;
    }

    clLogDebug(CL_CKPT_AREA_DEPUTY, CL_CKPT_CTX_DEP_SYNCUP, "Opening the handle [%#llX] at address [%d] for MasterHandle [%#llX]",
        clientHdl, localAddr, storedDBHdl);
                   
    /*
     * Check if the deputy is up or not.
     */
    CL_CKPT_SVR_EXISTENCE_CHECK;

    if(!clCpmIsSCCapable())
    {
        return CL_OK;
    }

    /*
     * Lock the master DB.
     */
    CKPT_LOCK(gCkptSvr->masterInfo.ckptMasterDBSem);

    /*
     * Skip for the master or sender
     */
    if((gCkptSvr->masterInfo.masterAddr == gCkptSvr->localAddr) ||
            (srcAddr.nodeAddress == gCkptSvr->localAddr))
    {
        CKPT_UNLOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
        return CL_OK;
    }

    rc = clHandleCheckout(gCkptSvr->masterInfo.masterDBHdl, 
            storedDBHdl,(void **) &pStoredData);  
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Master DB creation failed rc[0x %x]\n",rc),
            rc);
            
    /* 
     * Create the client handle.Add the info.
     */
    if (CL_OK != (rc = _ckptClientHdlInfoFill(storedDBHdl, clientHdl,
                                            CL_CKPT_SOURCE_DEPUTY)))
    {
        clLogDebug(CL_CKPT_AREA_DEPUTY, CL_CKPT_CTX_DEP_SYNCUP, "Checkpoint open on deputy failed for clientHdl[%#llX] rc[0x %x]",
            clientHdl, rc);
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
        clLogDebug(CL_CKPT_AREA_DEPUTY, CL_CKPT_CTX_DEP_SYNCUP, "Checkpoint open on deputy failed for addr[%d] rc[0x %x]", localAddr, rc);
        clHandleDestroy(gCkptSvr->masterInfo.clientDBHdl, clientHdl);
        clHandleCheckin(gCkptSvr->masterInfo.masterDBHdl, storedDBHdl);
        CKPT_UNLOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
        return rc;
    }
    /*
     * In case of sync checkpoint, add the masterHdl to the list of 
     * masterHdls for that node.
     */
    if((pStoredData->attrib.creationFlags & CL_CKPT_WR_ALL_REPLICAS))
    {
        _ckptPeerListMasterHdlAdd(storedDBHdl, CL_CKPT_UNINIT_ADDR,
                                  localAddr);
    }

    /*
     * Increment the checkpoint ref count.
     */
    pStoredData->refCount = pStoredData->refCount+1;

    /* 
     * Stop/Destroy the retention timer if runing.
     */
    if(pStoredData->retenTimerHdl != 0)
    {
        clTimerDelete(&pStoredData->retenTimerHdl);
    }
    clHandleCheckin(gCkptSvr->masterInfo.masterDBHdl, 
            storedDBHdl);
    clLogDebug(CL_CKPT_AREA_DEPUTY, CL_CKPT_CTX_CKPT_OPEN, 
               "Deputy updated for Open, mastHdl [%#llX] OpenHdl [%#llX]", 
                storedDBHdl, clientHdl);
            
 exitOnError:
    /*
     * Unlock the master DB.
     */
    CKPT_UNLOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
    return rc;
}



/*
 * Function to update the peerlist at deputy whenever a new code comes up.
 */
 
ClRcT VDECL_VER(clCkptDeputyPeerListUpdate, 4, 0, 0)(ClVersionT         version, 
                                 ClIocNodeAddressT  peerAddr,
                                 ClUint8T           credential)
{
    ClRcT rc = CL_OK;
    ClIocPhysicalAddressT srcAddr;

    rc = clRmdSourceAddressGet(&srcAddr);
    if( CL_OK != rc )
    {
        clLogError(CL_CKPT_AREA_DEPUTY, "UPD",
                   "Failed to get the src address rc[0x %x]", rc);
        return rc;
    }

    /*
     * Check if the deputy is up or not.
     */
    CL_CKPT_SVR_EXISTENCE_CHECK;

    if(!clCpmIsSCCapable())
    {
        return CL_OK;
    }

    /*  
     * Skip for the master or sender or peerAddr == localAddr.
     */
    if(gCkptSvr->masterInfo.masterAddr == gCkptSvr->localAddr
       || srcAddr.nodeAddress == gCkptSvr->localAddr
       || peerAddr == gCkptSvr->localAddr)
    {
        // Lock is never taken: CKPT_UNLOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
        return CL_OK;
    }
   
    /*
     * Verify the received version info.
     */
    rc = clVersionVerify(&gCkptSvr->versionDatabase, &version);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Ckpt: Version mismatch rc[0x %x]\n", rc), rc);

    /*
     * Add to the peerlist.
     */
    rc = clCkptMasterPeerUpdate(0, CL_CKPT_SERVER_UP,
                                peerAddr, credential);
    if (CL_GET_ERROR_CODE(rc) == CL_ERR_DUPLICATE)   rc = CL_OK;


 exitOnError:
    return rc;
}

ClRcT VDECL_VER(clCkptCreateInfoDeputyUpdate, 4, 0, 0)(ClVersionT         *pVersion,
                                   CkptOpenInfoT      *pOpenInfo,  
                                   CkptCreateInfoT    *pCreateInfo)
{ 
    ClRcT  rc = CL_OK;
    CkptMasterDBClientInfoT clientDBInfo   = {0};
    CkptXlationDBEntryT     xlationInfo    = {0};
    CkptPeerListInfoT       peerListInfo   = {0};
    CkptNodeListInfoT       *pNodeListInfo = NULL;
    ClUint32T                count         = 0;
    CkptMasterDBEntryIDLT   *pMasterDBInfo = NULL;
    CkptMasterDBEntryT   *pMasterDBEntry  = NULL;

    if(!clCpmIsSCCapable())
    {
        return CL_OK;
    }

    /*
     * Validate the input parameter.
     */
    CKPT_NULL_CHECK(pOpenInfo);
    CKPT_NULL_CHECK(pCreateInfo);
    /*be the safer side */
    peerListInfo.nodeListInfo = NULL;

    rc = clVersionVerify(&gCkptSvr->versionDatabase,pVersion);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Ckpt: Version mismatch rc[0x %x]\n", rc), rc);
    switch(pOpenInfo->discriminant)        
    {
        case CKPTOPENINFOTMASTERHDL:
            {
                clientDBInfo.clientHdl = pCreateInfo->clientHdl;        
                clientDBInfo.masterHdl = pOpenInfo->ckptOpenInfoT.masterHdl; 

                peerListInfo.nodeAddr = pCreateInfo->activeAddr;        
                peerListInfo.numOfOpens = 1;        
                pNodeListInfo = (CkptNodeListInfoT *)clHeapCalloc(1,
                        sizeof(CkptNodeListInfoT) * 1); 
                if(pNodeListInfo == NULL)
                {
                    rc = CL_CKPT_ERR_NO_MEMORY;
                    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                            ("Ckpt: Failed to allocate memory rc[0x %x]\n", rc), rc);
                }
                peerListInfo.nodeListInfo = pNodeListInfo;
                pNodeListInfo->appPortNum  = pCreateInfo->appPortNum;
                pNodeListInfo->appAddr  = pCreateInfo->activeAddr;
                pNodeListInfo->clientHdl = pCreateInfo->clientHdl;
                rc = clHandleCheckout( gCkptSvr->masterInfo.masterDBHdl,
                                       pOpenInfo->ckptOpenInfoT.masterHdl, 
                                       (void **)&pMasterDBEntry);
                CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                            ("Ckpt: Failed to allocate memory rc[0x %x]\n", rc), rc);
                if(pMasterDBEntry != NULL)
                {
                    pMasterDBEntry->refCount++;
                }    
                rc = clHandleCheckin( gCkptSvr->masterInfo.masterDBHdl,
                                      pOpenInfo->ckptOpenInfoT.masterHdl);
                
                break;
            }
        case CKPTOPENINFOTPMASTERDBINFO:
            {
                pMasterDBInfo = pOpenInfo->ckptOpenInfoT.pMasterDBInfo;
                if( pMasterDBInfo == NULL)
                {
                    rc = CL_CKPT_ERR_INVALID_STATE;
                }
                CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR, ("Ckpt: Passed info got corrupted rc[0x %x]\n", rc), rc);
                xlationInfo.mastHdl  = pMasterDBInfo->ckptMasterHdl;  
                clNameCopy(&xlationInfo.name, &pMasterDBInfo->name);
                clCksm32bitCompute ((ClUint8T *)xlationInfo.name.value, 
                        xlationInfo.name.length,
                        &xlationInfo.cksum);
                rc = ckptXlatioEntryUnpack(1,&xlationInfo);
                CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR, ("Ckpt: Failed to unpack the Xlation Entry rc[0x %x]\n", rc), rc);
                rc = ckptMasterDBInfoUnpack(1,pMasterDBInfo);
                CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR, ("Ckpt: Failed to unpack the MasterInfo rc[0x %x]\n", rc), rc);
                clientDBInfo.clientHdl = pCreateInfo->clientHdl;        
                clientDBInfo.masterHdl = pMasterDBInfo->ckptMasterHdl; 

                peerListInfo.nodeAddr = pCreateInfo->activeAddr;        
                peerListInfo.numOfOpens = 2;        
                pNodeListInfo = (CkptNodeListInfoT *)clHeapCalloc(1,
                        sizeof(CkptNodeListInfoT) * 2); 
                if(pNodeListInfo == NULL)
                {
                    rc = CL_CKPT_ERR_NO_MEMORY;
                    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                            ("Ckpt: Failed to allocate memory rc[0x %x]\n", rc), rc);
                }
                peerListInfo.nodeListInfo = pNodeListInfo;
                for( count = 0 ; count < 2; count++)
                {
                    pNodeListInfo->appPortNum  = pCreateInfo->appPortNum;
                    pNodeListInfo->appAddr  = pCreateInfo->activeAddr;
                    if( count == 0)
                    {
                        pNodeListInfo->clientHdl = pMasterDBInfo->ckptMasterHdl;
                    }
                    else
                    {
                        pNodeListInfo->clientHdl = pCreateInfo->clientHdl;
                    }  
                    pNodeListInfo++;
                }
                break;
            }
        default:
            rc = CL_CKPT_ERR_INVALID_STATE;
            break;
    }
    rc = ckptClientDBInfoUnpack(1,&clientDBInfo);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Ckpt: Failed to unpack the ClientDB Info rc[0x %x]\n", rc), rc);
    rc = ckptPeerListInfoUnpack(1,&peerListInfo);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Ckpt: Failed to unpack the PeerList info rc[0x %x]\n", rc), rc);
exitOnError:            
    if(peerListInfo.nodeListInfo != NULL)
        clHeapFree(peerListInfo.nodeListInfo);
    return rc;
}

