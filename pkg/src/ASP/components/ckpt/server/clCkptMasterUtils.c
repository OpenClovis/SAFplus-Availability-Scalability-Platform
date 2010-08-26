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
/*****************************************************************************
 * ModuleName  : ckpt                                                          
File        : clCkptMasterUtils.c
 ****************************************************************************/

/****************************************************************************
 * Description :                                                                
 *
 *  This file contains CKPT master node utils functionality.
 *
 *
 ****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <clHandleApi.h>
#include <ipi/clHandleIpi.h>

 #include <clCkptMaster.h>
 #include <clCkptUtils.h>
 #include <clCkptIpi.h>
 #include <clCkptMasterUtils.h>

extern CkptSvrCbT  *gCkptSvr;


/*
 * Replica list's key compare callback function.
 */
 
ClInt32T ckptReplicaListKeyComp(ClCntKeyHandleT key1,
                                ClCntKeyHandleT key2)
{
    // wudn't this suffice - return ((ClWordT)key1 - (ClWordT)key2); 
    ClIocNodeAddressT   info1 = (ClIocNodeAddressT)(ClWordT)key1;
    ClIocNodeAddressT   info2 = (ClIocNodeAddressT)(ClWordT)key2;

    if(info1 == info2)
    {
        return 0;
    }
    else
        return 1;
}



/*
 * Replica list's node delete callback function.
 */
 
void ckptReplicaListDeleteCallback(ClCntKeyHandleT  userKey,
                                   ClCntDataHandleT userData)
{
    return;
}




/*
 * ckptMasterHdlInfoFill()
 * - create the masterHdl.
 * - update master entry.
 * - create replica list. 
 * - update the entry.
 * - if it is any failure, deallocate the resources.
 * - exit.
 */
 
ClRcT
_ckptMasterHdlInfoFill(ClHandleT                           masterHdl,
                       ClNameT                             *pName,
                       ClCkptCheckpointCreationAttributesT *pCreateAttr,
                       ClIocNodeAddressT                   localAddr,
                       ClUint8T                            source,
                       ClIocNodeAddressT                   *pActiveAddr)
{
    ClRcT                rc             = CL_OK;
    CkptMasterDBEntryT  *pMasterDBEntry = NULL;

    CKPT_DEBUG_T(("ckptName: %s, LocalAddr: %d\n", pName->value, localAddr));

    /*
     * Checkpoints in both master and deputy should have same master hdls.
     * clHandleCreateSpecifiedHandle ensures this. If the caller is master
     * server masterHdl has already been created. But in case of deputy the
     * handle needs to be created.
     */
    if(source == CL_CKPT_SOURCE_DEPUTY)
    {
        if ((CL_OK != (rc = clHandleCreateSpecifiedHandle(
                gCkptSvr->masterInfo.masterDBHdl, 
                sizeof(CkptMasterDBEntryT),
                masterHdl))) && 
                (CL_GET_ERROR_CODE(rc) != CL_ERR_ALREADY_EXIST))
        {
            CKPT_DEBUG_E(("Handle create err: rc [0x %x]\n",rc));
            return rc;
        }
    }
    
    /*
     * Associate the checkpoint metadata with the masterHdl.
     */
    rc = clHandleCheckout(gCkptSvr->masterInfo.masterDBHdl, 
            masterHdl, (void **)&pMasterDBEntry);  
    CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Handle Checkout failed rc[0x %x]\n",rc),
            rc);

    memcpy(&pMasterDBEntry->attrib, pCreateAttr,
            sizeof(ClCkptCheckpointCreationAttributesT)); 
    clNameCopy(&pMasterDBEntry->name, pName);
    pMasterDBEntry->markedDelete  = 0;
    pMasterDBEntry->refCount      = 1;
    pMasterDBEntry->retenTimerHdl = 0;
    pMasterDBEntry->activeRepHdl  = masterHdl;

    /*
     * Keeping the active server same for any of kind of checkpoints
     */
    if (CL_CKPT_IS_COLLOCATED(pCreateAttr->creationFlags)) 
    {
        /*
         * Collocated checkpoint case.
         * return CL_CKPT_UNINIT_ADDR as active replica.
         */
        pMasterDBEntry->activeRepAddr   = CL_CKPT_UNINIT_ADDR;
    }
    else
    {
        /* 
         * Make the server that sent the creation request as 
         * active replica.
         */
        pMasterDBEntry->activeRepAddr = localAddr;
    }
    pMasterDBEntry->prevActiveRepAddr = pMasterDBEntry->activeRepAddr;
    
    *pActiveAddr = pMasterDBEntry->activeRepAddr;
    /* 
     * Create the replica list and add the requesting node to it.
     */
    rc = clCntLlistCreate(ckptReplicaListKeyComp,
            ckptReplicaListDeleteCallback,
            ckptReplicaListDeleteCallback,
            CL_CNT_UNIQUE_KEY,
            &pMasterDBEntry->replicaList);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Replica List create failed rc[0x %x]\n",rc),
            rc);
    rc = clCntNodeAdd( pMasterDBEntry->replicaList,
            (ClCntKeyHandleT)(ClWordT)localAddr, 
            (ClCntDataHandleT)(ClWordT)localAddr, NULL);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Replica List add failed rc[0x %x]\n",rc),
            rc);
            
    /*
     * Increment the master handle count.
     */
    gCkptSvr->masterInfo.masterHdlCount++;
    
    rc = clHandleCheckin(gCkptSvr->masterInfo.masterDBHdl, 
            masterHdl);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Handle checkin failed rc[0x %x]\n",rc),
            rc);
            
    return rc;        
exitOnError:
    clHandleCheckin(gCkptSvr->masterInfo.masterDBHdl, 
            masterHdl);
exitOnErrorBeforeHdlCheckout:
    clHandleDestroy(gCkptSvr->masterInfo.masterDBHdl,
                    masterHdl);
    return rc;
}



/*
 * ckptClientHdlDBUpdate()
 * - create the clientHdl.
 * - update clientDB entry.
 * - if it is any failure, deallocate the resources.
 * - exit.
 */
 
ClRcT
_ckptClientHdlInfoFill(ClHandleT   masterHdl,
                       ClHandleT   clientHdl,
                       ClUint8T    source)
{
    ClRcT                    rc              = CL_OK;
    CkptMasterDBClientInfoT  *pClientEntry   = NULL;

    CKPT_DEBUG_T(("masterHdl: %#llX\n", masterHdl));

    /*
     * Checkpoints in both master and deputy should have same client hdls.
     * clHandleCreateSpecifiedHandle ensures this. If the caller is master
     * server, clientHdl has already been created. But in case of deputy the
     * handle needs to be created.
     */
    if(source == CL_CKPT_SOURCE_DEPUTY)
    {
        rc = clHandleCreateSpecifiedHandle(
                            gCkptSvr->masterInfo.clientDBHdl, 
                            sizeof(CkptMasterDBClientInfoT),
                            clientHdl);
        if( (CL_OK != rc))/* && (CL_GET_ERROR_CODE(rc) != CL_ERR_ALREADY_EXIST) )*/
        {
            CKPT_DEBUG_E(("Client Handle create err: rc [0x %x]\n",rc));
            return rc;
        }
        rc = CL_OK;
    }
    rc = clHandleCheckout(gCkptSvr->masterInfo.clientDBHdl, 
            clientHdl, 
            (void **)&pClientEntry);
    CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Handle checkout failed rc[0x %x]\n",rc),
            rc);
    pClientEntry->clientHdl  =  clientHdl;
    pClientEntry->masterHdl  =  masterHdl;
    
    rc = clHandleCheckin(gCkptSvr->masterInfo.clientDBHdl, 
            clientHdl);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Client Hdl checkin failed rc[0x %x]\n",rc),
            rc);
    /*
     * Increment the client handle count.
     */
    gCkptSvr->masterInfo.clientHdlCount++;        
    clLogDebug("MAS", "CKP", 
            "HandleCount [%d] clietHdl [%#llX]", gCkptSvr->masterInfo.clientHdlCount, 
             clientHdl);
    return rc;
exitOnError:
    clHandleCheckin(gCkptSvr->masterInfo.clientDBHdl, 
            clientHdl);
exitOnErrorBeforeHdlCheckout:
    clHandleDestroy(gCkptSvr->masterInfo.clientDBHdl,
            clientHdl);
    return rc;
}



/*
 * _ckptPeerListMasterHdlAdd()
 * - Get the peerInfo of peerAddr.
 * - delete the handle if it is there.
 * - Add the masterHdl to the mastHdlList.
 * - If it is failure,deallocate the resources.
 * - Exit.
 */
 
ClRcT 
_ckptPeerListMasterHdlAdd( ClHandleT          masterHdl,
                           ClIocNodeAddressT  oldActAddr,
                           ClIocNodeAddressT  newActAddr)
{
    ClRcT              rc              = CL_OK;
    CkptPeerInfoT      *pPeerInfo      = NULL;
    ClCntDataHandleT   dataHdl         = 0;
    ClHandleT          *pMasterHandle  = NULL;

    CKPT_DEBUG_T(("MasterHdl: %#llX oldActAddr: %d NewActAddr: %d", masterHdl,
                    oldActAddr, newActAddr));
                    
    /*
     * In case of node restart and collocated checkpoint the prevActiveAddr
     * will be UNINIT ADDR.
     */
    if(oldActAddr != CL_CKPT_UNINIT_ADDR)
    {
        /*
         * Obtain the corresponding entry from peer list.
         */
        rc = clCntDataForKeyGet(gCkptSvr->masterInfo.peerList,
                (ClPtrT)(ClWordT)oldActAddr, &dataHdl);
        CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                ("Peeraddr [%d] doesn't exist in peerList rc[0x %x]\n",
                 oldActAddr, rc), rc);
        pPeerInfo = (CkptPeerInfoT *)dataHdl;         
        
        /*
         * Delete the master handle from the old AR's peer list.
         */
        rc = clCntAllNodesForKeyDelete(pPeerInfo->mastHdlList, &masterHdl);
        pPeerInfo->replicaCount--;
    }    

    if(newActAddr != CL_CKPT_UNINIT_ADDR)
    {
        rc = clCntDataForKeyGet(gCkptSvr->masterInfo.peerList,
                (ClPtrT)(ClWordT)newActAddr, &dataHdl);
        CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                ("Peeraddr [%d] doesn't exist in peerList rc[0x %x]\n",
                 newActAddr, rc), rc);
        pPeerInfo = (CkptPeerInfoT *)dataHdl;         
        
        /*
         * Allocate memory for the 64bit handle.
         */
        pMasterHandle = clHeapAllocate(sizeof(*pMasterHandle)); // Free on Error
        if(NULL == pMasterHandle)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Memory Allocation Failed\n"));
            goto exitOnError;
        }
        *pMasterHandle = masterHdl;
        /*
         * Add the master handle to the new AR's peer list.
         */
        if( CL_OK != (rc = clCntNodeAdd(pPeerInfo->mastHdlList, 
                        pMasterHandle,
                        0, NULL)))
        {
            clHeapFree(pMasterHandle);
            if (CL_GET_ERROR_CODE(rc) == CL_ERR_DUPLICATE)   
                rc = CL_OK;
            else
                CKPT_DEBUG_E(("MasterHdlList add failed while adding hdl=%#llX : \
                               rc [0x %x]\n", masterHdl, rc)); 
        }
        pPeerInfo->replicaCount++;
    }
exitOnError:    
    return rc;
}



/*
 * _ckptMasterPeerListiHdlsAdd()
 * - Get the peerInfo of peerAddr.
 * - Add the clientHdl to the ClientList.
 * - Add the masterHdl to the mastHdlList.
 * - Increment replica count.
 * - If it is failure,deallocate the resources.
 * - Exit.
 */
 
ClRcT 
_ckptMasterPeerListHdlsAdd(ClHandleT            clientHdl,
                           ClHandleT            masterHdl,
                           ClIocNodeAddressT    nodeAddr,
                           ClIocPortT           portId,
                           ClUint32T            openFlag,
                           ClCkptCreationFlagsT createFlags)
                           
{
    ClRcT              rc              = CL_OK;
    CkptPeerInfoT      *pPeerInfo      = NULL;
    ClCntDataHandleT   dataHdl         = 0;
    CkptNodeListInfoT  *pNodeListInfo  = NULL; 
    ClHandleT          *pMasterHandle  = NULL;

    CKPT_DEBUG_T(("clientHdl: %#llX, MasterHdl: %#llX, peerAddr: %d,"
                "openFlag : %d\n", clientHdl, masterHdl, nodeAddr,
                openFlag));

    rc = clCntDataForKeyGet(gCkptSvr->masterInfo.peerList,
            (ClPtrT)(ClWordT)nodeAddr, &dataHdl);
    if( CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST )
    {
        /*
         * This peer address doesn't exist on PeerList, before ckpt server
         * comes up, this open call has come from this node, so creating entry
         * for that node 
         */
         rc = _ckptMasterPeerListInfoCreate(nodeAddr, CL_CKPT_CREDENTIAL_POSITIVE, 0);
         if( CL_OK != rc )
         {
             clLogError("MAS", "CKP", "Failed to add node [%d] to peer list", nodeAddr);
             goto exitOnErrorBeforeHdlCheckout;
         }
        rc = clCntDataForKeyGet(gCkptSvr->masterInfo.peerList,
                                (ClPtrT)(ClWordT)nodeAddr, &dataHdl);
    }
    CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("peerAddr [%d] doesn't exist in peerList rc[0x %x]\n",
             nodeAddr, rc), rc);
    pPeerInfo = (CkptPeerInfoT *)dataHdl;         
    if ((openFlag == CL_CKPT_CREAT) && (!CL_CKPT_IS_COLLOCATED(createFlags)))
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
        *pMasterHandle = masterHdl;

        if( CL_OK != (rc = clCntNodeAdd(pPeerInfo->mastHdlList, 
                        pMasterHandle,
                        0, NULL)))
        {
           clHeapFree(pMasterHandle);
           CKPT_DEBUG_E(("MasterHdlList add failed: rc [0x %x]\n",rc)); 
           return rc;
        }
        pPeerInfo->replicaCount++;
    }

    pNodeListInfo         = (CkptNodeListInfoT *)clHeapCalloc(1,
            sizeof(CkptNodeListInfoT));
    if(pNodeListInfo == NULL)
    {
        rc = CL_CKPT_ERR_NO_MEMORY;
        CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                ("Failed to allocate the memory rc[0x %x]\n", rc), rc);
    }
    pNodeListInfo->clientHdl  = clientHdl;
    pNodeListInfo->appPortNum = portId;
    pNodeListInfo->appAddr    = nodeAddr;

    if( CL_OK != (rc = clCntNodeAdd(pPeerInfo->ckptList, 
            (ClPtrT)(ClWordT)clientHdl,  
            (ClCntDataHandleT)pNodeListInfo, NULL)))
    {
       clHeapFree(pNodeListInfo); 
       CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                ("clientHdl list add err rc[0x %x]\n", rc), rc);
    }
    return rc;
exitOnError:
    if (openFlag == CL_CKPT_CREAT)
    {
        clCntAllNodesForKeyDelete(pPeerInfo->mastHdlList, &masterHdl);
    }
exitOnErrorBeforeHdlCheckout:    
    return rc;
}



/*
 * _ckptMasterXlationTableUpdate()
 * - Fill the entries.
 * - add the entry to the xlation table.
 */
 
ClRcT 
_ckptMasterXlationDBEntryAdd(ClNameT   *pName,
                             ClUint32T  cksum,
                             ClHandleT  masterHdl)
{
    ClRcT                    rc             = CL_OK;
    CkptXlationDBEntryT     *pXlationEntry  = NULL;

    CKPT_DEBUG_T(("ckptName: %s, MasterHdl: %#llX\n",pName->value,
                masterHdl));

    /*
     * Perpare the entry to be added to the name Xlation List.
     */
    if (NULL == (pXlationEntry = (CkptXlationDBEntryT *)clHeapCalloc(1,
                    sizeof(CkptXlationDBEntryT))))
    {
        rc = CL_CKPT_ERR_NULL_POINTER;
        CKPT_DEBUG_E(("Failed to allocated memory\n"));
        return rc;
    }
    clNameCopy(&pXlationEntry->name, pName);
    pXlationEntry->cksum    = cksum; 
    pXlationEntry->mastHdl  = masterHdl; 

    /*
     * Add the entry.
     */
    rc = clCntNodeAdd(gCkptSvr->masterInfo.nameXlationDBHdl,
            (ClCntKeyHandleT )pXlationEntry,
            (ClCntDataHandleT)pXlationEntry, NULL);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Failed to add the entry rc[0x %x]\n", rc), rc);
    return rc;
exitOnError:
    clHeapFree(pXlationEntry);
    return rc;
}
                         
