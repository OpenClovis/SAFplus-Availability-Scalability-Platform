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
 * File        : clCkptPeer.c
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
*
*   This file contains implementation of communication between checkpoint servers
*
*
*****************************************************************************/

#include <string.h> 
#include <netinet/in.h> 
#include <clCommon.h>
#include <clDebugApi.h>
#include <clLogApi.h>
#include <clVersion.h>
#include <ipi/clRmdIpi.h>
#include <clIocErrors.h>
#include <clCkptPeer.h>
#include <clCkptUtils.h>
#include <clCkptCommon.h>
#include "clCkptSvrIpi.h"
#include <clCkptMaster.h>
#include <clCkptLog.h>
#include <clCpmExtApi.h>
#include <ckptEockptServerPeerPeerClient.h>
#include <ckptEockptServerCliServerFuncClient.h>
#include <ckptEockptServerExtCliServerFuncClient.h>
#include <ipi/clHandleIpi.h>
#include <clXdrApi.h>
#include <clCkptMasterUtils.h>
#include <ckptEoServer.h>
#include "xdrCkptPeerT.h"
#include "xdrCkptDpT.h"
#include "xdrCkptMasterDBInfoIDLT.h"
#include "xdrCkptMasterDBEntryIDLT.h"
#include "xdrCkptMasterDBClientInfoT.h"

#define CL_CKPT_MODIFY_SECCNT 1
extern CkptSvrCbT  *gCkptSvr;

static ClRcT _ckptPresenceNodePack(ClCntKeyHandleT   key,
                                   ClCntDataHandleT  hdl,
                                   void              *pMsgHdl,
                                   ClUint32T         dataLength);


static ClRcT  _ckptDataPlaneInfoPack(ClBufferHandleT  *pMsgHdl,
                                     CkptT                   *pCkpt) ;

static ClRcT  _ckptControlPlaneInfoPack(ClBufferHandleT   *pMsgHdl,
                                        CkptT                    *pCkpt) ;

static ClTimerHandleT gClPeerReplicateTimer;

typedef struct ClCkptPackArgs
{
    ClUint32T index;
    ClPtrT pOutData;
    ClUint32T maxEntries;
    ClUint32T size;
}ClCkptPackArgsT;

/*
  This routine packs the complete checkpoint DB 
*/
ClRcT   ckptDbPack( ClBufferHandleT   *pOutMsg, 
                    ClIocNodeAddressT        peerAddr)
{
    ClRcT                       rc = CL_OK;  
    CkptT                       *pCkpt  = NULL;
    ClCntNodeHandleT            nodeHdl = 0;
    ClBufferHandleT      msgHdl  = 0;   
    ClCntNodeHandleT            peerHdl = 0;
    ClCkptHdlT         *pCkptHdl      = NULL;

    if (pOutMsg == NULL) return CL_CKPT_ERR_NULL_POINTER;
    msgHdl = *pOutMsg;
    clCntFirstNodeGet (gCkptSvr->ckptHdlList, &nodeHdl);
    while (nodeHdl != 0)
    {
        rc = clCntNodeUserDataGet(gCkptSvr->ckptHdlList, nodeHdl, 
                                 (ClCntDataHandleT *)&pCkptHdl);
        CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                ("Could not get user data rc[0x %x]\n",rc), rc);
        clCntNextNodeGet(gCkptSvr->ckptHdlList, nodeHdl, &nodeHdl);
        rc = clHandleCheckout(gCkptSvr->ckptHdl,*pCkptHdl,(void **)&pCkpt);
        CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                ("Handle check out Error rc[0x %x]\n",rc), rc);
        if( pCkpt != NULL && (pCkpt->pCpInfo != NULL)) 
        {
             (void)clCntNodeFind( pCkpt->pCpInfo->presenceList,
                            (ClCntKeyHandleT)(ClWordT)peerAddr,
                             &peerHdl);
             if( clCpmIsSCCapable() && (peerAddr != gCkptSvr->localAddr) &&
                 (peerAddr != 0))
            {
               rc = ckptCheckpointEntryPack(pCkpt,*pCkptHdl, &msgHdl, peerAddr);
               clHandleCheckin(gCkptSvr->ckptHdl,*pCkptHdl);
               CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                   ("Cant update peer rc[0x %x]\n",rc), rc);
            }
            else
            {
               if(peerAddr == 0)
               {
                   rc = ckptCheckpointEntryPack(pCkpt,*pCkptHdl, &msgHdl, peerAddr);
                   clHandleCheckin(gCkptSvr->ckptHdl,*pCkptHdl);
                   CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                   ("Cant update peer rc[0x %x]\n",rc), rc);
               }
               else
               {
                   clHandleCheckin(gCkptSvr->ckptHdl,*pCkptHdl);
               }
            }
        }
        else
        {
              clHandleCheckin(gCkptSvr->ckptHdl,*pCkptHdl);
        }
 
    }
exitOnError:
    {
    return rc;
    }
}



/*
 * Function to pack the checkpoint info.
 */
 
ClRcT _ckptCheckpointInfoPack(CkptT       *pCkpt,
                              VDECL_VER(CkptCPInfoT, 5, 0, 0) *pCpInfo,
                              CkptDPInfoT *pDpInfo)
{
    ClRcT   rc = CL_OK;    

    /*
     * Pack the control plane info.
     */
    if(pCkpt->pCpInfo != NULL)
    {
        rc = _ckptCPlaneInfoPack(pCkpt->pCpInfo, pCpInfo);
        CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                 ("Failed to pack Control plane rc[0x %x]\n",rc), rc);
    }

    /*
     * Pack the data plane info.
     */
    if (pCkpt->pDpInfo != NULL) 
    {
        rc = _ckptDPlaneInfoPack(pCkpt, pCkpt->pDpInfo,pDpInfo);
        CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
             ("Pack: CP Info Write failed rc[0x %x]\n",rc), rc);
    }
    return rc;
exitOnError:
    if( pCpInfo != NULL)
    {
        if( pCpInfo->presenceList != NULL )
        {
            clHeapFree( pCpInfo->presenceList);
            pCpInfo->presenceList = NULL;
            clHeapFree( pCpInfo->pAppInfo);
            pCpInfo->pAppInfo = NULL;
        }    
    }
    return rc;
}



/* Function to pack a checkpoint */
ClRcT ckptCheckpointEntryPack( CkptT                    *pCkpt,
                               ClCkptHdlT               ckptHdl,
                               ClBufferHandleT   *pMsgHdl,
                               ClIocNodeAddressT        peerAddr)
{
    ClRcT           rc = CL_OK;
    CkptCheckFlagT  ckptFlag;

    /* Write the  Name*/
    
    ckptFlag = CKPT_CKPT_INFO;
    rc = clXdrMarshallClInt32T(&ckptFlag,*pMsgHdl,0);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Failed: Marshalling rc[0x %x]\n",rc), rc);
    rc = clXdrMarshallClHandleT(&ckptHdl,*pMsgHdl,0);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Failed: Marshalling rc[0x %x]\n",rc), rc);
    rc = clXdrMarshallClNameT(&pCkpt->ckptName, *pMsgHdl,0);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Failed: Marshalling rc[0x %x]\n",rc), rc);
    if (pCkpt->pCpInfo != NULL)
    {
        
        rc = _ckptControlPlaneInfoPack(pMsgHdl, pCkpt);
        CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
             ("Pack: CP Info Write failed rc[0x %x]\n",rc), rc);
    }
    if (pCkpt->pDpInfo != NULL) 
    {
        rc = _ckptDataPlaneInfoPack(pMsgHdl, pCkpt);
        CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
             ("Pack: CP Info Write failed rc[0x %x]\n",rc), rc);
    }
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Failed: Marshalling rc[0x %x]\n",rc), rc);
    
    return rc;
exitOnError:
    {
        return rc;
    }
}

static ClRcT
clCkptAppInfoPack(CkptCPlaneInfoT   *pCpInfo,
                  VDECL_VER(CkptCPInfoT, 5, 0, 0) *pCpOutInfo)
{
    ClRcT            rc        = CL_OK;
    ClCntNodeHandleT node      = CL_HANDLE_INVALID_VALUE;
    ClCntKeyHandleT  key       = CL_HANDLE_INVALID_VALUE;
    ClCkptAppInfoT   *pAppInfo = NULL;
    ClUint32T numApps = 0;

    /*
     * Find the no. of entries in the presence list of the checkpoint.
     */
    rc = clCntSizeGet(pCpInfo->appInfoList, &pCpOutInfo->numApps);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
           ("Failed to get the size of the presencelist rc[0x %x]\n",rc), rc);
    
    /*
     * Allocate memory for storing the elements in the presence list.
     */
    pCpOutInfo->pAppInfo = (ClCkptAppInfoT *)clHeapCalloc(1,
            sizeof(ClCkptAppInfoT) * pCpOutInfo->numApps );
    if( pCpOutInfo->pAppInfo == NULL)
    {
        rc = CL_CKPT_ERR_NO_MEMORY;
        CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                ("Failed to allocate memory rc[0x %x]\n",rc), rc);
    }
    pAppInfo = pCpOutInfo->pAppInfo;
    rc = clCntFirstNodeGet(pCpInfo->appInfoList, &node);
    if( CL_OK != rc )
    {
        clHeapFree(pCpOutInfo->pAppInfo);
        clLogError(CL_CKPT_AREA_PEER, CL_CKPT_CTX_REPL_UPDATE,
                   "Failed to get app info from list rc[0x %x]", rc);
        goto exitOnError;
    }
    numApps = pCpOutInfo->numApps;
    while( node != 0 && numApps)
    {
        numApps--;
        rc = clCntNodeUserKeyGet(pCpInfo->appInfoList, node, 
                                 &key);
        if( CL_OK != rc )
        {
            clHeapFree(pCpOutInfo->pAppInfo);
            clLogError(CL_CKPT_AREA_PEER, CL_CKPT_CTX_REPL_UPDATE,
                    "Failed to get app info from list rc[0x %x]", rc);
            goto exitOnError;
        }
        pCpOutInfo->pAppInfo->nodeAddress = ((ClCkptAppInfoT *) (ClWordT)
                key)->nodeAddress;
        pCpOutInfo->pAppInfo->portId      = ((ClCkptAppInfoT *) (ClWordT)
                key)->portId;
        rc = clCntNextNodeGet(pCpInfo->appInfoList, node, &node);
        if( CL_OK != rc )
        {
            rc = CL_OK;
            break;
        }
        pCpOutInfo->pAppInfo++;
    }
    pCpOutInfo->pAppInfo = pAppInfo;
    return CL_OK;
exitOnError:
    return rc;
}

/*
 * Function to pack the control plane info of a checkpoint.
 */

ClRcT   _ckptCPlaneInfoPack(CkptCPlaneInfoT  *pCpInfo,
                            VDECL_VER(CkptCPInfoT, 5, 0, 0)      *pCpOutInfo)
{
    ClRcT   rc = CL_OK; 
    ClCkptPackArgsT ckptPackArgs = {0};
    pCpOutInfo->updateOption = pCpInfo->updateOption;
    pCpOutInfo->id = pCpInfo->id;
    clLogDebug(CL_CKPT_AREA_PEER, CL_CKPT_CTX_REPL_UPDATE, 
               "Packing the control plane info");
    /*
     * Find the no. of entries in the presence list of the checkpoint.
     */
    rc = clCntSizeGet(pCpInfo->presenceList,&pCpOutInfo->size);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
           ("Failed to get the size of the presencelist rc[0x %x]\n",rc), rc);
    
    /*
     * Allocate memory for storing the elements in the presence list.
     */
    pCpOutInfo->presenceList = (ClUint32T *)clHeapCalloc(1,
            sizeof(ClIocNodeAddressT) * pCpOutInfo->size );
    if( pCpOutInfo->presenceList == NULL)
    {
        rc = CL_CKPT_ERR_NO_MEMORY;
        CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                ("Failed to allocate memory rc[0x %x]\n",rc), rc);
    }
    
    /*
     * Pack the elements of presence list.
     */
    ckptPackArgs.pOutData = (ClPtrT)pCpOutInfo->presenceList;
    ckptPackArgs.size = (ClUint32T)sizeof(ClIocNodeAddressT);
    ckptPackArgs.maxEntries = pCpOutInfo->size;
    ckptPackArgs.index = 0;
    clCntWalk(pCpInfo->presenceList,  _ckptPresenceNodePack,
              (ClPtrT)&ckptPackArgs, (ClUint32T)sizeof(ckptPackArgs));
    /* Pack the application info for this checkpoint */
    rc = clCkptAppInfoPack(pCpInfo, pCpOutInfo);
    if( CL_OK != rc )
    {
        clHeapFree(pCpOutInfo->presenceList);
        pCpOutInfo->presenceList = NULL;
    }
exitOnError:
    return rc;
}

ClRcT ckptPresenceListPack( ClCntKeyHandleT   key,
                            ClCntDataHandleT  hdl,
                            void              *pData,
                            ClUint32T         dataLength)
{
    ClRcT         rc = CL_OK;
    ClBufferHandleT msgHdl = *(ClBufferHandleT *)pData;
    clXdrMarshallClUint32T(&key,msgHdl,0);
    return rc;
}

#if 1/* This routine packs the control plane information */
ClRcT   _ckptControlPlaneInfoPack(ClBufferHandleT      *pMsgHdl,
                                  CkptT                       *pCkpt)
{
    ClRcT               rc = CL_OK;
    VDECL_VER(CkptCPInfoT, 5, 0, 0)         ckptCpInfo = {0};
    CkptCheckFlagT      cpFlag;
    ClUint32T           numPresList = 0;

    /* Write the  update option */
    cpFlag = CKPT_CPLANE_INFO;
    rc = clXdrMarshallClInt32T(&cpFlag ,*pMsgHdl,0);
    memset(&ckptCpInfo,0,sizeof(CkptCPInfoT));
    ckptCpInfo.updateOption  = pCkpt->pCpInfo->updateOption;
    ckptCpInfo.id = pCkpt->pCpInfo->id;
    rc = VDECL_VER(clXdrMarshallCkptCPInfoT, 4, 0, 0)(&ckptCpInfo,*pMsgHdl,0);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Failed: Marshalling rc[0x %x]\n",rc), rc);
    rc = clCntSizeGet(pCkpt->pCpInfo->presenceList,&numPresList);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Failed: Marshalling rc[0x %x]\n",rc), rc);
    rc = clXdrMarshallClUint32T(&numPresList,*pMsgHdl,0);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Failed: Marshalling rc[0x %x]\n",rc), rc);
    clCntWalk(pCkpt->pCpInfo->presenceList, ckptPresenceListPack, pMsgHdl, sizeof(pMsgHdl));
    
exitOnError:
    {
        return rc;
    }
}
#endif


/* 
 * Function to pack the presence node.
 */
ClRcT _ckptPresenceNodePack(ClCntKeyHandleT   key,
                            ClCntDataHandleT  hdl,
                            void              *pData,
                            ClUint32T         dataLength)
{
    ClCkptPackArgsT *pArg = (ClCkptPackArgsT*)pData;
    ClIocNodeAddressT    *pEntry = (ClIocNodeAddressT *)pArg->pOutData;

    CL_ASSERT(pArg->size == (ClUint32T)sizeof(*pEntry));
    if(pArg->index < pArg->maxEntries)
    {
        pEntry = pEntry + pArg->index;
        ++pArg->index;
        *pEntry = (ClIocNodeAddressT)(ClWordT)key;
    }
    return CL_OK;
}



/*
 * Function to pack the data plane info of a checkpoint.
 */
ClRcT   _ckptDPlaneInfoPack(CkptT           *pCkpt,
                            CkptDPlaneInfoT *pDpInfo,
                            CkptDPInfoT     *pDpOutInfo)
{
    ClRcT           rc  = CL_OK;
    
    clLogDebug(CL_CKPT_AREA_PEER, CL_CKPT_CTX_REPL_UPDATE, 
               "Packing the data plane info of checkpoint");
    /*
     * Pack all section related  meta data.
     */
    pDpOutInfo->maxCkptSize  = pDpInfo->maxCkptSize;
    pDpOutInfo->maxScns      = pDpInfo->maxScns;
    pDpOutInfo->maxScnSize   = pDpInfo->maxScnSize;
    pDpOutInfo->maxScnIdSize = pDpInfo->maxScnIdSize;
    pDpOutInfo->numScns      = pDpInfo->numScns;
    pDpOutInfo->genIndex     = pDpInfo->indexGenScns;
    if( pDpOutInfo->maxScns == 1)
    {
       pDpOutInfo->numScns = 1;
    }   

    /*
     * Allocate memory for storing section related data.
     */
    pDpOutInfo->pSection     = (CkptSectionInfoT *)clHeapCalloc(1,
                   sizeof(CkptSectionInfoT) * pDpOutInfo->numScns);
    if(pDpOutInfo->pSection == NULL)               
    {                   
        rc = CL_CKPT_ERR_NO_MEMORY;
        CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Failed to allocate memory rc[0x %x]\n",rc), rc);
    }            

    /*
     * Pack the section data.
     */
    rc = _ckptSectionInfoPack(pCkpt, pDpInfo,pDpOutInfo->pSection, pDpOutInfo->numScns);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Failed to pack the section Info rc[0x %x]\n",rc), rc);
    return rc;        
 exitOnError:
    if(pDpOutInfo->pSection != NULL)
    {
        clHeapFree(pDpOutInfo->pSection);
        pDpOutInfo->pSection = NULL;
    }    
    return rc;
}            

ClRcT _ckptSectionInfoPack(CkptT            *pCkpt,
                           CkptDPlaneInfoT  *pDpInfo,
                           CkptSectionInfoT *pSection,
                           ClUint32T maxScns)
{                           
    ClRcT              rc       = CL_OK;
    CkptSectionT       *pSec    = NULL;
    ClUint32T          count    = 0;
    ClCntNodeHandleT   secNode  = CL_HANDLE_INVALID_VALUE;
    ClCntNodeHandleT   nextNode = CL_HANDLE_INVALID_VALUE;
    ClCkptSectionKeyT  *pKey    = NULL;
    CkptSectionInfoT   *pTemp   = pSection;

    CKPT_NULL_CHECK(pDpInfo);
    CKPT_NULL_CHECK(pSection);

    rc = clCntFirstNodeGet(pDpInfo->secHashTbl, &secNode);
    if( CL_OK != rc )
    {
        clLogInfo(CL_CKPT_AREA_PEER, CL_CKPT_CTX_CKPT_OPEN, 
                "No sections have been created");
        return CL_OK;
    }
    while( secNode != 0 && maxScns)
    {
        ClBoolT  sectionLockTaken = CL_FALSE;
        maxScns--;
        nextNode = CL_HANDLE_INVALID_VALUE;
        rc = clCntNodeUserDataGet(pDpInfo->secHashTbl, secNode, 
                (ClCntDataHandleT *) &pSec);
        if( CL_OK != rc )
        {
            clLogError(CL_CKPT_AREA_PEER, CL_CKPT_CTX_CKPT_OPEN, 
                    "Failed to get data while packing section");
            return rc;
        }
        rc = clCntNodeUserKeyGet(pDpInfo->secHashTbl, secNode, 
                                 (ClCntKeyHandleT *) &pKey);
        if( CL_OK != rc )
        {
            clLogError(CL_CKPT_AREA_PEER, CL_CKPT_CTX_CKPT_OPEN, 
                    "Failed to get data while packing section");
            return rc;
        }
        clLogDebug(CL_CKPT_AREA_PEER, CL_CKPT_CTX_REPL_UPDATE,
                   "Packing section [%.*s] info", pKey->scnId.idLen, 
                   pKey->scnId.id);
        /*
         * Take section level lock to ensure that no threads are working on this 
         * section.
         */
        sectionLockTaken = CL_FALSE;
        rc = clCkptSectionLevelLock(pCkpt, &pKey->scnId, &sectionLockTaken);
        if( CL_OK != rc )
        {
            clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN, 
                "Checkpoint data pack failed while acquiring section level lock"
                "for section [%*.s] rc [#%x]", pKey->scnId.idLen, pKey->scnId.id, rc);
            return rc;
        }
        rc = _ckptSectionDataPack(pSec,
                &pKey->scnId, pSection,
                CKPT_SECTION_CREAT,
                count++);
        clCkptSectionLevelUnlock(pCkpt, &pKey->scnId, sectionLockTaken);       
        if( CL_OK != rc )
        {
            return rc;
        }
        rc = clCntNextNodeGet(pDpInfo->secHashTbl, secNode, &nextNode);
        if( CL_OK != rc )
        {
            break;
        }
        pSection++;
        secNode = nextNode;
    }
    pSection = pTemp;
    return CL_OK;
}


/* 
 * This routine packs the control plane information.
 */
 
ClRcT   _ckptDataPlaneInfoPack(ClBufferHandleT   *pMsgHdl,
                               CkptT                    *pCkpt) 
{
    ClRcT             rc         = CL_OK;
    CkptDpT           ckptDpInfo = {0};
    CkptCheckFlagT    dpFlag     = 0;
    ClCntNodeHandleT  secNode    = CL_HANDLE_INVALID_VALUE;
    ClCntNodeHandleT  nextNode   = CL_HANDLE_INVALID_VALUE;
    CkptSectionT      *pSec      = NULL;
    ClCkptSectionKeyT *pKey      = NULL;
    ClUint8T          defaultSec = 0;

    /*
     * Indicate it is data plane info.
     */
    dpFlag = CKPT_DPLANE_INFO;
    rc = clXdrMarshallClInt32T(&dpFlag,*pMsgHdl,0);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Failed: Marshalling rc[0x %x]\n",rc), rc);

    /*
     * Copy the DP Info.
     */
    ckptDpInfo.maxCkptSize  = pCkpt->pDpInfo->maxCkptSize;
    ckptDpInfo.maxScns      = pCkpt->pDpInfo->maxScns;
    ckptDpInfo.maxScnSize   = pCkpt->pDpInfo->maxScnSize;
    ckptDpInfo.maxScnIdSize = pCkpt->pDpInfo->maxScnIdSize;
    ckptDpInfo.numScns      = pCkpt->pDpInfo->numScns;

    rc = VDECL_VER(clXdrMarshallCkptDpT, 4, 0, 0)(&ckptDpInfo,*pMsgHdl,0);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Failed: Marshalling rc[0x %x]\n",rc), rc);

    /*
     * Copy the section data.
     */
    rc = clCntFirstNodeGet(pCkpt->pDpInfo->secHashTbl, &secNode);
    if( CL_OK != rc )
    {
        return rc;
    }
    while( 0 != secNode )
    {
        nextNode = CL_HANDLE_INVALID_VALUE;
        rc = clCntNodeUserDataGet(pCkpt->pDpInfo->secHashTbl, secNode, 
                (ClCntDataHandleT *) &pSec);
        if( CL_OK != rc )
        {
            clLogError(CL_CKPT_AREA_PEER, CL_CKPT_CTX_CKPT_OPEN, 
                    "Failed to get data while packing section");
            return rc;
        }
        rc = clCntNodeUserKeyGet(pCkpt->pDpInfo->secHashTbl, secNode, 
                (ClCntKeyHandleT *) &pKey);
        if( CL_OK != rc )
        {
            clLogError(CL_CKPT_AREA_PEER, CL_CKPT_CTX_CKPT_OPEN, 
                    "Failed to get data while packing section");
            return rc;
        }
        /*
         * Indicate it is section info.
         */
        dpFlag = CKPT_SECTION_INFO;
        rc = clXdrMarshallClInt32T(&dpFlag,*pMsgHdl,0);

        /*
         * Copy the section Id.
         */
        if( pCkpt->pDpInfo->maxScns > 1 )
        {
            rc = clXdrMarshallClUint8T(&defaultSec,*pMsgHdl,0);
            rc = clXdrMarshallClUint16T( &pKey->scnId.idLen ,
                    *pMsgHdl,0);
            CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                    ("Failed: Marshalling rc[0x %x]\n",rc), rc);
            rc = clXdrMarshallArrayClCharT( pKey->scnId.id,
                    pKey->scnId.idLen,
                    *pMsgHdl,0);  
            CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                    ("Failed: Marshalling rc[0x %x]\n",rc), rc);
        }
        else
        {
            defaultSec = 1;
            rc = clXdrMarshallClUint8T(&defaultSec,*pMsgHdl,0);
        }

        /*
         * Copy the section size.
         */
        rc = clXdrMarshallClUint64T(&pSec->size,*pMsgHdl,0);
        CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                ("Failed: Marshalling rc[0x %x]\n",rc), rc);

        /*
         * Copy the section data.
         */
        rc = clXdrMarshallArrayClUint8T( (ClUint8T *)pSec->pData,
                pSec->size,
                *pMsgHdl,0);     
        CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                ("Failed: Marshalling rc[0x %x]\n",rc), rc);
        rc = clCntNextNodeGet(pCkpt->pDpInfo->secHashTbl, secNode,
                &nextNode);
        if( CL_OK != rc )
        {
            break;
        }
        secNode = nextNode;
    }
exitOnError:
    return rc;
}


ClRcT  _ckptInitialConsume(ClBufferHandleT   inMsg,ClUint16T flagVersion)
{
#if 0
    ClUint32T       msgLen    = 0;
    CkptCheckFlagT   checkFlag ;
    CkptT           *pCkpt    = NULL;
    CkptSectionT    *pSec     = NULL;
    ClUint32T        remLen   = 0;
    ClRcT            rc       = CL_OK;
    ClCkptHdlT      *pCkptHdl = NULL;
    ClUint32T        cksum    = 0;
    ClBufferHandleT msg = 0;
    

    CkptPeerInfoT      *pPeerInfo      = NULL;
    rc = clBufferLengthGet(inMsg, &msgLen);
    remLen = msgLen;
    if(flagVersion == 1)
       remLen = remLen - (sizeof(ClVersionT) * sizeof(ClUint32T));

    while(remLen > 0)
    {
         rc = clXdrUnmarshallClInt32T(inMsg,&checkFlag);
         CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                        ("Fail: During Unamrshall  rc[0x %x]\n", rc), rc);
         remLen = remLen - sizeof(ClInt32T);
         switch(checkFlag)
         {
             case CKPT_RESTART_INFO:
             {
                 ClIocNodeAddressT  activeAddr  = 0;
                 ClIocNodeAddressT  localAddr   = 0;
                 ClHandleT          ckptActHdl  = -1;
                 
                 clXdrUnmarshallClHandleT(inMsg,&ckptActHdl);
                 remLen = remLen - sizeof(ClInt32T);
                 clXdrUnmarshallClUint32T(inMsg,&activeAddr);
                 remLen = remLen - sizeof(ClUint32T);
                 if(activeAddr != gCkptSvr->localAddr)
                 {
                      rc = clBufferCreate(&msg);
                      CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                          ("Fail: BufferMessage create failed  rc[0x %x]\n", rc), rc);
                      rc = clXdrMarshallClHandleT(&ckptActHdl,msg,0); 
                      CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                          ("Fail: During Marshall  rc[0x %x]\n", rc), rc);
                      localAddr  = gCkptSvr->localAddr;
                      rc = clXdrMarshallClUint32T(&localAddr,msg,0); 
                      CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                          ("Fail: During Marshall  rc[0x %x]\n", rc), rc);
                      rc = clHandleCreateSpecifiedHandle(gCkptSvr->ckptHdl,sizeof(CkptT),ckptActHdl);      
                      if( CL_GET_ERROR_CODE(rc) == CL_ERR_ALREADY_EXIST) 
                         rc = CL_OK;
                      CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                          ("Fail: During Marshall  rc[0x %x]\n", rc), rc);
                      rc = clHandleCheckout(gCkptSvr->ckptHdl, ckptActHdl,(void **)&pCkpt);    
                      CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                          ("Fail: During Marshall  rc[0x %x]\n", rc), rc);
                      pCkpt->isPending = CL_TRUE;    
                      rc = clHandleCheckin(gCkptSvr->ckptHdl,ckptActHdl);
                      CKPT_SVR_SVR_RMD_CALLBACK( CKPT_GET_CKPT_INFO, msg,activeAddr,
                                                 rc,ckptSyncCkptInfoGetCallback,NULL);
                      clBufferDelete(&msg);                           
                 }
                 else
                 {
                      CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                          ("Fail: Invalid active Address \n"));
                      CL_ASSERT(activeAddr != gCkptSvr->localAddr);
                 }         
                 break;
             }
             case CKPT_PEER_INFO:
             {
                ClIocNodeAddressT  peerAddr = 0;

                rc = clXdrUnmarshallClUint32T(inMsg,&peerAddr);
                CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                ("Unpack:Unmarsahlling[peerinfo] failed rc[0x %x]\n", rc), rc);
                remLen = remLen - sizeof(ClUint32T);
                rc = clCntNodeAdd(gCkptSvr->peerList, 
                               (ClCntKeyHandleT)(ClWordT)peerAddr, 
                                0, NULL);
                if (CL_GET_ERROR_CODE(rc) == CL_ERR_DUPLICATE)   rc = CL_OK;
                CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                           ("Unpack: Node Add failed rc[0x %x]\n", rc), rc);

                if(clIocLocalAddressGet() == gCkptSvr->masterInfo.deputyAddr)
                {
                   if(peerAddr == gCkptSvr->masterInfo.masterAddr)
                   {
                       pPeerInfo = (CkptPeerInfoT*) clHeapCalloc(1,sizeof(CkptPeerInfoT));
                       pPeerInfo->addr       = peerAddr;
                       pPeerInfo->credential = CL_CKPT_CREDENTIAL_POSITIVE;
                       pPeerInfo->available  = CL_CKPT_NODE_AVAIL;
                       pPeerInfo->replicaCount = 0;

                       rc = clCntNodeAdd(gCkptSvr->masterInfo.peerList, (ClPtrT)(ClWordT)peerAddr,
                                         (ClCntDataHandleT)pPeerInfo, NULL);
                       if(CL_GET_ERROR_CODE(rc) == CL_ERR_DUPLICATE)
                       {
                       rc = CL_OK;
                       clHeapFree(pPeerInfo);
                       }
                                         
                   }
                }
                break;
             }
             case CKPT_CKPT_INFO:
             {
                  ClCkptHdlT  ckptTempHdl;
                  ClNameT     ckptName;

                  rc = clXdrUnmarshallClHandleT(inMsg,&ckptTempHdl); 
                  remLen = remLen - (sizeof(ClInt32T) + sizeof(ClHandleT) );
                  CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                     ("Fail: During Unamrshall  rc[0x %x]\n", rc), rc);
                  rc = clXdrUnmarshallClNameT(inMsg,&ckptName);
                  CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                     ("Fail: During Unamrshall  rc[0x %x]\n", rc), rc);
                  remLen = remLen - (CL_MAX_NAME_LENGTH * sizeof(ClUint8T)
                                      + (1 * sizeof(ClUint32T)));
                  clCksm32bitCompute ((ClUint8T *)ckptName.value, ckptName.length, &cksum);
                  rc = clHandleCreateSpecifiedHandle(gCkptSvr->ckptHdl,sizeof(CkptT),ckptTempHdl);
                  if( CL_GET_ERROR_CODE(rc) == CL_ERR_ALREADY_EXIST) 
                      rc = CL_OK;
                  CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                          ("Unpack: ckptinfo get failed rc[0x %x]\n", rc), rc);
                  *pCkptHdl = ckptTempHdl;
                  rc = clCntNodeAdd(gCkptSvr->ckptHdlList,(ClPtrT)(ClWordT)cksum,
                                    pCkptHdl,NULL);
                  if( CL_GET_ERROR_CODE(rc) == CL_ERR_DUPLICATE) 
                      rc = CL_OK;
                  CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                          ("Unpack: ckptHdl is added to list rc[0x %x]\n", rc), rc);
                  rc = clHandleCheckout(gCkptSvr->ckptHdl,*pCkptHdl,(void **)&pCkpt);
                  CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                          ("Unpack: ckptInfo checkout failed rc[0x %x]\n", rc), rc);
                  clNameCopy(&pCkpt->ckptName, &ckptName);
                  rc = clHandleCheckin(gCkptSvr->ckptHdl,*pCkptHdl);
                  break;
             }    
             case  CKPT_CPLANE_INFO:
             {
                 VDECL_VER(CkptCPInfoT, 5, 0, 0)  ckptCpInfo = {0};
                  ClUint32T    presListCount = 0;
                  ClUint32T    count = 0;
                  rc = clHandleCheckout(gCkptSvr->ckptHdl,*pCkptHdl,(void **)&pCkpt);
                  CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                          ("Unpack: ckptInfo checkout failed rc[0x %x]\n", rc), rc);
                  if (pCkpt != NULL) 
                  {
                      rc = _ckptCplaneInfoAlloc(&pCkpt->pCpInfo);
                      CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                     ("Unpack: Cplane Info create failed rc[0x %x]\n",rc), rc);
                      rc = clXdrUnmarshallCkptCPInfoT(inMsg,&ckptCpInfo);
                      CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                     ("Fail: During Unamrshall  rc[0x %x]\n", rc), rc);
                      remLen = remLen - sizeof(CkptCPInfoT);  
                      CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                     ("Unpack: Cplane Info get failed rc[0x %x]\n",rc), rc);
                     pCkpt->pCpInfo->updateOption = ckptCpInfo.updateOption;
                     pCkpt->pCpInfo->id = ckptCpInfo.id;
                     rc = _ckptPresenceListUpdate(pCkpt,gCkptSvr->localAddr);
                     CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                     ("Fail:Updating presence list rc[0x %x]\n", rc), rc);
                     rc = clXdrUnmarshallClUint32T(inMsg,&presListCount);
                     CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                     ("Fail: During Unamrshall  rc[0x %x]\n", rc), rc);
                     remLen = remLen - sizeof(ClUint32T);
                     for( count = 0 ; count < presListCount ; count++)
                     {
                        ClIocNodeAddressT   nodeId = 0; 
                        rc = clXdrUnmarshallClUint32T(inMsg,&nodeId);
                        CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                        ("Fail: During Unamrshall  rc[0x %x]\n", rc), rc);
                        remLen = remLen - sizeof(ClUint32T);
                        rc = clCntNodeAdd(pCkpt->pCpInfo->presenceList,
                                (ClPtrT)(ClWordT)nodeId, 0, NULL);
                        if (CL_GET_ERROR_CODE(rc) == CL_ERR_DUPLICATE)  
                            rc = CL_OK;
                        CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                                ("Presence list add failed rc[0x %x]\n", rc),
                                rc);
                     }
                  }
                  rc = clHandleCheckin(gCkptSvr->ckptHdl,*pCkptHdl);
                  break;
             }
             case   CKPT_DPLANE_INFO:
             {
                  CkptDPInfoT  ckptDpInfo;
                  rc = clHandleCheckout(gCkptSvr->ckptHdl,*pCkptHdl,(void **)&pCkpt); 
                  CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                          ("Unpack: ckptInfo checkout failed rc[0x %x]\n", rc), rc);
                  rc = clXdrUnmarshallCkptDPInfoT(inMsg,&ckptDpInfo);
                  CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                     ("Fail: During Unamrshall  rc[0x %x]\n", rc), rc);
                  remLen = remLen - sizeof(CkptDPInfoT);
                  CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                          ("Unpack: DP info get failed  rc[0x %x]\n", rc), rc);
                  rc = _ckptDplaneInfoAlloc ( &pCkpt->pDpInfo,
                                             ckptDpInfo.maxScns);
                  CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                          ("Unpack: Malloc failed  rc[0x %x]\n", rc), rc);

                  pCkpt->pDpInfo->maxScns      = ckptDpInfo.maxScns;
                  pCkpt->pDpInfo->maxScnSize   = ckptDpInfo.maxScnSize;
                  pCkpt->pDpInfo->maxScnIdSize = ckptDpInfo.maxScnIdSize;
                  pCkpt->pDpInfo->numScns      = ckptDpInfo.numScns;
                  pCkpt->pDpInfo->maxCkptSize  = ckptDpInfo.maxCkptSize;
                  rc = clHandleCheckin(gCkptSvr->ckptHdl,*pCkptHdl);
                  break;
             }
             case CKPT_SECTION_INFO:
             {
                 ClUint8T defaultSec = 0;
                 rc = clXdrUnmarshallClUint8T(inMsg,&defaultSec);
                 CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                     ("Fail: During Unamrshall  rc[0x %x]\n", rc), rc);
                 remLen -= sizeof(ClUint32T);
                 if(defaultSec)
                 {
                  pSec->scnId.id = NULL;
                  pSec->scnId.idLen = 0;
                  pSec->exprTime  = 0;
                 }
                 else
                 {
                  rc = clXdrUnmarshallClUint16T(inMsg,&pSec->scnId.idLen);
                  CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                     ("Fail: During Unamrshall  rc[0x %x]\n", rc), rc);
                  remLen -= sizeof(ClUint32T);
                  CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                          ("Unpack: SectionInfo failed  rc[0x %x]\n", rc), rc);
                  pSec->scnId.id =(ClUint8T *)clHeapAllocate(
                                                    pSec->scnId.idLen);
                 CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                          ("Unpack: SectionInfo failed  rc[0x %x]\n", rc), rc);
                 memset(pSec->scnId.id,'\0',pSec->scnId.idLen);
                 rc = clXdrUnmarshallArrayClCharT( inMsg,pSec->scnId.id,
                                                   pSec->scnId.idLen);
                 CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                     ("Fail: During Unamrshall  rc[0x %x]\n", rc), rc);
                 remLen -= pSec->scnId.idLen  * sizeof(ClUint8T); 
                 }
                 rc = clXdrUnmarshallClUint64T(inMsg, &pSec->size);
                 CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                     ("Fail: During Unamrshall  rc[0x %x]\n", rc), rc);
                 remLen -= sizeof(ClUint64T); 
                 pSec->pData = (ClPtrT) clHeapAllocate(pSec->size);
                 memset(pSec->pData, '\0', pSec->size);
                 rc = clXdrUnmarshallArrayClUint8T( inMsg,pSec->pData,
                                                    pSec->size);
                 CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                     ("Fail: During Unamrshall  rc[0x %x]\n", rc), rc);
                 remLen -= pSec->size  * sizeof(ClUint8T); 
                 pSec->used = CL_TRUE;
                 pSec++;
                 break;
             }/*case End*/
             default :
                        break;
         }/*Switch end*/
    }/*While end*/
      return CL_OK;
exitOnError:
        clHandleCheckin(gCkptSvr->ckptHdl,*pCkptHdl);
exitOnErrorBeforeHdlCheckout:
#endif
        return CL_OK;
}/*fun end*/

/* 
 *   This routine announces the arrival of a checkpoint server 
 *   to the Master. Called with masterDB lock held.
 */

ClRcT   _ckptSvrArrvlAnnounce()
{
    ClRcT           rc           = CL_OK;
    ClUint8T        credential   = CL_CKPT_CREDENTIAL_POSITIVE;
    ClVersionT      ckptVersion  = {0};        
    CkptPeerInfoT   *pPeerInfo   = NULL;
    ClTimerTimeOutT timeOut      = {0};
    ClCkptReplicateTimerArgsT *pTimerArgs = NULL;

    if( gCkptSvr->localAddr == gCkptSvr->masterInfo.masterAddr)
    {
        /*
         * Allocate and set values to CkptPeerInfoT members.
         * addr - ioc address of the node.
         * credential - whether the node can store replicas or not.
         * available  - whether ckpt server is running on the node or not.
         * ckptList   - list of all checkpoint handles/opens made on 
         *              that node.
         */
         if( CL_OK != (rc = clCntDataForKeyGet(gCkptSvr->masterInfo.peerList,
                                (ClPtrT)(ClWordT)gCkptSvr->localAddr,
                                (ClCntDataHandleT *)&pPeerInfo)))
         {                                

             rc = _ckptMasterPeerListInfoCreate(gCkptSvr->localAddr,
                     credential, 0);
             CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                     ("PeerList Info create failed rc[0x %x]\n",rc),
                     rc);
         }   
         else
         {
             /*
              * Check whether any checkpoint needs to be replicated.
              * It is possible that when a checkpoint was created no
              * other node capable for replicating the info was present.
              * The other scenario is that the node holding the replica
              * info has gone down. Now that a node is available,replicate
              * the checkpoint is needed.
              */
             pPeerInfo->available = CL_CKPT_NODE_AVAIL;
             
             pTimerArgs = clHeapCalloc(1, sizeof(*pTimerArgs));

             if(!pTimerArgs)
                 rc = CL_CKPT_RC(CL_ERR_NO_MEMORY);

             CKPT_ERR_CHECK(CL_CKPT_SVR, CL_DEBUG_ERROR,
                            ("Failed to allocate memory for timer\n"),
                            rc);
             pTimerArgs->nodeAddress = gCkptSvr->masterInfo.masterAddr;
             pTimerArgs->pTimerHandle = &gClPeerReplicateTimer;

             memset(&timeOut, 0, sizeof(ClTimerTimeOutT));
             timeOut.tsSec = 2;
             rc = clTimerCreateAndStart(timeOut,
                                        CL_TIMER_ONE_SHOT,
                                        CL_TIMER_SEPARATE_CONTEXT,
                                        _ckptMasterCkptsReplicateTimerExpiry,
                                        (ClPtrT)pTimerArgs,
                                        &gClPeerReplicateTimer);

             /*
              *  In case of master restart, all Collocated checkpoints has to
              *  be reset to prevActive Address 
              */
             if(pPeerInfo->mastHdlList != 0)
             {
                 clCntWalk(pPeerInfo->mastHdlList, _ckptMastHdlListWalk, 
                         (ClPtrT)(ClWordT)gCkptSvr->localAddr, 
                          sizeof(gCkptSvr->localAddr));  
             }            
         }
         gCkptSvr->isAnnounced = CL_TRUE;    
    }        
    if( gCkptSvr->localAddr == gCkptSvr->masterInfo.deputyAddr )
    {
        /*
         * Deputy address is not there on peerList, announcing node is deputy
         * address, then add the node info to the list
         */
         if( CL_OK != (rc = clCntDataForKeyGet(gCkptSvr->masterInfo.peerList,
                                (ClPtrT)(ClWordT)gCkptSvr->localAddr,
                                (ClCntDataHandleT *) &pPeerInfo)))
         {                                

             rc = _ckptMasterPeerListInfoCreate(gCkptSvr->localAddr,
                     credential, 0);
             CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                     ("PeerList Info create failed rc[0x %x]\n",rc),
                     rc);
         }   
         if( pPeerInfo != NULL ) pPeerInfo->available = CL_CKPT_NODE_AVAIL;
    }
    
    /*
     * Non-master checkpoint server. Contact the master (if exisitng)
     */
    if( (gCkptSvr->localAddr != gCkptSvr->masterInfo.masterAddr) &&
        (gCkptSvr->masterInfo.masterAddr != CL_CKPT_UNINIT_VALUE) &&
        (gCkptSvr->masterInfo.masterAddr != -1) )
    {    
	clLogDebug(CL_CKPT_AREA_PEER, CL_CKPT_CTX_PEER_ANNOUNCE,
	           "Ckpt server [%d] arrival announce to master [%d]",
	           gCkptSvr->localAddr, gCkptSvr->masterInfo.masterAddr);
        /*
         * Set the supported version.
         */
        memcpy( &ckptVersion,
            gCkptSvr->versionDatabase.versionsSupported,
            sizeof(ClVersionT));

         /*
          * Update idl handle with deputy's address.
          */
         rc = ckptIdlHandleUpdate( gCkptSvr->masterInfo.masterAddr,
            gCkptSvr->ckptIdlHdl,2);
        CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            (" Idl Handle Update failed rc[0x %x]\n",rc),
            rc);
            
        /* 
         * Send the hello to master server.
         */        
        rc = VDECL_VER(clCkptRemSvrWelcomeClientAsync, 4, 0, 0)(gCkptSvr->ckptIdlHdl,
                &ckptVersion,
                gCkptSvr->localAddr,
                credential,
                NULL,NULL);
        CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Failed to broadcast message rc[0x %x]\n", rc), rc);
        gCkptSvr->isAnnounced = CL_TRUE;    
    }       

exitOnError:
    {
        return rc;
    }
}

ClRcT ckptSvrArrvlAnnounce(void)
{
    ClRcT rc;
    CKPT_LOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
    rc = _ckptSvrArrvlAnnounce();
    CKPT_UNLOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
    return rc;
}

/*This should be in deputy file */
ClRcT    VDECL_VER(clCkptDeputyCkptInfoUpdate, 4, 0, 0)(ClVersionT *pVersion,
                                                        ClUint32T   numOfCkpts,
                                                        CkptInfoT  *pCkptInfo)
{
    ClRcT      rc    = CL_OK;
    ClUint32T  count = 0;
    VDECL_VER(CkptInfoT, 5, 0, 0) ckptInfo = {0};
    VDECL_VER(CkptCPInfoT, 5, 0, 0) cpInfo = {0};
    CkptDPInfoT dpInfo = {0};

    CL_CKPT_SVR_EXISTENCE_CHECK;
    
    rc = clVersionVerify(&gCkptSvr->versionDatabase,pVersion);
    if(rc != CL_OK)
    {
        /*TODO nack send has to be uncommented*/
        //       rc = clCkptNackSend(*pVersion,CKPT_PEER_HELLO);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                       ("Deputy info update version verify returned rc[0x %x]\n", rc));
        return rc;
    }
    ckptInfo.pCpInfo = &cpInfo;
    ckptInfo.pDpInfo = &dpInfo;
    CKPT_LOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
    if(gCkptSvr->masterInfo.deputyAddr == gCkptSvr->localAddr && pCkptInfo)
    {
        for(count = 0; count < numOfCkpts; count++)
        {
            ckptInfoVersionConvertForward(&ckptInfo, pCkptInfo);
            rc = _ckptReplicaInfoUpdate(pCkptInfo->ckptHdl, &ckptInfo);
            CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR, 
                           ("Failed to unpack DB rc[0x %x]\n", rc), rc);
            pCkptInfo++;
        }
    }
    else
    {
        /*TODO : throw ckptError,so that sender has to handle resend to correct guy */
    }
    exitOnError:
    CKPT_UNLOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
    return rc;
}                                    

ClRcT    VDECL_VER(clCkptDeputyCkptInfoUpdate, 5, 0, 0)(ClVersionT *pVersion,
                                                        ClUint32T   numOfCkpts,
                                                        VDECL_VER(CkptInfoT, 5, 0, 0)  *pCkptInfo)
{
    ClRcT      rc    = CL_OK;
    ClUint32T  count = 0;

    CL_CKPT_SVR_EXISTENCE_CHECK;
    
    rc = clVersionVerify(&gCkptSvr->versionDatabase,pVersion);
    if(rc != CL_OK)
    {
        /*TODO nack send has to be uncommented*/
        //       rc = clCkptNackSend(*pVersion,CKPT_PEER_HELLO);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                       ("Deputy info update version verify returned rc[0x %x]\n", rc));
        return rc;
    }

    CKPT_LOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
    if(gCkptSvr->masterInfo.deputyAddr == gCkptSvr->localAddr && pCkptInfo)
    {
        for(count = 0; count < numOfCkpts; count++)
        {
            rc = _ckptReplicaInfoUpdate(pCkptInfo->ckptHdl, pCkptInfo);
            CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR, 
                           ("Failed to unpack DB rc[0x %x]\n", rc), rc);
            pCkptInfo++;
        }
    }
    else
    {
        /*TODO : throw ckptError,so that sender has to handle resend to correct guy */
    }
    exitOnError:
    CKPT_UNLOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
    return rc;
}                                    


/* 
 * This routine announces the departure of a checkpoint server  to the rest of * the checkpoint servers (if any).
 */
 
ClRcT   ckptSvrDepartureAnnounce()
{
    ClRcT           rc          = CL_OK;
    ClVersionT      ckptVersion = {0};

    /*
     * Check whether the server is fully up or not.
     */
    CL_CKPT_SVR_EXISTENCE_CHECK;
    
    /*
     * If the node going down is master, then find the new AR and inform 
     * the clients about the same.
     */
    if(gCkptSvr->localAddr == gCkptSvr->masterInfo.masterAddr)
    {
        ckptPeerDown(gCkptSvr->localAddr, CL_CKPT_SVR_DOWN, 
                     CL_CKPT_UNINIT_VALUE);
    }    
    
    /*
     * Copy the supported version.
     */
    memcpy(&ckptVersion,gCkptSvr->versionDatabase.versionsSupported,
           sizeof(ClVersionT));
    
    /*
     * Broadcase the departure to all the nodes in the cluster.
     */
    rc = ckptIdlHandleUpdate(CL_IOC_BROADCAST_ADDRESS,
                             gCkptSvr->ckptIdlHdl,0);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Failed to update the idl handle rc[0x %x]\n",rc), rc);
    rc = VDECL_VER(clCkptRemSvrByeClientAsync, 4, 0, 0)(gCkptSvr->ckptIdlHdl,
                                    ckptVersion,
                                    gCkptSvr->localAddr,
                                    CL_IOC_CKPT_PORT, 
                                    CL_CKPT_SVR_DOWN,
                                    NULL,0);
exitOnError:
    {
        return rc;
    }
}

ClRcT
clCkptSectionInfoUpdate(ClCkptHdlT        ckptHdl, 
                        CkptT             *pCkpt,
                        ClCkptSectionIdT  *pSecId, 
                        ClTimeT           exprTime,
                        ClUint32T         size,
                        ClUint8T          *pData)
{
    ClRcT         rc    = CL_OK;
    CkptSectionT  *pSec = NULL;

    /* Trying to find the section, is already there */
    rc = clCkptSectionInfoGet(pCkpt, pSecId, &pSec);
    if( CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST )
    {
        /* Section does not exist, Create the sections here and return*/
        rc = clCkptSectionChkNAdd(ckptHdl, pCkpt, pSecId, 
                                  exprTime, size, 
                                  pData); 
        if( CL_OK != rc )
        {
            return rc;
        }
        /* Done */
        return rc;
    }
    if( CL_OK != rc )
    {
        /* Some other problem, just return here */
        clLogError(CL_CKPT_AREA_PEER, CL_CKPT_CTX_REPL_UPDATE, 
                   "Finding section [%.*s] failed with rc[0x %x]", 
                    pSecId->idLen, pSecId->id, rc);
        return rc;
    }
    /* Found it, Overwriting the info with the latest information */
    clHeapFree(pSec->pData);
    pSec->pData = (ClAddrT)clHeapCalloc(size, sizeof(ClCharT));
    if( NULL == pSec->pData ) 
    {
        clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN, 
                "Memory allocation failed during section overwrite");
        return CL_CKPT_ERR_NO_MEMORY;
    }
    memcpy(pSec->pData, pData, size);
    pSec->size  = size;

    return rc;
}


ClRcT  VDECL_VER(clCkptRemSvrCkptInfoSync, 5, 0, 0)(ClVersionT  *pVersion, 
                                                    ClHandleT   ckptHdl,
                                                    ClNameT     *pName, 
                                                    VDECL_VER(CkptCPInfoT, 5, 0, 0) *pCpInfo,
                                                    CkptDPInfoT *pDpInfo)
{
    ClRcT       rc      = CL_OK;
    CkptT       *pCkpt  = NULL;
    ClUint32T   count   = 0;
    ClCharT     *pTemp  = NULL;
    ClUint32T   *pData  = NULL;

    /*
     * Check whether the server is fully up or not.
     */
    CL_CKPT_SVR_EXISTENCE_CHECK;

    /*
     * Verify the version.
     */
    rc = clVersionVerify(&gCkptSvr->versionDatabase,pVersion);
    if( CL_GET_ERROR_CODE(rc) == CL_ERR_VERSION_MISMATCH)
    {
        return CL_OK;
    }
    
    /*
     * Retrieve the info associated with the active handle.
     */
    rc = ckptSvrHdlCheckout(gCkptSvr->ckptHdl,ckptHdl,(void **)&pCkpt);      
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("Handle checkout in checkpoint create failed rc[0x %x]\n",
                 rc));
        return rc;
    }

    /*
     * Lock the checkpoint's mutex.
     */
    CKPT_LOCK(pCkpt->ckptMutex);
    /*
     * Check if we raced with a delete.
     */
    if(!pCkpt->ckptMutex)
    {
        clHandleCheckin(gCkptSvr->ckptHdl, ckptHdl);
        clLogWarning("INFO", "SYNC", "Ckpt [%.*s] already deleted for handle [%#llx]", 
                     pName->length, pName->value, ckptHdl);
        return CL_OK;
    }
    /*
     * Copy the checkpoint name.
     */
    clNameCopy(&pCkpt->ckptName, pName);
    
    /* 
     * Copy the control plane information.
     */
    pCkpt->pCpInfo->updateOption = pCpInfo->updateOption;
    pCkpt->pCpInfo->id = pCpInfo->id;
    /*
     * Add to the presence list.
     */
    pTemp = (ClCharT *)pCpInfo->presenceList;
    for( count = 0; count < pCpInfo->size ; count++)
    {
        rc = _ckptPresenceListUpdate(pCkpt,*(pCpInfo->presenceList));
        CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                ("Failed to update the presenceList rc[0x %x]\n", rc), rc);
        (pCpInfo->presenceList)++;
    }
    pCpInfo->presenceList = (ClUint32T *)pTemp;

    /*
     * Add application information also 
     */
    pTemp = (ClCharT *) pCpInfo->pAppInfo;
    for( count = 0; count < pCpInfo->numApps ; count++)
    {
        rc = clCntDataForKeyGet(pCkpt->pCpInfo->appInfoList, (ClCntKeyHandleT)
                (ClWordT) pCpInfo->pAppInfo, (ClCntDataHandleT *) &pData);
            if(CL_OK != rc )
            {
                clCkptActiveAppInfoUpdate(pCkpt, pCpInfo->pAppInfo->nodeAddress, 
                        pCpInfo->pAppInfo->portId, &pData);
            }
        pCpInfo->pAppInfo++;
    }
    pCpInfo->pAppInfo = (ClCkptAppInfoT *) pTemp;
    
    /*
     * Copy the dataplane information.
     */
    pCkpt->pDpInfo->maxCkptSize  = pDpInfo->maxCkptSize;
    pCkpt->pDpInfo->maxScns      = pDpInfo->maxScns;
    pCkpt->pDpInfo->numScns      = pDpInfo->numScns;
    pCkpt->pDpInfo->maxScnSize   = pDpInfo->maxScnSize;
    pCkpt->pDpInfo->maxScnIdSize = pDpInfo->maxScnIdSize;

    /*
     * Copy the sections and start the expiry timer if running.
     */
    pTemp = (ClCharT *)pDpInfo->pSection;
    for(count = 0; count < pDpInfo->numScns; count++)
    {
            rc = clCkptSectionInfoUpdate(ckptHdl, pCkpt,
                    &pDpInfo->pSection->secId, 
                    pDpInfo->pSection->expiryTime, pDpInfo->pSection->size, 
                    pDpInfo->pSection->pData); 

            /* Its ok if the section already exists */
            if (CL_CKPT_ERR_ALREADY_EXIST == rc) rc = CL_OK;
            else
              {
                if ( CL_OK != rc )
                  {
                    clLogError(CL_CKPT_AREA_PEER, CL_CKPT_CTX_CKPT_OPEN, 
                               "Adding the sections failed rc[0x %x]", rc);
                    goto exitOnError;
                  }
                if(!strncmp((ClCharT *)pDpInfo->pSection->secId.id, 
                            (ClCharT *)"generatedSection",
                            strlen("generatedSection")))
                  {
                    pCkpt->pDpInfo->indexGenScns++;
                  }
                pDpInfo->pSection++;
              }
    }
    pDpInfo->pSection = (CkptSectionInfoT *)pTemp;

    exitOnError:
    /*
     * Unlock the checkpoint's mutex.
     */
    CKPT_UNLOCK(pCkpt->ckptMutex);

    clHandleCheckin(gCkptSvr->ckptHdl, ckptHdl);
    
    return rc;
}                            

/* 
 * Function at the remote server that is called whenever synchronize API
 * is invoked.
 */
ClRcT  VDECL_VER(clCkptRemSvrCkptInfoSync, 4, 0, 0)(ClVersionT  *pVersion, 
                                                    ClHandleT   ckptHdl,
                                                    ClNameT     *pName, 
                                                    CkptCPInfoT *pCpInfo,
                                                    CkptDPInfoT *pDpInfo)
{
    VDECL_VER(CkptInfoT, 5, 0, 0) ckptInfo = {0};
    VDECL_VER(CkptCPInfoT, 5, 0, 0) cpInfo = {0};
    CkptInfoT ckptInfoBase = {0};
    ckptInfo.pCpInfo = &cpInfo;
    ckptInfo.pDpInfo = pDpInfo;
    ckptInfoBase.pCpInfo = pCpInfo;
    ckptInfoBase.pDpInfo = pDpInfo;
    ckptInfoVersionConvertForward(&ckptInfo, &ckptInfoBase);
    return VDECL_VER(clCkptRemSvrCkptInfoSync, 5, 0, 0)(pVersion, ckptHdl, pName, 
                                                        ckptInfo.pCpInfo,
                                                        ckptInfo.pDpInfo);
}                            

/*
 * This is a common pack function for SectionCreate, SectionOverwrite
 * and Section Delete.
 */

ClRcT _ckptSectionDataPack(CkptSectionT      *pSec,
                           ClCkptSectionIdT  *pSecId, 
                           CkptSectionInfoT  *pSection,
                           CkptUpdateFlagT   updateFlag,
                           ClUint32T         count)
{
    ClRcT   rc = CL_OK;

    /*
     * Validate the input parameters.
     */
    CKPT_NULL_CHECK(pSec);
    CKPT_NULL_CHECK(pSection);
    CKPT_NULL_CHECK(pSecId);

    /*
     * Pack the sectionId related info.
     */
    pSection->secId.idLen = pSecId->idLen;   
    
    if( pSecId->id != NULL )
    {
        pSection->secId.id    = (ClUint8T *)clHeapCalloc(1, 
                sizeof(ClUint8T) * pSecId->idLen);
        if(pSection->secId.id == NULL)        
        {                   
            rc = CL_CKPT_ERR_NO_MEMORY;
            CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                    ("Failed to allocate memory rc[0x %x]\n",rc), rc);
        }            
        memcpy( pSection->secId.id,pSecId->id,
                pSecId->idLen);
    }
    else
    {
        /*
         * default section
         */
        pSection->secId.id = NULL;
    }

    /*
     * Pack the section data.
     */
    switch(updateFlag)
    {
        case CKPT_SECTION_CREAT:
             /*
              * Copy the expiration timer.
              */
             pSection->expiryTime = pSec->exprTime;
			/* Break not required since "CREATE" option is superset of "OVERWRITE" */
        case CKPT_SECTION_OVERWRITE:
            /*
             * Copy the data and datasize.
             */
            pSection->size  = pSec->size;
            pSection->pData = (ClUint8T *)clHeapCalloc(1, 
                    sizeof(ClUint8T) * pSec->size);
            if(pSection->pData == NULL)        
            {                   
                rc = CL_CKPT_ERR_NO_MEMORY;
                CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                        ("Failed to allocate memory rc[0x %x]\n",rc), rc);
            }            
            memcpy(pSection->pData, pSec->pData, pSec->size);
            break;
        case CKPT_SECTION_DELETE:
            /*
             * Make data and datasize = 0.
             */
            pSection->pData = NULL;
            pSection->size  = 0;
            break;
        default:
            rc = CL_CKPT_ERR_INVALID_STATE;
            CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                    ("Update flag is not proper rc[0x %x]\n",rc), rc);
    }
    /*
     * copy the section index.
     */
    pSection->slot = count;
    return rc;
exitOnError:
    /*
     * Cleanup code.
     */
    if (pSection->secId.id != NULL) 
    {
        clHeapFree( pSection->secId.id);
        pSection->secId.id = NULL;
    }    
    if (pSection->pData != NULL)
    {
        clHeapFree(pSection->pData);
    }
    return rc;
}

/*
 * This routine is called whenever a node or a component or a checkpoint 
 * server  goes down either gracefully or ungracefully. 
 * This will be called at all the nodes.
 */
 
void ckptPeerDown(ClIocNodeAddressT   peerAddr, ClUint32T flag, 
                  ClIocPortT portId) 
{
    ClCkptAppInfoT  appInfo = {0};
    if(!gCkptSvr) return;

    CKPT_LOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
    CKPT_LOCK(gCkptSvr->ckptActiveSem);
    if(!gCkptSvr->serverUp)
    {
        CKPT_UNLOCK(gCkptSvr->ckptActiveSem);
        CKPT_UNLOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
        return;
    }
    clLogMultiline(CL_LOG_SEV_DEBUG, CL_CKPT_AREA_PEER, CL_CKPT_CTX_PEER_DOWN, 
                   "Recieved peer down notification\n"
                   "PeerAddr :[%d]\n"
                   "flag     :[%d]\n"
                   "portId   :[%d]", peerAddr, flag, portId);
    if(gCkptSvr->masterInfo.masterAddr == gCkptSvr->localAddr || 
       gCkptSvr->masterInfo.deputyAddr == gCkptSvr->localAddr || 
       gCkptSvr->masterInfo.prevMasterAddr == gCkptSvr->localAddr ||
       clCpmIsSCCapable())
    {
        /*
         * Master has to update its peer list and has to select new active
         * replica depending on checkpoint type and the node/component going 
         * down being active replica.
         */
        clCkptMasterPeerUpdateNoLock(portId, flag, peerAddr, 
                                     CL_CKPT_CREDENTIAL_POSITIVE); 
        if( flag != CL_CKPT_COMP_DOWN)
        {
            clCkptMasterReplicaListUpdateNoLock(peerAddr);
        }
    }    
    /*
     * Delete the node from the presence list maintained by replica nodes.
     */
    if( flag != CL_CKPT_COMP_DOWN)
    {
        appInfo.nodeAddress = peerAddr;
        appInfo.portId   = portId; 
        /*
         * Walk through the presence list of checkpoints and pass the address
         * of node/ckptserver going down in the cookie.
         */
        clCntWalk(gCkptSvr->ckptHdlList, ckptPresenceListNodeDelete, 
                  &appInfo, sizeof(appInfo));
    }        
    CKPT_UNLOCK(gCkptSvr->ckptActiveSem);
    CKPT_UNLOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
}



/*
 * Function to make RMDs to all the replicas of a checkpoint
 * to update their presence list.
 */
 
ClRcT _ckptPresenceListPeerUpdate(CkptT             *pCkpt,
                                  ClHandleT         ckptHdl,
                                  ClIocNodeAddressT peerAddr)
{
    ClRcT               rc          = CL_OK;
    ClCntNodeHandleT    nodeHdl     = 0;
    ClCntNodeHandleT    dataHdl     = 0;
    ClIocNodeAddressT   tempAddr    = 0;
    ClVersionT          ckptVersion = {0};

    /*
     * Copy the supported version.
     */
    memcpy(&ckptVersion, gCkptSvr->versionDatabase.versionsSupported,
           sizeof(ClVersionT));

    /* 
     * Walk through the presence list of the checkpoint and update the
     * replicas one by one.
     */
    clCntFirstNodeGet(pCkpt->pCpInfo->presenceList,&nodeHdl);
    while(nodeHdl != 0)
    {
        rc = clCntNodeUserKeyGet( pCkpt->pCpInfo->presenceList,nodeHdl,
                             (ClCntKeyHandleT *)&dataHdl);
        if( CL_OK != rc )
        {
            clLogCritical(CL_CKPT_AREA_PEER, CL_CKPT_CTX_PEER_DOWN, 
                    "Failed to get the data for nodeHdl [%p]", (void *)
                    nodeHdl);
            break;
        }
        tempAddr = (ClIocNodeAddressT)(ClWordT)dataHdl;
        if( tempAddr != gCkptSvr->localAddr && peerAddr != tempAddr)
        {
            rc = ckptIdlHandleUpdate(tempAddr,gCkptSvr->ckptIdlHdl,0);        
            CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                           ("Failed to update the IDL handle rc[0x %x]\n",rc), rc);
            rc = VDECL_VER(clCkptAllReplicaPresenceListUpdateClientAsync, 4, 0, 0)(
                                                               gCkptSvr->ckptIdlHdl,
                                                               ckptVersion,
                                                               ckptHdl,peerAddr,
                                                               NULL, NULL);
            /* If the node is dead we don't need to update its presence list */        
            if (CL_GET_ERROR_CODE(rc) == CL_IOC_ERR_COMP_UNREACHABLE || CL_GET_ERROR_CODE(rc) == CL_IOC_ERR_HOST_UNREACHABLE) rc = CL_OK;
		    if( CL_OK != rc )
            {
		    	clLogWarning(CL_CKPT_AREA_PEER,CL_CKPT_CTX_REPL_UPDATE,
                             "Failed to update address [%d] for CKPT [%.*s], rc [%#x]", tempAddr,pCkpt->ckptName.length,pCkpt->ckptName.value,rc);
            }
        }               
        clCntNextNodeGet(pCkpt->pCpInfo->presenceList,nodeHdl,&nodeHdl);
    } 
    exitOnError:
    return rc;
}



/*
 * Function call pull a checkpoint from the active node.
 */
 
ClRcT VDECL_VER(clCkptRemSvrCkptInfoGet, 4, 0, 0)(ClVersionT         *pVersion,
                              ClHandleT          ckptHdl,
                              ClIocNodeAddressT  peerAddr,
                              CkptInfoT          *pCkptInfo)
{
    ClRcT    rc        = CL_OK;
    CkptT    *pCkpt    = NULL;
    VDECL_VER(CkptInfoT, 5, 0, 0) ckptInfo = {0};

    /*
     * Check whether the server is fully up or not.
     */
    CL_CKPT_SVR_EXISTENCE_CHECK;
    if( NULL == pCkptInfo ) 
    {
        clLogError(CL_CKPT_AREA_PEER, CL_CKPT_CTX_REPL_UPDATE, 
                "The outvariable is NULL, can't load the data");
        return CL_CKPT_ERR_NULL_POINTER;
    }

    /*
     * Verify the version.
     */
    rc = clVersionVerify(&gCkptSvr->versionDatabase,pVersion);
    if( CL_OK != rc)
    {
        clLogDebug(CL_CKPT_AREA_PEER, CL_CKPT_CTX_REPL_UPDATE, 
                   "Ckpt version is not proper");
        /*
         * IDL will not pack the inout variable,if rc is not CL_OK
         */
        return  CL_OK;
    }
    memset(pVersion, '\0', sizeof(ClVersionT));

    pCkptInfo->pCpInfo = NULL;
    pCkptInfo->pDpInfo = NULL;

    /*
     * Retrieve the info associated with the active handle.
     */

    rc = ckptSvrHdlCheckout(gCkptSvr->ckptHdl,  ckptHdl, (void **)&pCkpt);
    CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Fail: Handle checkout [0x %x]\n", rc), rc);

    /*
     * Lock the checkpoint's mutex.
     */
    CKPT_LOCK( pCkpt->ckptMutex);
    /*
     * Check if we raced with a delete
     */
    if(!pCkpt->ckptMutex)
    {
        clHandleCheckin(gCkptSvr->ckptHdl, ckptHdl);
        clLogWarning(CL_CKPT_AREA_PEER, CL_CKPT_CTX_REPL_UPDATE, "Ckpt with handle [%#llx] was deleted", ckptHdl);
        return CL_CKPT_ERR_NOT_EXIST;
    }
    clLogDebug(CL_CKPT_AREA_PEER, CL_CKPT_CTX_REPL_UPDATE, 
               "Packing the checkpoint [%.*s] for replication, ckptHdl [%#llX]",
               pCkpt->ckptName.length, pCkpt->ckptName.value, ckptHdl);
    /*
     * Allocate memory for the control plane info.
     */
    ckptInfo.pCpInfo = clHeapCalloc(1, sizeof(*ckptInfo.pCpInfo));
    CL_ASSERT(ckptInfo.pCpInfo != NULL);
    ckptInfo.pDpInfo = clHeapCalloc(1, sizeof(*ckptInfo.pDpInfo));
    CL_ASSERT(ckptInfo.pDpInfo != NULL);

    /* 
     * Pack the control plane and data plane info.
     */
    rc = _ckptCheckpointInfoPack(pCkpt, ckptInfo.pCpInfo, ckptInfo.pDpInfo);

    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Fail: Handle checkout [0x %x]\n", rc), rc);

    ckptInfo.ckptHdl = ckptHdl;

    /*
     * Copy the checkpoint name.
     */
    ckptInfo.pName   = (ClNameT *)clHeapCalloc(1,sizeof(ClNameT));
    CL_ASSERT(ckptInfo.pName != NULL);

    clNameCopy(ckptInfo.pName, &pCkpt->ckptName);

    /*
     * Unlock the checkpoint's mutex.
     */
    CKPT_UNLOCK(pCkpt->ckptMutex);        

    clHandleCheckin(gCkptSvr->ckptHdl,ckptHdl);

    ckptInfoVersionConvertBackward(pCkptInfo, &ckptInfo);

    if(ckptInfo.pCpInfo) clHeapFree(ckptInfo.pCpInfo);
    if(ckptInfo.pDpInfo) clHeapFree(ckptInfo.pDpInfo);

    return rc;    

exitOnError:

    /*
     * Free all resources which have been allocated till now
     */
    ckptPackInfoFree(&ckptInfo);
    
    /*
     * Unlock the checkpoint's mutex.
     */
    CKPT_UNLOCK(pCkpt->ckptMutex);
    
    clHandleCheckin(gCkptSvr->ckptHdl,ckptHdl);
    
exitOnErrorBeforeHdlCheckout:
    return rc;
}

ClRcT VDECL_VER(clCkptRemSvrCkptInfoGet, 5, 0, 0)(ClVersionT         *pVersion,
                                                  ClHandleT          ckptHdl,
                                                  ClIocNodeAddressT  peerAddr,
                                                  VDECL_VER(CkptInfoT, 5, 0, 0) *pCkptInfo)
{
    ClRcT    rc        = CL_OK;
    CkptT    *pCkpt    = NULL;

    /*
     * Check whether the server is fully up or not.
     */
    CL_CKPT_SVR_EXISTENCE_CHECK;
    if( NULL == pCkptInfo ) 
    {
        clLogError(CL_CKPT_AREA_PEER, CL_CKPT_CTX_REPL_UPDATE, 
                "The outvariable is NULL, can't load the data");
        return CL_CKPT_ERR_NULL_POINTER;
    }

    /*
     * Verify the version.
     */
    rc = clVersionVerify(&gCkptSvr->versionDatabase,pVersion);
    if( CL_OK != rc)
    {
        clLogDebug(CL_CKPT_AREA_PEER, CL_CKPT_CTX_REPL_UPDATE, 
                   "Ckpt version is not proper");
        /*
         * IDL will not pack the inout variable,if rc is not CL_OK
         */
        return  CL_OK;
    }
    memset(pVersion, '\0', sizeof(ClVersionT));

    pCkptInfo->pCpInfo = NULL;
    pCkptInfo->pDpInfo = NULL;

    /*
     * Retrieve the info associated with the active handle.
     */

    rc = ckptSvrHdlCheckout(gCkptSvr->ckptHdl,  ckptHdl, (void **)&pCkpt);
    CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Fail: Handle checkout [0x %x]\n", rc), rc);


    /*
     * Lock the checkpoint's mutex.
     */
    CKPT_LOCK( pCkpt->ckptMutex);
    /*
     * Check if we raced with a delete
     */
    if(!pCkpt->ckptMutex)
    {
        clHandleCheckin(gCkptSvr->ckptHdl, ckptHdl);
        clLogWarning(CL_CKPT_AREA_PEER, CL_CKPT_CTX_REPL_UPDATE, "Ckpt with handle [%#llx] was deleted", ckptHdl);
        return CL_CKPT_ERR_NOT_EXIST;
    }
    
    clLogDebug(CL_CKPT_AREA_PEER, CL_CKPT_CTX_REPL_UPDATE, 
               "Packing the checkpoint [%.*s] for replication, ckptHdl [%#llX]",
               pCkpt->ckptName.length, pCkpt->ckptName.value, ckptHdl);
    /*
     * Allocate memory for the control plane info.
     */
    pCkptInfo->pCpInfo = clHeapCalloc(1, sizeof(*pCkptInfo->pCpInfo));
    CL_ASSERT(pCkptInfo->pCpInfo != NULL);
    pCkptInfo->pDpInfo = clHeapCalloc(1, sizeof(*pCkptInfo->pDpInfo));
    CL_ASSERT(pCkptInfo->pDpInfo != NULL);

    /* 
     * Pack the control plane and data plane info.
     */
    rc = _ckptCheckpointInfoPack(pCkpt, pCkptInfo->pCpInfo, pCkptInfo->pDpInfo);

    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Fail: Handle checkout [0x %x]\n", rc), rc);

    pCkptInfo->ckptHdl = ckptHdl;

    /*
     * Copy the checkpoint name.
     */
    pCkptInfo->pName   = (ClNameT *)clHeapCalloc(1,sizeof(ClNameT));
    CL_ASSERT(pCkptInfo->pName != NULL);

    clNameCopy(pCkptInfo->pName, &pCkpt->ckptName);

    /*
     * Unlock the checkpoint's mutex.
     */
    CKPT_UNLOCK(pCkpt->ckptMutex);        

    clHandleCheckin(gCkptSvr->ckptHdl,ckptHdl);

    return rc;    

exitOnError:

    /*
     * Free all resources which have been allocated till now
     */
    if(pCkptInfo)
        ckptPackInfoFree(pCkptInfo);
    
    /*
     * Unlock the checkpoint's mutex.
     */
    CKPT_UNLOCK(pCkpt->ckptMutex);
    
    clHandleCheckin(gCkptSvr->ckptHdl,ckptHdl);
    
exitOnErrorBeforeHdlCheckout:
    return rc;
}


/*
 * Function at the replica nodes that is called to add a new replica
 * address to the presence list.
 */
 
ClRcT VDECL_VER(clCkptAllReplicaPresenceListUpdate, 4, 0, 0)(ClVersionT        version,
                                         ClHandleT         ckptActHdl,
                                         ClIocNodeAddressT peerAddr)
{
    ClRcT  rc     = CL_OK;
    CkptT  *pCkpt = NULL;

    /*
     * Check whether the server is fully up or not.
     */
    CL_CKPT_SVR_EXISTENCE_CHECK;

    /*
     * Verify the version.
     */
    rc = clVersionVerify(&gCkptSvr->versionDatabase, &version);
    CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Ckpt: VersionMismatch rc[0x %x]\n", rc), rc);
    
    /*
     * Retrieve the info associated with the active handle.
     */
    rc = clHandleCheckout(gCkptSvr->ckptHdl, ckptActHdl, (void **)&pCkpt);    
    CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Fail: Handle checkout failure  rc[0x %x]\n", rc), rc);
            
    /*
     * Lock the checkpoint's mutex.
     */
    CKPT_LOCK(pCkpt->ckptMutex);           
    /*
     * Check if we raced with a delete
     */
    if(!pCkpt->ckptMutex)
    {
        clHandleCheckin(gCkptSvr->ckptHdl, ckptActHdl);
        clLogWarning(CL_CKPT_AREA_PEER, "PRE", "Ckpt with handle [%#llx] got deleted", ckptActHdl);
        return CL_OK;
    }

    /*
     * Update the checkpoint's presence list with the passed address.
     */

    rc = _ckptPresenceListUpdate(pCkpt, peerAddr);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Fail: Presence List Update failed rc[0x %x]\n", rc), rc);
exitOnError:
    clHandleCheckin(gCkptSvr->ckptHdl,ckptActHdl);
    
    /*
     * Unlock the checkpoint's mutex.
     */
    CKPT_UNLOCK(pCkpt->ckptMutex);           
exitOnErrorBeforeHdlCheckout:
    return rc;
}



/*
 * Function called during active replica set switchover to inform the
 * previous active about the new active replica address.
 */
 
ClRcT VDECL_VER(clCkptActiveAddrInform, 4, 0, 0)(ClVersionT         version,
                             ClHandleT          masterHdl,
                             ClIocNodeAddressT  activeAddr)
{
    ClRcT                   rc            = CL_OK;
    CkptMasterDBEntryT      *pStoredData  = NULL;

    /*
     * Check whether the server is fully up or not.
     */
    CL_CKPT_SVR_EXISTENCE_CHECK;

    /*
     * Lock the master DB.
     */
    CKPT_LOCK(gCkptSvr->masterInfo.ckptMasterDBSem);

    /*
     * Retrieve the data associated with the master handle.
     */
    rc = clHandleCheckout(gCkptSvr->masterInfo.masterDBHdl, 
                          masterHdl, (void **)&pStoredData);  
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("MasterActiveReplicaSet failed rc[0x %x]\n",rc),
            rc);
            
    /* 
     * Update the active replica address.
     */
    pStoredData->activeRepAddr = activeAddr;
    
    /* 
     * Checkin the updates stuff.
     */
    rc = clHandleCheckin(gCkptSvr->masterInfo.masterDBHdl, 
            masterHdl);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("MasterActiveReplicaSet failed rc[0x %x]\n",rc),
            rc);
exitOnError:

    /*
     * Unlock the master DB.
     */
    CKPT_UNLOCK(gCkptSvr->masterInfo.ckptMasterDBSem);

    return rc;
}
    


/*
 * Function to send back the nack.
 */
 
ClRcT clCkptNackSend(ClVersionT suppVersion, ClUint32T funId) 
{
    ClRmdOptionsT          rmdOptions = {0};
    ClUint32T              rmdFlags   = CL_RMD_CALL_ASYNC;
    ClIocAddressT          destAddr   = {{0}};
    ClBufferHandleT inMsg      = 0;
    ClRcT                  rc         = CL_OK;

    /*
     * Set the rmd parameters.
     */
    rmdOptions.timeout         = CKPT_RMD_DFLT_TIMEOUT;
    rmdOptions.retries         = CKPT_RMD_DFLT_RETRIES;
    rmdOptions.priority        = 0;
    rmdOptions.transportHandle = 0;

    /*
     * Get the source address.
     */
    rc = clRmdSourceAddressGet(&destAddr.iocPhyAddress);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Fail: Nack Send problem [0x %x]\n", rc), rc);
            
    rc = clBufferCreate(&inMsg);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Fail: Nack Send problem [0x %x]\n", rc), rc);
        
    /*
     * Store the supported version info.
     */
    rc = clXdrMarshallClVersionT(&suppVersion,inMsg,0);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Fail: Nack Send failed [0x %x]\n", rc), rc);
            
    /*
     * Store the function no.
     */
    rc = clXdrMarshallClUint32T(&funId,inMsg,0);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Fail: Nack Send failed [0x %x]\n", rc), rc);

    /*
     * Make the RMD.
     */
    rc = clRmdWithMsg(destAddr, CKPT_NACK_SEND,
                      inMsg, 0, rmdFlags, &rmdOptions, NULL);
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("RMD Failed with an error %x", rc)); 
    }
exitOnError:
    {
        return rc;
    }
}

/*
 * Function to store the checkpoint info pulled from the active replica.
 */
 
ClRcT _ckptReplicaInfoUpdate(ClHandleT   ckptHdl,
                             VDECL_VER(CkptInfoT, 5, 0, 0)   *pCkptInfo)
{
    ClRcT             rc            = CL_OK;
    CkptT             *pCkpt        = NULL;
    ClUint32T         cksum         = 0;
    ClUint32T         count         = 0;
    ClUint32T         *presenceList = NULL;
    CkptSectionInfoT  *pSection     = NULL;
    ClCkptHdlT        *pCkptHdl     = NULL;
    ClCkptAppInfoT    *pAppInfo     = NULL;
    ClUint32T         *pData        = NULL;

    /*
     * Validate the checkpoint name.
     */
    CKPT_NULL_CHECK(pCkptInfo->pName);

    clLogDebug(CL_CKPT_AREA_PEER, CL_CKPT_CTX_REPL_UPDATE, 
               "Replicating the checkpoint [%.*s] at address [%d]", pCkptInfo->pName->length,
               pCkptInfo->pName->value, gCkptSvr->localAddr);
    /*
     * Add the active handle to the list of handles.
     */
    clCksm32bitCompute ((ClUint8T *)pCkptInfo->pName->value, pCkptInfo->pName->length,
                        &cksum);

    pCkptHdl = clHeapAllocate(sizeof(*pCkptHdl)); // Free where necessary NTC
    if(pCkptHdl == NULL)
    {
        rc =  CL_CKPT_ERR_NO_MEMORY;
        clLogWrite(CL_LOG_HANDLE_APP,CL_LOG_CRITICAL,CL_LOG_CKPT_LIB_NAME,
                   CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
        return rc;            
    }

    *pCkptHdl = ckptHdl;

    /*
     * Lock the active server ds.
     */ 
    CKPT_LOCK(gCkptSvr->ckptActiveSem);        

    /*
     * If there is an existing key, find and delete it.
     */
    (void)clCntNonUniqueKeyDelete(gCkptSvr->ckptHdlList, (ClPtrT)(ClWordT)cksum, 
                                  (ClPtrT)pCkptInfo->pName,
                                  ckptHdlNonUniqueKeyCompare);
    rc = clCntNodeAdd(gCkptSvr->ckptHdlList, (ClPtrT)(ClWordT)cksum,
                      pCkptHdl,NULL); 
    if(rc != CL_OK)
    {
        clHeapFree(pCkptHdl);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                       ("CheckpointInfo Updation Failed rc[0x %x]\n",
                        rc));
        /*
         * Unlock the active server ds.
         */ 
        CKPT_UNLOCK(gCkptSvr->ckptActiveSem );           
        return rc;            
    }

    clLogDebug(CL_CKPT_AREA_PEER, CL_CKPT_CTX_REPL_UPDATE, 
               "Adding ckpt hdl [%#llX] to ckpt hdls list rc[0x %x]", 
               ckptHdl, rc);
    /*
     * Obtain the active handle related memory from the handle database.
     */
    rc = clHandleCreateSpecifiedHandle(gCkptSvr->ckptHdl, sizeof(CkptT),
                                       *pCkptHdl);
    if((CL_OK != rc) && (CL_GET_ERROR_CODE(rc) != CL_ERR_ALREADY_EXIST)) 
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                       ("CheckpointHandle Create failed rc[0x %x]\n",
                        rc));
        /*
         * Unlock the active server ds.
         */ 
        CKPT_UNLOCK(gCkptSvr->ckptActiveSem );           
        return rc;      
    }
    
    /*
     * Retrieve the info associated with the active handle.
     */
    rc = ckptSvrHdlCheckout(gCkptSvr->ckptHdl, *pCkptHdl, (void **)&pCkpt);     
    /*
     * Unlock the active server ds.
     */ 
    CKPT_UNLOCK(gCkptSvr->ckptActiveSem );           
    CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_DEBUG_ERROR, 
                                  ("Failed to allocate memory for checkpoint %s rc[0x %x]\n",
                                   pCkptInfo->pName->value,rc),rc);

    /* 
     * Allocate memory for Control plane and Data plane info.
     */
    rc = _ckptDplaneInfoAlloc(&pCkpt->pDpInfo,
                              pCkptInfo->pDpInfo->maxScns); 
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR, 
                   ("Failed to allocate memory for checkpoint %s rc[0x %x]\n",
                    pCkpt->ckptName.value,rc),rc);

    rc = _ckptCplaneInfoAlloc(&pCkpt->pCpInfo); 
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR, 
                   ("Failed to allocate memory for checkpoint %s rc[0x %x]\n",
                    pCkpt->ckptName.value,rc),rc);

    /*
     * Create a mutex to safeguard the checkpoint.
     */
    clOsalMutexCreate( &pCkpt->ckptMutex );         
    /* Create the mutexs for the same */
    pCkpt->numMutex = CL_MIN(pCkpt->pDpInfo->maxScns, 
                             (ClUint32T)(sizeof(pCkpt->secMutex) / sizeof(pCkpt->secMutex[0])) );
    /* If default section is not there, create mutexes */
    if( pCkpt->pDpInfo->maxScns != 1 )
    {
        rc = clCkptSectionLevelMutexCreate(pCkpt->secMutex, pCkpt->numMutex);
        if( CL_OK != rc )
        {
            clLogError(CL_CKPT_AREA_PEER, CL_CKPT_CTX_REPL_UPDATE,
                       "Failed to create mutex while updating replica rc[0x %x]", rc);
            goto exitOnError;
        }
    }

    /*
     * Lock the checkpoint's mutex.
     */
    CKPT_LOCK(pCkpt->ckptMutex);           
    /*
     * Check if we raced with a ckpt delete
     */
    if(!pCkpt->ckptMutex)
    {
        clHandleCheckin(gCkptSvr->ckptHdl, ckptHdl);
        clLogWarning(CL_CKPT_AREA_PEER, CL_CKPT_CTX_REPL_UPDATE, "Ckpt [%.*s] already deleted for handle [%#llx]",
                     pCkptInfo->pName->length, pCkptInfo->pName->value, ckptHdl);
        return CL_OK;
    }
    clNameCopy(&pCkpt->ckptName, pCkptInfo->pName);

    /*
     * Update the control plane information.
     */
    if(pCkptInfo->pCpInfo != NULL)
    {
        pCkpt->pCpInfo->updateOption = pCkptInfo->pCpInfo->updateOption;
        pCkpt->pCpInfo->id = pCkptInfo->pCpInfo->id;
        presenceList                 = pCkptInfo->pCpInfo->presenceList;

        /*
         * Update the presence list with the passed replica entries.
         */
        for(count = 0; count < pCkptInfo->pCpInfo->size ; count++)
        {
            rc = _ckptPresenceListUpdate(pCkpt, *(presenceList));
            CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                           ("Failed to update the presenceList rc[0x %x]\n", rc), rc);
            (presenceList)++;
        }
        /*
         * Update the app info list 
         */
        pAppInfo = pCkptInfo->pCpInfo->pAppInfo;
        for( count = 0; count < pCkptInfo->pCpInfo->numApps; count++ )
        {
            clCkptActiveAppInfoUpdate(pCkpt, pAppInfo->nodeAddress,
                                      pAppInfo->portId, &pData);
            pAppInfo++;
        }

        /*
         * Add local address as well to the replica list.
         */
        rc = _ckptPresenceListUpdate(pCkpt, gCkptSvr->localAddr);
        CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                       ("Failed to update the presenceList rc[0x %x]\n", rc), rc);
    }

    /*
     * Update the data plane info.
     */
    if(pCkptInfo->pDpInfo != NULL)
    {
        pCkpt->pDpInfo->maxCkptSize  = pCkptInfo->pDpInfo->maxCkptSize;
        pCkpt->pDpInfo->maxScns      = pCkptInfo->pDpInfo->maxScns;
        pCkpt->pDpInfo->maxScnSize   = pCkptInfo->pDpInfo->maxScnSize;
        pCkpt->pDpInfo->maxScnIdSize = pCkptInfo->pDpInfo->maxScnIdSize;
        pCkpt->pDpInfo->indexGenScns = pCkptInfo->pDpInfo->genIndex;

        /*
         * Copy the sections and start the expiry timer if running.
         */
        pSection = pCkptInfo->pDpInfo->pSection;
        if( pCkptInfo->pDpInfo->maxScns == 1 )
        {
            /* Adding the default section */
            rc = clCkptDefaultSectionAdd(pCkpt, pSection->size,
                                         pSection->pData);
            if(CL_GET_ERROR_CODE(rc) == CL_ERR_DUPLICATE) rc = CL_OK;
            if( CL_OK != rc )
            {
                clLogError(CL_CKPT_AREA_PEER, CL_CKPT_CTX_CKPT_OPEN, 
                           "Adding the default sections failed rc[0x %x]", rc);
                goto exitOnError;
            }
            pCkpt->pDpInfo->numScns      = 0;
        }
        else
        {
            for( count = 0; count < pCkptInfo->pDpInfo->numScns; count++)
            {
                rc = clCkptSectionChkNAdd(ckptHdl, pCkpt, &pSection->secId, 
                                          pSection->expiryTime, pSection->size, 
                                          pSection->pData); 
                if( CL_CKPT_ERR_ALREADY_EXIST == rc )
                {
                    rc = CL_OK;
                }
                if( CL_OK != rc )
                {
                    clLogError(CL_CKPT_AREA_PEER, CL_CKPT_CTX_CKPT_OPEN, 
                               "Adding the sections failed rc[0x %x]", rc);
                    goto exitOnError;
                }
                if(!strncmp((ClCharT *)pSection->secId.id, 
                            (ClCharT *)"generatedSection",
                            strlen("generatedSection")))
                {
                    pCkpt->pDpInfo->indexGenScns++;
                }
                pSection++;         
                pCkpt->pDpInfo->numScns++;
            }
        }
    }
    clHandleCheckin(gCkptSvr->ckptHdl,*pCkptHdl);

    /* 
     * Update presence list across all replicas. 
     * Dont care about errors in presence list peer updates since
     * some peers could have this replica in its presence list or some wont.
     * But this node is anyway having the ckptinfo replicated. So need
     * to unwind incase of peer presence list update as peers would update
     * their presence list on a peer down.
     */     

    _ckptPresenceListPeerUpdate(pCkpt, ckptHdl, gCkptSvr->localAddr);

    /*
     * Unlock the checkpoint's mutex.
     */
    CKPT_UNLOCK(pCkpt->ckptMutex);           

    return rc;
    exitOnError:
    /*
     * cleaning up the resources.
     */
    clHandleCheckin(gCkptSvr->ckptHdl,*pCkptHdl);

    /*
     * Unlock the checkpoint's mutex.
     */
    CKPT_UNLOCK(pCkpt->ckptMutex);           
    exitOnErrorBeforeHdlCheckout:
    /*
     * Lock the active server db.
     */
    CKPT_LOCK(gCkptSvr->ckptActiveSem);

    /*
     * Failed to replicate the checkpoint info . Hence delete the
     * handle entry form the ckpt handle list.
     */
    clCntNonUniqueKeyDelete(gCkptSvr->ckptHdlList, (ClPtrT)(ClWordT)cksum, 
                            (ClCntDataHandleT)pCkptInfo->pName, ckptHdlNonUniqueKeyCompare);

    /*
     * Unlock the active server db.
     */
    CKPT_UNLOCK( gCkptSvr->ckptActiveSem);
    return rc;
}

/*
 * Function to remove a checkpoint at the  active node.
 */
 
ClRcT VDECL_VER(clCkptRemSvrCkptDelete, 4, 0, 0)(ClVersionT  *pVersion,
                             ClHandleT   ckptHdl)
{
    ClRcT rc     = CL_OK;
    CkptT *pCkpt = NULL;

    /*
     * Check whether the server is fully up or not.
     */
    CL_CKPT_SVR_EXISTENCE_CHECK;
    
    /*
     * Verify the version.
     */
    rc = clVersionVerify(&gCkptSvr->versionDatabase,pVersion);
    if( CL_OK != rc )
    {
        clLogError(CL_CKPT_AREA_PEER, CL_CKPT_CTX_CKPT_DEL,
                "Version mismatch happened rc[0x %x]", rc);
        return rc;
    }

    CKPT_LOCK(gCkptSvr->ckptActiveSem);
    /* 
     * Retrieve the info associated with the active handle.
     */
    rc = clHandleCheckout(gCkptSvr->ckptHdl,ckptHdl,(void **)&pCkpt);
    if( CL_OK != rc )
    {
        CKPT_UNLOCK(gCkptSvr->ckptActiveSem);
        clLogError(CL_CKPT_AREA_PEER, CL_CKPT_CTX_CKPT_DEL, 
                "Failed to checkout handle [%#llX] rc [0x %x]", 
                 ckptHdl, rc);
        return rc;
    }
    if(pCkpt == NULL)                 
    {
        rc = CL_CKPT_ERR_INVALID_STATE;
        clHandleCheckin(gCkptSvr->ckptHdl, ckptHdl);
        CKPT_UNLOCK(gCkptSvr->ckptActiveSem);
        return rc;
    }

    /* 
     * Delete the ckpt & sections 
     */
    rc = clCkptSvrReplicaDelete(pCkpt, ckptHdl, CL_FALSE);
    clHandleCheckin(gCkptSvr->ckptHdl, ckptHdl);

    CKPT_UNLOCK(gCkptSvr->ckptActiveSem);
    return rc;
}


/*
 * Checkpoint write vector function at the replica node.
 */
ClRcT VDECL_VER(clCkptRemSvrCkptWriteVector, 4, 0, 0)(ClVersionT             *pVersion,
                                                      ClCkptHdlT             ckptHdl,
                                                      ClIocNodeAddressT      nodeAddress,
                                                      ClIocPortT             portId,
                                                      ClUint32T              numberOfElements,
                                                      ClCkptDifferenceIOVectorElementT *pDifferenceIoVector)
{
    ClRcT      rc        = CL_OK;
    ClUint32T  errCount  = 0;
    CkptT      *pCkpt    = NULL;

    /*
     * Check whether the server is fully up or not.
     */
    if (gCkptSvr == NULL || gCkptSvr->serverUp == CL_FALSE) 
    {
        rc = CL_CKPT_ERR_TRY_AGAIN;
        goto exitOnError;
    }

    /*
     * Verify the version.
     */
    rc = clVersionVerify(&gCkptSvr->versionDatabase,pVersion);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Ckpt: VersionMismatch rc[0x %x]\n", rc), rc);

    /*
     * Retrieve the data associated with the active handle.
     */
    rc = clHandleCheckout(gCkptSvr->ckptHdl, ckptHdl, (void **)&pCkpt);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Handle [%#llX] failed during updation rc[0x %x]\n", ckptHdl, rc), rc);


    /*
     * Lock the checkpoint's mutex.
     */
    CKPT_LOCK(pCkpt->ckptMutex);
    /*
     * Check if we raced with a delete
     */
    if(!pCkpt->ckptMutex)
    {
        clHandleCheckin(gCkptSvr->ckptHdl, ckptHdl);
        clLogWarning(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_SEC_OVERWRITE, 
                     "Ckpt with handle [%#llx] deleted", ckptHdl);
        goto exitOnError;
    }

    /*
     * Update the checkpoint data.
     */
    rc = _ckptCheckpointLocalWriteVector(ckptHdl,
                                         numberOfElements,
                                         pDifferenceIoVector,&errCount, nodeAddress,
                                         portId, NULL, NULL, NULL);
    if( CL_GET_ERROR_CODE(rc) == CL_ERR_INVALID_STATE )
    {
        /*
         * Handle checkout failed at that function which didn't release the
         * mutex so release and exit 
         */
        clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_SEC_OVERWRITE,
                "Failed to write on the local copy rc [0x %x]" ,rc);
        /*
         * Unlock the checkpoint's mutex.
         */
        CKPT_UNLOCK(pCkpt->ckptMutex);
        clHandleCheckin(gCkptSvr->ckptHdl, ckptHdl);
        goto exitOnError;
    }
    if( CL_OK != rc )
    {
        /* any other error, this would have unheld the lock there so just go
         * out, return from here 
         */
        clHandleCheckin(gCkptSvr->ckptHdl, ckptHdl);
        goto exitOnError;
    }
    /*
     * Unlock the checkpoint's mutex.
     */
    CKPT_UNLOCK(pCkpt->ckptMutex);

    /*
     * Checkin the data associated with active handle.
     */
    clHandleCheckin(gCkptSvr->ckptHdl, ckptHdl);

exitOnError:    
    /* as we need to release all these memory allocated even though some
     * errors, moving under exitOnError
     */
    clCkptDifferenceIOVectorFree(pDifferenceIoVector, numberOfElements);

    return rc;
}

/*
 * Checkpoint write function at the replica node.
 */
ClRcT VDECL_VER(clCkptRemSvrCkptWrite, 4, 0, 0)(ClVersionT             *pVersion,
                            ClCkptHdlT             ckptHdl,
                            ClIocNodeAddressT      nodeAddress,
                            ClIocPortT             portId,
                            ClUint32T              numberOfElements,
                            ClCkptIOVectorElementT *pIoVector)
{
    ClRcT      rc        = CL_OK;
    ClUint32T  errCount  = 0;
    CkptT      *pCkpt    = NULL;
    ClUint32T  count     = 0;

    /*
     * Check whether the server is fully up or not.
     */
    if (gCkptSvr == NULL || gCkptSvr->serverUp == CL_FALSE) 
    {
        rc = CL_CKPT_ERR_TRY_AGAIN;
        goto exitOnError;
    }

    /*
     * Verify the version.
     */
    rc = clVersionVerify(&gCkptSvr->versionDatabase,pVersion);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Ckpt: VersionMismatch rc[0x %x]\n", rc), rc);

    /*
     * Retrieve the data associated with the active handle.
     */
    rc = clHandleCheckout(gCkptSvr->ckptHdl, ckptHdl, (void **)&pCkpt);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Handle [%#llX] failed during updation rc[0x %x]\n", ckptHdl, rc), rc);


    /*
     * Lock the checkpoint's mutex.
     */
    CKPT_LOCK(pCkpt->ckptMutex);
    /*
     * Check if we raced with a delete
     */
    if(!pCkpt->ckptMutex)
    {
        clHandleCheckin(gCkptSvr->ckptHdl, ckptHdl);
        clLogWarning(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_SEC_OVERWRITE, 
                     "Ckpt with handle [%#llx] is already deleted", ckptHdl);
        goto exitOnError;
    }

    /*
     * Update the checkpoint data.
     */
    rc = _ckptCheckpointLocalWrite(ckptHdl,
            numberOfElements,
            pIoVector,&errCount, nodeAddress,
            portId);
    if( CL_GET_ERROR_CODE(rc) == CL_ERR_INVALID_STATE )
    {
        /*
         * Handle checkout failed at that function which didn't release the
         * mutex so release and exit 
         */
        clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_SEC_OVERWRITE,
                "Failed to write on the local copy rc [0x %x]" ,rc);
        /*
         * Unlock the checkpoint's mutex.
         */
        CKPT_UNLOCK(pCkpt->ckptMutex);
        clHandleCheckin(gCkptSvr->ckptHdl, ckptHdl);
        goto exitOnError;
    }
    if( CL_OK != rc )
    {
        /* any other error, this would have unheld the lock there so just go
         * out, return from here 
         */
        clHandleCheckin(gCkptSvr->ckptHdl, ckptHdl);
        goto exitOnError;
    }
    /*
     * Unlock the checkpoint's mutex.
     */
    CKPT_UNLOCK(pCkpt->ckptMutex);

    /*
     * Checkin the data associated with active handle.
     */
    clHandleCheckin(gCkptSvr->ckptHdl, ckptHdl);
exitOnError:    
    /* as we need to release all these memory allocated even though some
     * errors, moving under exitOnError
     */
    for( count = 0; count <  numberOfElements; count++)
    {
        if(pIoVector->sectionId.id != NULL)
        {
            clHeapFree(pIoVector->sectionId.id);
            pIoVector->sectionId.id = NULL;
        }
        clHeapFree(pIoVector->dataBuffer);
        pIoVector++;
    }
    return rc;
}
                            


/*
 * Function at replica nodes for changing the value of section
 * expiration for a given section. Called by active replica 
 * whenever there is a change in expiration time.
 */

ClRcT VDECL_VER(clCkptRemSvrSectionExpTimeSet, 4, 0, 0)(ClVersionT        *pVersion,
                                    ClCkptHdlT        ckptHdl,
                                    ClCkptSectionIdT  *pSectionId,
                                    ClTimeT           exprTime) 
{
    ClRcT rc = CL_OK;

    /*
     * Update the expiration time locally.
     */
    rc = _ckptLocalSectionExpirationTimeSet(pVersion, ckptHdl, 
                                            pSectionId, exprTime);

    return rc;
}



/* 
 * Function to check whether ckpt server is running on the given
 * node or not.
 */

ClRcT clCkptIsServerRunning(ClIocNodeAddressT addr, ClUint8T *pIsRunning)
{
    ClRcT            rc           = CL_OK;
    CkptPeerInfoT    *pPeerInfo   = NULL;

    /*
     * Find the corresponding entry from the peer list.
     */
    if( CL_OK != (rc = clCntDataForKeyGet(gCkptSvr->masterInfo.peerList,
                            (ClCntKeyHandleT)(ClWordT)addr,
                            (ClCntDataHandleT *)&pPeerInfo)))
    {
        *pIsRunning = 0;
    }
    else
    {
        if(pPeerInfo->available == CL_CKPT_NODE_UNAVAIL)
            *pIsRunning = 0;
        else
            *pIsRunning = 1;
    }

    return rc;
}

void clCkptRemSvcSecCreateCb(ClIdlHandleT                     ckptIdlHdl,
                             ClCkptHdlT                       ckptHdl,
                             ClBoolT                          srcClient,
                             ClCkptSectionCreationAttributesT *pSecCreateAttr,
                             ClUint8T                         *pInitialData,
                             ClSizeT                          size,
                             ClVersionT                       *pVersion, 
                             ClUint32T                        *index,
                             ClRcT                            retCode,
                             void                             *pData)
{
    ClIocAddressT   srcAddr     = {{0}};
    ClRcT           rc          =  CL_OK;
    ClVersionT      ckptVersion =  {0}; 
    CkptCbInfoT    *pCbInfo     = (CkptCbInfoT *)pData;
    CkptT *pCkpt = NULL;
    
    if(!gCkptSvr || !gCkptSvr->serverUp)
    {
        goto process_resp;
    }

    rc = clHandleCheckout(gCkptSvr->ckptHdl, ckptHdl, (void**)&pCkpt);
    if(rc != CL_OK)
    {
        clLogError("CREATE", "CB", "Handle checkout for ckpt handle [%llx] returned [%#x]",
                   ckptHdl, rc);
        goto process_resp;
    }

    CKPT_LOCK(pCkpt->ckptMutex);
    /*
     * Check for a race with a ckpt delete.
     */
    if(!pCkpt->ckptMutex)
    {
        clHandleCheckin(gCkptSvr->ckptHdl, ckptHdl);
        pCkpt = NULL;
        clLogWarning("CREATE", "CB", "Ckpt with handle [%#llx] already deleted", ckptHdl);
    }
    /*
     * Decrement the callback count associated with the section
     * updation. If the count reaches 0, inform the client about SUCCESS
     * or FAILURE.
     */
    process_resp:
    (pCbInfo->cbCount)--;
    clLogDebug(CL_CKPT_AREA_PEER, CL_CKPT_CTX_CKPT_OPEN, 
            "Section create callback cbcount [%d] handle [%llx]",
            pCbInfo->cbCount, ckptHdl);
    if( retCode != CL_OK ) 
        pCbInfo->retCode = retCode;
    if( pCbInfo->cbCount == 0 )
    {
        if(CL_GET_ERROR_CODE(retCode) == CL_ERR_VERSION_MISMATCH)
        {
            memcpy(&ckptVersion, pVersion, sizeof(ClVersionT));
            rc = clRmdSourceAddressGet(&srcAddr.iocPhyAddress);
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL,
                    CL_CKPT_LOG_6_VERSION_NACK, "CkptRemSvrAdd",
                    srcAddr.iocPhyAddress.nodeAddress,
                    srcAddr.iocPhyAddress.portId, pVersion->releaseCode,
                    pVersion->majorVersion, pVersion->minorVersion);

            clLogError(CL_CKPT_AREA_PEER, CL_CKPT_CTX_CKPT_OPEN, 
                    "Ckpt nack recieved from NODE [0x%x:0x%x] rc [0x%x]", 
                    srcAddr.iocPhyAddress.nodeAddress,
                    srcAddr.iocPhyAddress.portId,
                    CL_CKPT_ERR_VERSION_MISMATCH);
        }
        memset(&ckptVersion, '\0', sizeof(ClVersionT));
        /*
         * Based on updateFlag call the appropriate function.
         */
        rc = VDECL_VER(_ckptSectionCreateResponseSend, 4, 0, 0)( pCbInfo->rmdHdl,
                pCbInfo->retCode,
                ckptVersion,0);
        clHeapFree(pCbInfo); 
    }
    if(pCkpt)
    {
        CKPT_UNLOCK(pCkpt->ckptMutex);
        clHandleCheckin(gCkptSvr->ckptHdl, ckptHdl);
    }
    if( NULL != pInitialData ) clHeapFree(pInitialData);
    clHeapFree(pSecCreateAttr->sectionId->id);
    clHeapFree(pSecCreateAttr->sectionId);
    return;
}

static void clCkptRemSvrSecOverwriteVectorCb(ClIdlHandleT      ckptIdlHdl,
                                             ClCkptHdlT        ckptHdl,
                                             ClIocNodeAddressT localAddr,
                                             ClIocPortT        localPort,
                                             ClBoolT           srcClient,
                                             ClCkptSectionIdT  *pSectionId,              
                                             ClTimeT           expirationTime,
                                             ClSizeT           size,
                                             ClDifferenceVectorT  *differenceVector,
                                             ClVersionT        *pVersion, 
                                             ClRcT             retCode,
                                             void              *pData)
{
    ClIocAddressT   srcAddr     = {{0}};
    ClRcT           rc          =  CL_OK;
    ClVersionT      ckptVersion =  {0}; 
    CkptCbInfoT    *pCbInfo     = (CkptCbInfoT *)pData;
    CkptT *pCkpt = NULL;
    ClBoolT sectionLockTaken = CL_TRUE;
    /*
     * Decrement the callback count associated with the section
     * updation. If the count reaches 0, inform the client about SUCCESS
     * or FAILURE.
     */
    if(!gCkptSvr || !gCkptSvr->serverUp)
        goto process_resp;

    rc = clHandleCheckout(gCkptSvr->ckptHdl, ckptHdl, (void**)&pCkpt);
    if(rc != CL_OK)
    {
        clLogError("OVERWRITE", "CB", "Handle checkout for ckpt handle [%llx] returned [%#x]",
                   ckptHdl, rc);
        goto process_resp;
    }
    CKPT_LOCK(pCkpt->ckptMutex);
    /*
     * Check if racing with a delete.
     */
    if(!pCkpt->ckptMutex)
    {
        clHandleCheckin(gCkptSvr->ckptHdl, ckptHdl);
        clLogWarning("OVERWRITE", "CB", "Ckpt with handle [%#llx] already deleted", ckptHdl);
        pCkpt = NULL;
        goto process_resp;
    }
    rc = clCkptSectionLevelLock(pCkpt, pSectionId,  &sectionLockTaken);
    if(rc != CL_OK)
    {
        goto process_resp;
    }
    if(sectionLockTaken == CL_TRUE)
    {
        CKPT_UNLOCK(pCkpt->ckptMutex);
    }
    process_resp:
    (pCbInfo->cbCount)--;
    clLogDebug(CL_CKPT_AREA_PEER, CL_CKPT_CTX_CKPT_OPEN, 
               "Section create callback cbcount [%d] handle [%llx]",
               pCbInfo->cbCount, ckptHdl);
    if( retCode != CL_OK ) 
        pCbInfo->retCode = retCode;
    if( pCbInfo->cbCount == 0 )
    {
        if(CL_GET_ERROR_CODE(retCode) == CL_ERR_VERSION_MISMATCH)
        {
            memcpy(&ckptVersion, pVersion, sizeof(ClVersionT));
            rc = clRmdSourceAddressGet(&srcAddr.iocPhyAddress);
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL,
                       CL_CKPT_LOG_6_VERSION_NACK, "CkptRemSvrSecoverwrite",
                       srcAddr.iocPhyAddress.nodeAddress,
                       srcAddr.iocPhyAddress.portId, pVersion->releaseCode,
                       pVersion->majorVersion, pVersion->minorVersion);

            clLogError(CL_CKPT_AREA_PEER, CL_CKPT_CTX_CKPT_OPEN, 
                       "Ckpt nack recieved from NODE [0x%x:0x%x] rc [0x%x]", 
                       srcAddr.iocPhyAddress.nodeAddress,
                       srcAddr.iocPhyAddress.portId,
                       CL_CKPT_ERR_VERSION_MISMATCH);
        }
        memset(&ckptVersion, '\0', sizeof(ClVersionT));
        /*
         * Based on updateFlag call the appropriate function.
         */
        rc = VDECL_VER(_ckptSectionOverwriteResponseSend, 4, 0, 0)( pCbInfo->rmdHdl,
                                                                    retCode,
                                                                    ckptVersion);
        clHeapFree(pCbInfo); 
    }
    
    if(pCkpt)
    {
        if(sectionLockTaken)
        {
            clCkptSectionLevelUnlock(pCkpt, pSectionId, sectionLockTaken);
        }
        else
        {
            CKPT_UNLOCK(pCkpt->ckptMutex);
        }
        clHandleCheckin(gCkptSvr->ckptHdl, ckptHdl);
    }
    if(differenceVector)
        clDifferenceVectorFree(differenceVector, CL_TRUE);
    clHeapFree(pSectionId->id);
    return;
}


static void clCkptRemSvrSecOverwriteCb(ClIdlHandleT      ckptIdlHdl,
                                       ClCkptHdlT        ckptHdl,
                                       ClIocNodeAddressT localAddr,
                                       ClIocPortT        localPort,
                                       ClBoolT           srcClient,
                                       ClCkptSectionIdT  *pSectionId,              
                                       ClTimeT           expirationTime,
                                       ClSizeT           size,
                                       ClUint8T          *pInitialData,
                                       ClVersionT        *pVersion, 
                                       ClRcT             retCode,
                                       void              *pData)
{
    ClDifferenceVectorT differenceVector = {0};
    differenceVector.numDataVectors = 1;
    differenceVector.dataVectors = clHeapCalloc(1, sizeof(*differenceVector.dataVectors));
    CL_ASSERT(differenceVector.dataVectors != NULL);
    differenceVector.dataVectors[0].dataBlock = 0;
    differenceVector.dataVectors[0].dataBase = pInitialData;
    differenceVector.dataVectors[0].dataSize = size;
    clCkptRemSvrSecOverwriteVectorCb(ckptIdlHdl, ckptHdl, localAddr, localPort, srcClient, pSectionId,
                                      expirationTime, size, &differenceVector, pVersion, retCode, pData);
}


void clCkptRemSvrSecDeleteCb(ClIdlHandleT      ckptIdlHdl,
                             ClCkptHdlT        ckptHdl,
                             ClBoolT           srcClient,
                             ClCkptSectionIdT  *pSectionId,              
                             ClVersionT        *pVersion, 
                             ClRcT             retCode,
                             void              *pData)
{
    ClIocAddressT   srcAddr     = {{0}};
    ClRcT           rc          =  CL_OK;
    ClVersionT      ckptVersion =  {0}; 
    CkptCbInfoT    *pCbInfo     = (CkptCbInfoT *)pData;
    CkptT *pCkpt = NULL;
    ClBoolT sectionLockTaken = CL_TRUE;

    /*
     * Decrement the callback count associated with the section
     * updation. If the count reaches 0, inform the client about SUCCESS
     * or FAILURE.
     */
    if(!gCkptSvr || !gCkptSvr->serverUp)
        goto process_resp;

    rc = clHandleCheckout(gCkptSvr->ckptHdl, ckptHdl, (void**)&pCkpt);
    if(rc != CL_OK)
    {
        clLogError("DELETE", "CB", "Handle checkout for ckpt handle [%llx] returned [%#x]",
                   ckptHdl, rc);
        goto process_resp;
    }
    CKPT_LOCK(pCkpt->ckptMutex);
    if(!pCkpt->ckptMutex)
    {
        clHandleCheckin(gCkptSvr->ckptHdl, ckptHdl);
        clLogWarning("DELETE", "CB", "Ckpt with handle [%#llx] found deleted", ckptHdl);
        pCkpt = NULL;
        goto process_resp;
    }
    rc = clCkptSectionLevelLock(pCkpt, pSectionId,  &sectionLockTaken);
    if(rc != CL_OK)
    {
        goto process_resp;
    }
    if(sectionLockTaken == CL_TRUE)
    {
        CKPT_UNLOCK(pCkpt->ckptMutex);
    }

    /*
     * Decrement the callback count associated with the section
     * updation. If the count reaches 0, inform the client about SUCCESS
     * or FAILURE.
     */
    process_resp:
    (pCbInfo->cbCount)--;
    clLogDebug(CL_CKPT_AREA_PEER, CL_CKPT_CTX_CKPT_OPEN, 
            "Section delete callback cbcount [%d] handle [%llx]",
            pCbInfo->cbCount, ckptHdl);
    if( retCode != CL_OK ) 
        pCbInfo->retCode = retCode;
    if( pCbInfo->cbCount == 0 )
    {
        if(CL_GET_ERROR_CODE(retCode) == CL_ERR_VERSION_MISMATCH)
        {
            memcpy(&ckptVersion, pVersion, sizeof(ClVersionT));
            rc = clRmdSourceAddressGet(&srcAddr.iocPhyAddress);
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL,
                    CL_CKPT_LOG_6_VERSION_NACK, "CkptRemSvrSecoverwrite",
                    srcAddr.iocPhyAddress.nodeAddress,
                    srcAddr.iocPhyAddress.portId, pVersion->releaseCode,
                    pVersion->majorVersion, pVersion->minorVersion);

            clLogError(CL_CKPT_AREA_PEER, CL_CKPT_CTX_CKPT_OPEN, 
                    "Ckpt nack recieved from NODE [0x%x:0x%x] rc [0x%x]", 
                    srcAddr.iocPhyAddress.nodeAddress,
                    srcAddr.iocPhyAddress.portId,
                    CL_CKPT_ERR_VERSION_MISMATCH);
        }
        memset(&ckptVersion, '\0', sizeof(ClVersionT));
        /*
         * Based on updateFlag call the appropriate function.
         */
        rc = VDECL_VER(_ckptSectionDeleteResponseSend, 4, 0, 0)( pCbInfo->rmdHdl,
                                                                 retCode,
                                                                 ckptVersion);
        clHeapFree(pCbInfo); 
    }

    if(pCkpt)
    {
        if(sectionLockTaken)
        {
            clCkptSectionLevelUnlock(pCkpt, pSectionId, sectionLockTaken);
        }
        else
        {
            CKPT_UNLOCK(pCkpt->ckptMutex);
        }
        clHandleCheckin(gCkptSvr->ckptHdl, ckptHdl);
    }
    clHeapFree(pSectionId->id);
    return;
}

ClRcT
clCkptRemSvrSectionCreate(ClCkptHdlT                        ckptHdl, 
                          CkptT                             *pCkpt, 
                          ClCkptSectionCreationAttributesT  *pSecCreateAttr,
                          ClUint32T                         initialDataSize,
                          ClUint8T                          *pInitialData)
{
    ClRcT              rc          = CL_OK;
    ClCntNodeHandleT   nodeHdl     = 0;
    ClIocNodeAddressT  peerAddr    = 0;
    ClCntDataHandleT   dataHdl     = 0;
    CkptCbInfoT        *pCallInfo  = NULL;
    ClVersionT         ckptVersion = {0};
    ClCharT            *pTempData  = "";
    ClUint32T           index      = 0;

    if( (pInitialData == NULL) && (initialDataSize == 0) )
    {
        pInitialData = (ClUint8T *) pTempData;
    }
    /* 
     * This maks sense only in case of sync checkpoint.
     */
    if((pCkpt->pCpInfo->updateOption & CL_CKPT_WR_ALL_REPLICAS))
    {
        /* 
         * Set the defer sync handle.
         */
        pCallInfo = (CkptCbInfoT *)clHeapCalloc(1,sizeof(CkptCbInfoT));
        if(pCallInfo == NULL)
        {
            rc = CL_CKPT_ERR_NO_MEMORY;
            CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                    ("Failed to allocate the memory rc[0x %x]\n", rc), rc);
        }
        pCallInfo->delFlag   = CL_TRUE;
        pCallInfo->cbCount   = 0;
        rc = clCkptEoIdlSyncDefer((ClIdlHandleT *)&pCallInfo->rmdHdl);
        CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                ("Failed to defer the call rc[0x %x]\n", rc), rc);
    }

    /*
     * Set the supported version information.
     */
    memcpy( &ckptVersion, gCkptSvr->versionDatabase.versionsSupported,
            sizeof(ClVersionT));
    /* 
     * Walk through the checkpoint's presence list.
     * Inform all the replica nodes of the checkpoint.
     */
    clCntFirstNodeGet (pCkpt->pCpInfo->presenceList, &nodeHdl);
    while (nodeHdl != 0)
    {
        rc = clCntNodeUserKeyGet (pCkpt->pCpInfo->presenceList, nodeHdl, 
                (ClCntKeyHandleT *) &dataHdl);
        CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                ("Cant update peer rc[0x %x]\n", rc), rc);
        peerAddr = (ClIocNodeAddressT)(ClWordT)dataHdl;
        clCntNextNodeGet(pCkpt->pCpInfo->presenceList, nodeHdl, &nodeHdl);

        /*
         * Dont make the call if the presence list node is the active replica
         * itself.
         */
        if (peerAddr != gCkptSvr->localAddr)
        {
            /*
             * Allocate memory for section data to be packed and sent to 
             * replicas.
             */
            rc = ckptIdlHandleUpdate(peerAddr,gCkptSvr->ckptIdlHdl,0);
            CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                    ("Failed to update the handle rc[0x %x]\n", rc), rc);

            /*
             * Update the replica.
             */
            if((pCkpt->pCpInfo->updateOption & CL_CKPT_WR_ALL_REPLICAS ))
            {
                /*
                 * Use the defer sync concept.
                 */
                pCallInfo->cbCount++;
                rc = VDECL_VER(_ckptSectionCreateClientAsync, 4, 0, 0)(
                        gCkptSvr->ckptIdlHdl,
                        ckptHdl, 
                        CL_FALSE, 
                        pSecCreateAttr, 
                        (ClUint8T *) pInitialData,
                        initialDataSize, 
                        &ckptVersion,
                        &index,
                        clCkptRemSvcSecCreateCb, //_ckptRemSvrSectionUpdateCallback,
                        pCallInfo);
                if(rc != CL_OK)
                {
                    pCallInfo->cbCount--;
                }
            }
            else
            {
                /*
                 * Make async call.
                 */
                rc = VDECL_VER(_ckptSectionCreateClientAsync, 4, 0, 0)(
                        gCkptSvr->ckptIdlHdl,
                        ckptHdl, 
                        CL_FALSE, 
                        pSecCreateAttr, 
                        (ClUint8T *) pInitialData,
                        initialDataSize, 
                        &ckptVersion,
                        &index,
                        NULL, NULL);
            }
            CL_CKPT_UNREACHABEL_ERR_CHK(rc, peerAddr);
        }
    }
    return rc;

exitOnError:
    {
        /*
         * pCallInfo should be freed incase of failure or in the Callback.
         */
        if(pCallInfo != NULL)
            clHeapFree(pCallInfo);

    }
exitOnErrorBeforeHdlCheckout:
    return rc;
}

ClRcT
clCkptRemSvrSectionOverwriteVector(ClCkptHdlT        ckptHdl, 
                                   CkptT             *pCkpt, 
                                   ClCkptSectionIdT  *pSectionId, 
                                   CkptSectionT      *pSec,
                                   ClUint8T          *pData,
                                   ClUint32T         dataSize,
                                   ClIocNodeAddressT nodeAddress,
                                   ClIocPortT        portId,
                                   ClDifferenceVectorT    *differenceVector)
{
    ClRcT              rc          = CL_OK;
    ClCntNodeHandleT   nodeHdl     = 0;
    ClCntKeyHandleT    dataHdl     = 0;
    ClIocNodeAddressT  peerAddr    = 0;
    CkptCbInfoT        *pCallInfo  = NULL;
    ClVersionT         ckptVersion = {0};

    /* 
     * This maks sense only in case of sync checkpoint.
     */
    if((pCkpt->pCpInfo->updateOption & CL_CKPT_WR_ALL_REPLICAS))
    {
        /* 
         * Set the defer sync handle.
         */
        pCallInfo = (CkptCbInfoT *)clHeapCalloc(1,sizeof(CkptCbInfoT));
        if(pCallInfo == NULL)
        {
            rc = CL_CKPT_ERR_NO_MEMORY;
            CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                                          ("Failed to allocate the memory rc[0x %x]\n", rc), rc);
        }
        pCallInfo->delFlag   = CL_TRUE;
        pCallInfo->cbCount   = 0;
        rc = clCkptEoIdlSyncDefer((ClIdlHandleT *)&pCallInfo->rmdHdl);
        CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                       ("Failed to defer the call rc[0x %x]\n", rc), rc);
    }

    /*
     * Set the supported version information.
     */
    memcpy( &ckptVersion, gCkptSvr->versionDatabase.versionsSupported,
            sizeof(ClVersionT));
    /* 
     * Walk through the checkpoint's presence list.
     * Inform all the replica nodes of the checkpoint.
     */
    clCntFirstNodeGet (pCkpt->pCpInfo->presenceList, &nodeHdl);
    while (nodeHdl != 0)
    {
        rc = clCntNodeUserKeyGet (pCkpt->pCpInfo->presenceList, nodeHdl, &dataHdl);
        CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                       ("Cant update peer rc[0x %x]\n", rc), rc);
        peerAddr = (ClIocNodeAddressT)(ClWordT)dataHdl;
        clCntNextNodeGet(pCkpt->pCpInfo->presenceList, nodeHdl, &nodeHdl);

        /*
         * Dont make the call if the presence list node is the active replica
         * itself.
         */
        if (peerAddr != gCkptSvr->localAddr)
        {
            /*
             * Allocate memory for section data to be packed and sent to 
             * replicas.
             */
            rc = ckptIdlHandleUpdate(peerAddr,gCkptSvr->ckptIdlHdl,0);
            CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                           ("Failed to update the handle rc[0x %x]\n", rc), rc);

            /*
             * Update the replica.
             */
            if((pCkpt->pCpInfo->updateOption & CL_CKPT_WR_ALL_REPLICAS ))
            {
                /*
                 * Use the defer sync concept.
                 */
                pCallInfo->cbCount++;
                if(differenceVector)
                {
                    rc = VDECL_VER(_ckptSectionOverwriteVectorClientAsync, 4, 0, 0)(
                                                                                    gCkptSvr->ckptIdlHdl,
                                                                                    ckptHdl, 
                                                                                    nodeAddress, 
                                                                                    portId, 
                                                                                    CL_FALSE, 
                                                                                    pSectionId, 
                                                                                    pSec->exprTime, 
                                                                                    dataSize, 
                                                                                    differenceVector,
                                                                                    &ckptVersion,
                                                                                    clCkptRemSvrSecOverwriteVectorCb, 
                                                                                    pCallInfo);
                }
                else
                {
                    rc = VDECL_VER(_ckptSectionOverwriteClientAsync, 4, 0, 0)(
                                                                              gCkptSvr->ckptIdlHdl,
                                                                              ckptHdl, 
                                                                              nodeAddress, 
                                                                              portId, 
                                                                              CL_FALSE, 
                                                                              pSectionId, 
                                                                              pSec->exprTime, 
                                                                              dataSize, 
                                                                              (ClUint8T *) pData,  
                                                                              &ckptVersion,
                                                                              clCkptRemSvrSecOverwriteCb, 
                                                                              pCallInfo);

                }
                if(rc != CL_OK)
                {
                    pCallInfo->cbCount--;
                }
            }
            else
            {
                /*
                 * Make async call.
                 */
                if(differenceVector)
                {
                    rc = VDECL_VER(_ckptSectionOverwriteVectorClientAsync, 4, 0, 0)(
                                                                                    gCkptSvr->ckptIdlHdl,
                                                                                    ckptHdl, 
                                                                                    nodeAddress,
                                                                                    portId,
                                                                                    CL_FALSE, 
                                                                                    pSectionId, 
                                                                                    pSec->exprTime, 
                                                                                    dataSize, 
                                                                                    differenceVector,
                                                                                    &ckptVersion, NULL, NULL);
                }
                else
                {
                    rc = VDECL_VER(_ckptSectionOverwriteClientAsync, 4, 0, 0)(
                                                                              gCkptSvr->ckptIdlHdl,
                                                                              ckptHdl, 
                                                                              nodeAddress,
                                                                              portId,
                                                                              CL_FALSE, 
                                                                              pSectionId, 
                                                                              pSec->exprTime, 
                                                                              dataSize, 
                                                                              (ClUint8T *) pData,  
                                                                              &ckptVersion, NULL, NULL);
                }
            }
            /*
             * Check the rc is unreachable and catch print warning &
             * any other return codem, log the error and continue to 
             * update on other nodes
             */
            CL_CKPT_UNREACHABEL_ERR_CHK(rc, peerAddr);
        }
    }
    return rc;

    exitOnError:
    {
        /*
         * pCallInfo should be freed incase of failure or in the Callback.
         */
        if(pCallInfo != NULL)
            clHeapFree(pCallInfo);

    }
    exitOnErrorBeforeHdlCheckout:
    return rc;
}

ClRcT
clCkptRemSvrSectionOverwrite(ClCkptHdlT        ckptHdl, 
                             CkptT             *pCkpt, 
                             ClCkptSectionIdT  *pSectionId, 
                             CkptSectionT      *pSec,
                             ClUint8T          *pData,
                             ClUint32T         dataSize,
                             ClIocNodeAddressT nodeAddress,
                             ClIocPortT        portId)
{
    return clCkptRemSvrSectionOverwriteVector(ckptHdl, pCkpt, pSectionId, pSec, pData,
                                              dataSize, nodeAddress, portId, NULL);
}

ClRcT
clCkptRemSvrSectionDelete(ClCkptHdlT        ckptHdl,
                          CkptT             *pCkpt, 
                          ClCkptSectionIdT  *pSecId)
{
    ClRcT              rc          = CL_OK;
    ClCntNodeHandleT   nodeHdl     = 0;
    ClIocNodeAddressT  peerAddr    = 0;
    ClCntKeyHandleT    dataHdl     = 0;
    CkptCbInfoT        *pCallInfo  = NULL;
    ClVersionT         ckptVersion = {0};

    /* 
     * This maks sense only in case of sync checkpoint.
     */
    if((pCkpt->pCpInfo->updateOption & CL_CKPT_WR_ALL_REPLICAS))
    {
        /* 
         * Set the defer sync handle.
         */
        pCallInfo = (CkptCbInfoT *)clHeapCalloc(1,sizeof(CkptCbInfoT));
        if(pCallInfo == NULL)
        {
            rc = CL_CKPT_ERR_NO_MEMORY;
            CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                    ("Failed to allocate the memory rc[0x %x]\n", rc), rc);
        }
        pCallInfo->delFlag   = CL_TRUE;
        pCallInfo->cbCount   = 0;
        pCallInfo->retCode   = CL_OK;
        rc = clCkptEoIdlSyncDefer((ClIdlHandleT *)&pCallInfo->rmdHdl);
        CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                ("Failed to defer the call rc[0x %x]\n", rc), rc);
    }

    /*
     * Set the supported version information.
     */
    memcpy( &ckptVersion, gCkptSvr->versionDatabase.versionsSupported,
            sizeof(ClVersionT));
    /* 
     * Walk through the checkpoint's presence list.
     * Inform all the replica nodes of the checkpoint.
     */
    clCntFirstNodeGet (pCkpt->pCpInfo->presenceList, &nodeHdl);
    while (nodeHdl != 0)
    {
        rc = clCntNodeUserKeyGet (pCkpt->pCpInfo->presenceList, nodeHdl, 
                (ClCntKeyHandleT *) &dataHdl);
        CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                ("Cant update peer rc[0x %x]\n", rc), rc);
        peerAddr = (ClIocNodeAddressT)(ClWordT)dataHdl;
        clCntNextNodeGet(pCkpt->pCpInfo->presenceList, nodeHdl, &nodeHdl);

        /*
         * Dont make the call if the presence list node is the active replica
         * itself.
         */
        if (peerAddr != gCkptSvr->localAddr)
        {
            /*
             * Allocate memory for section data to be packed and sent to 
             * replicas.
             */
            rc = ckptIdlHandleUpdate(peerAddr,gCkptSvr->ckptIdlHdl,0);
            CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                    ("Failed to update the handle rc[0x %x]\n", rc), rc);

            /*
             * Update the replica.
             */
            if((pCkpt->pCpInfo->updateOption & CL_CKPT_WR_ALL_REPLICAS ))
            {
                /*
                 * Use the defer sync concept.
                 */
                pCallInfo->cbCount++;
                rc = VDECL_VER(_ckptSectionDeleteClientAsync, 4, 0, 0)(gCkptSvr->ckptIdlHdl, 
                                        ckptHdl, CL_FALSE, pSecId, 
                                        &ckptVersion, 
                                        clCkptRemSvrSecDeleteCb,
                                        pCallInfo);
                if(rc != CL_OK)
                {
                    pCallInfo->cbCount--;
                }
            }
            else
            {
                /*
                 * Make async call.
                 */
                rc = VDECL_VER(_ckptSectionDeleteClientAsync, 4, 0, 0)(gCkptSvr->ckptIdlHdl, 
                                        ckptHdl, CL_FALSE, pSecId, 
                                        &ckptVersion, 
                                        NULL, NULL);
            }
        }
        if( CL_OK != rc )
        {
            clLogError(CL_CKPT_AREA_PEER, CL_CKPT_CTX_CKPT_OPEN,
                       "Section delete updation on remote node failed");
            goto exitOnError;
        }
    }
    return rc;
exitOnError:
    {
        /*
         * pCallInfo should be freed incase of failure or in the Callback.
         */
        if( (pCallInfo != NULL) && (pCallInfo->cbCount == 0) )
        {
            clHeapFree(pCallInfo);
        }
    }
exitOnErrorBeforeHdlCheckout:
    return rc;
}


/*
 * Function at replica nodes, that will be invoked whenever there is a 
 * change in a checkpoint's section at active server.
 */
ClRcT VDECL_VER(clCkptRemSvrSectionInfoUpdate, 4, 0, 0)(ClVersionT       *pVersion,
                                    ClHandleT        ckptHdl,
                                    CkptUpdateFlagT  updateFlag,
                                    CkptSectionInfoT *pSecInfo)
{
    ClRcT                             rc            = CL_OK;
    CkptT                             *pCkpt        = NULL;
    CkptSectionT                      *pSec         = NULL;
    ClVersionT                        ckptVersion   = {0};
    ClCkptSectionCreationAttributesT  secCreateAttr = {0};
    ClIocPhysicalAddressT             peerAddr      = {0};
    ClUint32T                         index         = 0;

    /*
     * Check whether the server is fully up or not.
     */
    CL_CKPT_SVR_EXISTENCE_CHECK;
    /*
     * Retrieve the data associated with the active handle.
     */
    if( CL_OK  != (rc = ckptSvrHdlCheckout( gCkptSvr->ckptHdl, 
                    ckptHdl, (void **)&pCkpt)))
    {
        clLogError(CL_CKPT_AREA_PEER, CL_CKPT_CTX_REPL_UPDATE, 
                "Failed to get retrieving info from CkptHdl [%#llX]",
                ckptHdl);
        return rc;      
    }
    /*
     * Lock the checkpoint's mutex.
     */
    CKPT_LOCK(pCkpt->ckptMutex);           
    /*
     * Check if racing with a delete.
     */
    if(!pCkpt->ckptMutex)
    {
        clHandleCheckin(gCkptSvr->ckptHdl, ckptHdl);
        clLogWarning(CL_CKPT_AREA_PEER, CL_CKPT_CTX_REPL_UPDATE, "Ckpt handle [%#llx] deleted", ckptHdl);
        return CL_OK;
    }
    memcpy( &ckptVersion, gCkptSvr->versionDatabase.versionsSupported,
            sizeof(ClVersionT));

    rc = clCkptSectionInfoGet(pCkpt, &pSecInfo->secId, &pSec);
    if( CL_OK != rc )
    {
        clLogWarning(CL_CKPT_AREA_PEER, CL_CKPT_CTX_REPL_UPDATE, 
                "Section [%.*s] doesn't exist with ckpt [%.*s]",
                pSecInfo->secId.idLen, pSecInfo->secId.id, pCkpt->ckptName.length, 
                pCkpt->ckptName.value);
        goto exitOnError;
    }
    rc = clRmdSourceAddressGet(&peerAddr);
    if( CL_OK != rc )
    {
        clLogWarning(CL_CKPT_AREA_PEER, CL_CKPT_CTX_REPL_UPDATE, 
                "Rmd source address get failed rc [0x %x]", rc);
        goto exitOnError;
    }
    /*
     * Allocate memory for section data to be packed and sent to 
     * replicas.
     */
    rc = ckptIdlHandleUpdate(peerAddr.nodeAddress,gCkptSvr->ckptIdlHdl,0);
    if( CL_OK != rc )
    {
        clLogWarning(CL_CKPT_AREA_PEER, CL_CKPT_CTX_REPL_UPDATE, 
                "Failed to update the handle [0x %x]", rc);
        goto exitOnError;
    }
    /* allocate memory section creation attributes */
    secCreateAttr.sectionId = clHeapCalloc(1,
            sizeof(ClCkptSectionIdT));
    if( NULL == secCreateAttr.sectionId )
    {
        clLogCritical(CL_CKPT_AREA_PEER, CL_CKPT_CTX_REPL_UPDATE, 
                "Failed to allocate memory");
        goto exitOnError;
    }
    secCreateAttr.sectionId->id = pSecInfo->secId.id; 
    secCreateAttr.sectionId->idLen = pSecInfo->secId.idLen;
    secCreateAttr.expirationTime = pSec->exprTime;
    rc = VDECL_VER(_ckptSectionCreateClientAsync, 4, 0, 0)(
            gCkptSvr->ckptIdlHdl,
            ckptHdl, 
            CL_FALSE, 
            &secCreateAttr, 
            NULL, 0, 
            &ckptVersion,
            &index,
            NULL, NULL);
    /*
     * Check the rc is unreachable and catch print warning &
     * any other return codem, log the error and continue to 
     * update on other nodes
     */
    CL_CKPT_UNREACHABEL_ERR_CHK(rc, peerAddr.nodeAddress);

    clHeapFree(secCreateAttr.sectionId);
exitOnError:
    /*
     * Unlock the checkpoint's mutex.
     */
    CKPT_UNLOCK(pCkpt->ckptMutex);           

    clHandleCheckin(gCkptSvr->ckptHdl, ckptHdl);
    return rc;
}

ClRcT
VDECL_VER(clCkptActiveCallbackNotify, 4, 0, 0)(ClCkptHdlT        ckptHdl,
                           ClIocNodeAddressT nodeAddr,
                           ClIocPortT        portId)
{
    ClRcT           rc      = CL_OK;
    CkptT           *pCkpt  = NULL;
    ClUint32T       *pData  = NULL;
    ClCkptAppInfoT  appInfo = {0};

    /*
     * Check whether the server is fully up or not.
     */
    CL_CKPT_SVR_EXISTENCE_CHECK;
    /*
     * Retrieve the data associated with the active handle.
     */
    rc = ckptSvrHdlCheckout(gCkptSvr->ckptHdl,ckptHdl,(void **)&pCkpt);
    CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_DEBUG_ERROR,
              ("Handle checkout failed for handle [%#llX] rc[0x %x]\n", 
               ckptHdl, rc), rc);
    /*
     * Lock the checkpoint's mutex.
     */
    CKPT_LOCK(pCkpt->ckptMutex);           
    if(!pCkpt->ckptMutex)
    {
        clHandleCheckin(gCkptSvr->ckptHdl, ckptHdl);
        clLogWarning(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN, "Ckpt handle [%#llx] deleted", ckptHdl);
        return CL_OK;
    }

    appInfo.nodeAddress = nodeAddr;
    appInfo.portId      = portId;

    rc = clCntDataForKeyGet(pCkpt->pCpInfo->appInfoList, (ClCntKeyHandleT)(ClWordT)
                            &appInfo, (ClCntDataHandleT *) &pData);
    if( CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST )
    {
        rc = clCkptActiveAppInfoUpdate(pCkpt, nodeAddr, portId, &pData);
    }
    if( CL_OK != rc )
    {
        clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN,
                   "While finding appInfo [%d:%d] to ckpt [%.*s] failed"
                      "rc [0x %x]", nodeAddr, portId, pCkpt->ckptName.length,
                      pCkpt->ckptName.value, rc);
        goto exitOnError;
    }
    /* Saying callback is there, so have to be notified */
    *pData = 1;
    clLogDebug(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN,
               "Callback info updated on server [%d] for ckptName[%.*s]", 
               gCkptSvr->localAddr, pCkpt->ckptName.length,
               pCkpt->ckptName.value);
exitOnError:
    /*
     * Unlock the checkpoint's mutex.
     */
    CKPT_UNLOCK(pCkpt->ckptMutex);           
    
    clHandleCheckin(gCkptSvr->ckptHdl,ckptHdl);
exitOnErrorBeforeHdlCheckout:
    return rc;

}

