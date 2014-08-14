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
 * File        : clCkptSaf.c
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
*
*     This file contains Checkpoint service APIs implementation
*
*
*****************************************************************************/
#include <string.h>  /* For memcmp */
#include <netinet/in.h> 
#include <clCommon.h>
#include <clDebugApi.h>
#include <clHandleApi.h>
#include <clVersion.h>
#include <ipi/clHandleIpi.h>
#include <clCkptApi.h>
#include <clIocErrors.h>
#include <clCkptExtApi.h>
#include <clCkptUtils.h>
#include <clCkptErrors.h>
#include "clCkptIpi.h"
#include <clCkptCommon.h>
#include <clCkptSvr.h>
#include <clCkptPeer.h>
#include <clCkptLog.h>
#include <ckptEoServer.h>
#include <ckptEockptServerCliServerFuncServer.h>
#include <ckptEockptServerExtCliServerFuncClient.h>
#include <ckptEockptServerActivePeerClient.h>
#include <ckptEockptServerPeerPeerClient.h>
#include <ckptClntEockptClntckptClntClient.h>
#include <xdrClCkptAppInfoT.h>

extern CkptSvrCbT  *gCkptSvr;

#define VERSIONED_CKPT_INFO_SYNC_CLIENT_ASYNC(idlHdl, pVersion, ckptHdl, pCkptName, pCkptInfo, callback, pCookie, rc) do { \
        rc = VDECL_VER(clCkptRemSvrCkptInfoSyncClientAsync, 5, 0, 0)((idlHdl), (pVersion), (ckptHdl), (pCkptName), \
                                                                     (pCkptInfo)->pCpInfo, (pCkptInfo)->pDpInfo, \
                                                                     VDECL_VER(callback, 5, 0, 0), (pCookie)); \
        if(CL_RMD_VERSION_ERROR(rc))                                    \
        {                                                               \
            CkptInfoT __ckptInfoBase = {0};                             \
            ckptInfoVersionConvertBackward(&__ckptInfoBase, (pCkptInfo)); \
            rc = VDECL_VER(clCkptRemSvrCkptInfoSyncClientAsync, 4, 0, 0)((idlHdl), (pVersion), (ckptHdl), (pCkptName), \
                                                                         __ckptInfoBase.pCpInfo, __ckptInfoBase.pDpInfo, \
                                                                         VDECL_VER(callback, 4, 0, 0), (pCookie)); \
            if(__ckptInfoBase.pCpInfo) clHeapFree(__ckptInfoBase.pCpInfo); \
            if(__ckptInfoBase.pDpInfo) clHeapFree(__ckptInfoBase.pDpInfo); \
        }                                                               \
}while(0)

#define VERSIONED_CKPT_INFO_SYNC_CLIENT_ASYNC_NOCB(idlHdl, pVersion, ckptHdl, pCkptName, pCkptInfo, callback, pCookie, rc) \
do {                                                                    \
    rc = VDECL_VER(clCkptRemSvrCkptInfoSyncClientAsync, 5, 0, 0)((idlHdl), (pVersion), (ckptHdl), (pCkptName), \
                                                                 (pCkptInfo)->pCpInfo, (pCkptInfo)->pDpInfo, NULL, NULL); \
    if(CL_RMD_VERSION_ERROR(rc))                                        \
    {                                                                   \
        CkptInfoT __ckptInfoBase = {0};                                 \
        ckptInfoVersionConvertBackward(&__ckptInfoBase, (pCkptInfo));   \
        rc = VDECL_VER(clCkptRemSvrCkptInfoSyncClientAsync, 4, 0, 0)((idlHdl), (pVersion), (ckptHdl), (pCkptName), \
                                                                     __ckptInfoBase.pCpInfo, \
                                                                     __ckptInfoBase.pDpInfo, NULL, NULL); \
        if(__ckptInfoBase.pCpInfo) clHeapFree(__ckptInfoBase.pCpInfo);  \
        if(__ckptInfoBase.pDpInfo) clHeapFree(__ckptInfoBase.pDpInfo);  \
    }                                                                   \
}while(0)

//static ClRcT _ckptUpdateClientImmConsmptn(ClNameT *pName);

/*************************************************************************/
/*           S A F  Related Core functions                               */
/*************************************************************************/


/*
 * Core functionality for creating a checkpoint on active server.
 */
ClRcT _ckptLocalDataUpdate(ClCkptHdlT         ckptHdl,
                           ClNameT            *pName,
      ClCkptCheckpointCreationAttributesT     *pCreateAttr,
                           ClUint32T          cksum,
                           ClIocNodeAddressT  appAddr,
                           ClIocPortT         appPort)
{
    CkptT                *pCkpt    = NULL;
    ClRcT                 rc       = CL_OK;
    ClVersionT            version  = {0};
    ClCkptSectionCreationAttributesT pSectionAttr={NULL,0};
    ClCkptHdlT           *pCkptHdl = NULL;
    ClUint32T            *pData    = NULL;
    ClUint32T             index    = 0;

    CKPT_DEBUG_T(("CkptName: %s, MastHdl: %#llX\n", pName->value, ckptHdl));
    
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
     * Lock the active server database.
     */
    CKPT_LOCK(gCkptSvr->ckptActiveSem);        

    /*
     * masterHdl is also treated as active handle.
     * Add the active handle to the list of handles maintained by the
     * active server. Associated with this handle will be the ckpt 
     * information. All section/ckpt usgae/management calls are made
     * using this handle.
     */
    (void)clCntNonUniqueKeyDelete(gCkptSvr->ckptHdlList, (ClCntKeyHandleT)(ClWordT)cksum,
                                  (ClPtrT)pName, ckptHdlNonUniqueKeyCompare);

    rc = clCntNodeAdd( gCkptSvr->ckptHdlList,(ClCntKeyHandleT)(ClWordT)cksum,
            pCkptHdl,NULL); 
            
    /*
     * Unlock the active server database.
     */
    CKPT_UNLOCK( gCkptSvr->ckptActiveSem );           
    
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("CheckpointInfo Updation Failed rc[0x %x]\n",
                 rc));
        return rc;            
    }

    /*
     * active handle id is same as masterHdl. So create it using
     * clHandleCreateSpecifiedHandle.
     */
    rc = clHandleCreateSpecifiedHandle(gCkptSvr->ckptHdl,
                                      sizeof(CkptT), ckptHdl);
    if( (CL_OK != rc) && 
         (CL_GET_ERROR_CODE(rc) != CL_ERR_ALREADY_EXIST)) 
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("CheckpointHandle Create failed rc[0x %x]\n",
                 rc));
        return rc;      
    }

    /*
     * Get the memory associated with the active handle.
     */
    rc = ckptSvrHdlCheckout(gCkptSvr->ckptHdl, ckptHdl,
                            (void **)&pCkpt);      
    CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_DEBUG_ERROR, 
         ("Checkout failed for handle [%#llX] checkpoint %.*s rc[0x %x]\n",
          ckptHdl, pName->length, pName->value,rc),rc);

    /*
     * Create and lock the mutex to protect the specific checkpoint.
     */
    clOsalMutexCreate(&pCkpt->ckptMutex);         
    CKPT_LOCK(pCkpt->ckptMutex);           
    if(!pCkpt->ckptMutex)
    {
        clHandleCheckin(gCkptSvr->ckptHdl, ckptHdl);
        clLogWarning(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN, "Ckpt with handle [%#llx] already delete", 
                     ckptHdl);
        return CL_OK;
    }
    /*
     * Allocate memory for Control plane and Dataplane.
     */
    rc = _ckptDplaneInfoAlloc(&pCkpt->pDpInfo, pCreateAttr->maxSections); 
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR, 
          ("Failed to allocate memory for checkpoint %s rc[0x %x]\n",
           pCkpt->ckptName.value,rc),rc);
    rc = _ckptCplaneInfoAlloc(&pCkpt->pCpInfo); 
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR, 
          ("Failed to allocate memory for checkpoint %s rc[0x %x]\n",
           pCkpt->ckptName.value,rc),rc);

    clNameCopy(&pCkpt->ckptName, pName);
    
    /*
     * Copy Data plane Information. 
     */
    pCkpt->pDpInfo->maxCkptSize  = pCreateAttr->checkpointSize;
    pCkpt->pDpInfo->maxScns      = pCreateAttr->maxSections;
    pCkpt->pDpInfo->numScns      = 0;
    pCkpt->pDpInfo->maxScnSize   = pCreateAttr->maxSectionSize;
    pCkpt->pDpInfo->maxScnIdSize = pCreateAttr->maxSectionIdSize;

    /*
     * Set checkpoint id.
     */
    ckptIDSet(pCkpt);
    /*
     * Copy Control plane Information. 
     */
    pCkpt->pCpInfo->updateOption = pCreateAttr->creationFlags;

    /* Num of mutex need to be created */
    pCkpt->numMutex = CL_MIN(pCkpt->pDpInfo->maxScns, 
                            (ClUint32T)(sizeof(pCkpt->secMutex) / sizeof(pCkpt->secMutex[0])) );
    if(pCreateAttr->maxSections == 1)
    {
        /*
         * Create the default section
         */
        memcpy(&version, gCkptSvr->versionDatabase.versionsSupported,
               sizeof(ClVersionT));
        CKPT_UNLOCK( pCkpt->ckptMutex );
        rc =  VDECL_VER(_ckptSectionCreate, 4, 0, 0)(ckptHdl, CL_FALSE, &pSectionAttr, (ClUint8T *)"",
                                 0, &version, &index);
        CKPT_LOCK( pCkpt->ckptMutex );                        
        CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR, 
                ("CkptCreation failed for %s with rc[0x %x]\n",
                 pName->value,rc),rc);
    }                            
    else /* not having default sections */
    {
        /* 
         * If default section is not there, Create pool of mutexs for section level lock 
         */
        rc = clCkptSectionLevelMutexCreate(pCkpt->secMutex, pCkpt->numMutex);
        if( CL_OK != rc )
        {
            clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN, 
                    "While creation mutexs for sections failed rc[0x %x]", rc);
            goto exitOnError;
        }
    }

    /*
     * Update application info 
     */
     rc = clCkptActiveAppInfoUpdate(pCkpt, appAddr, appPort, &pData);
     if( CL_OK != rc )
     {
         clLogWarning(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN,
           "Immediate consumption for checkpoints wont work rc[0x %x]"
           "for this ckptHdl [%#llX]", rc, ckptHdl);
     }
    /*
     * Add yourself to the presence list of the checkpoint.
     */
    rc = _ckptPresenceListUpdate(pCkpt,gCkptSvr->localAddr);
    
    clHandleCheckin(gCkptSvr->ckptHdl,ckptHdl);

    /*
     * Unlock the checkpoint's mutex.
     */
    CKPT_UNLOCK(pCkpt->ckptMutex);
    return rc;
exitOnError:
    /*
     * cleaning up the resources.
     */
    clHandleCheckin(gCkptSvr->ckptHdl,ckptHdl);
    
    /*
     * Unlock and delete the checkpoint's mutex.
     */
    CKPT_UNLOCK(pCkpt->ckptMutex);           
    clOsalMutexDelete(pCkpt->ckptMutex);           
    pCkpt->ckptMutex = CL_HANDLE_INVALID_VALUE;
exitOnErrorBeforeHdlCheckout:
    CKPT_LOCK( gCkptSvr->ckptActiveSem);
    /*
     * Failed to replicate the checkpoint info . Hence delete the
     * handle entry form the ckpt handle list.
     */
    (void)clCntNonUniqueKeyDelete(gCkptSvr->ckptHdlList, (ClPtrT)(ClWordT)cksum,
                                  (ClPtrT)pName, ckptHdlNonUniqueKeyCompare);
    
    /*
     * Unlock the active server's DB.
     */
    CKPT_UNLOCK( gCkptSvr->ckptActiveSem);
    return rc;
}  	 



/*
 * Active server side functionality related to checkpoint open.
 */
 
ClRcT  VDECL_VER(clCkptActiveCkptOpen, 4, 0, 0)(ClVersionT        *pVersion,
                            ClCkptHdlT        ckptMastHdl,
             		    ClNameT           *pName,
                            ClCkptOpenFlagsT  checkpointOpenFlags,
                            ClCkptCheckpointCreationAttributesT *pCreateAttr,
                            ClIocNodeAddressT appAddr, 
                            ClIocPortT        appPort)
{
    ClRcT        rc      = CL_OK;   
    ClUint32T    cksum   = 0;

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
    
    memset(pVersion,'\0',sizeof(ClVersionT));
    
    /*
     * Create the checkpoint is checkpoint is being opened in create mode.
     */
    if (checkpointOpenFlags & CL_CKPT_CHECKPOINT_CREATE)
    {
        clCksm32bitCompute((ClUint8T *)pName->value,
                           pName->length, &cksum);
                            
        rc = _ckptLocalDataUpdate(ckptMastHdl,pName,pCreateAttr,
                cksum, appAddr, appPort);
        CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                ("Checkpoint %s create get failed rc[0x %x]\n",
                 pName->value,rc), rc);
    }
    clLogWrite(CL_LOG_HANDLE_APP,CL_LOG_INFORMATIONAL,CL_LOG_CKPT_SVR_NAME,
            CL_CKPT_LOG_1_CKPT_CREATED, pName->value);
exitOnError:
    {
        return rc;
    }
}



/*
 * Removes the checkpoint from the system if no other execution entity
 * has opened it.
 */
ClRcT  VDECL_VER(clCkptActiveCkptDelete, 4, 0, 0)(ClVersionT     version,
                              ClHandleT      ckptHdl)
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
    rc = clVersionVerify(&gCkptSvr->versionDatabase,&version);
    if( CL_OK != rc )
    {
        clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN, 
                "Version mismatch during ckpt delete rc[0x %x]", rc);
        return rc;
    }

    CKPT_LOCK(gCkptSvr->ckptActiveSem);        

    /*
     * Retrieve the data associated with the active handle.
     */
    rc = ckptSvrHdlCheckout(gCkptSvr->ckptHdl,ckptHdl,(void **)&pCkpt);
    if( CL_OK != rc )
    {
        CKPT_UNLOCK(gCkptSvr->ckptActiveSem);
        clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN, 
                "Failed to checkout handle [%#llX] rc[0x %x]", ckptHdl, rc);
        return rc;
    }
    rc = clCkptSvrReplicaDelete(pCkpt, ckptHdl, CL_TRUE);

    clHandleCheckin(gCkptSvr->ckptHdl,ckptHdl);

    CKPT_UNLOCK(gCkptSvr->ckptActiveSem);

    return rc;
}


/*
 * Function to make the given node as active replica. If prevActiveAddr 
 * exists, then it pulls checkpoint info from it as well
 */

ClRcT VDECL_VER(clCkptActiveAddrSet, 4, 0, 0)(ClVersionT        version,
                          ClHandleT         ckptActHdl,
                          ClIocNodeAddressT prevActiveAddr)
{                           
    ClRcT      rc        = CL_OK;
    CkptInfoT  ckptInfoBase  = {0}; 
    VDECL_VER(CkptInfoT, 5, 0, 0) ckptInfo = {0};
    ClBoolT scaleDown = CL_FALSE;
    /*
     * Check whether the server is fully up or not.
     */
    CL_CKPT_SVR_EXISTENCE_CHECK;

    /*
     * Verify the version.
     */
    rc = clVersionVerify(&gCkptSvr->versionDatabase,&version);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Ckpt: VersionMismatch rc[0x %x]\n", rc), rc);
    rc = ckptIdlHandleUpdate(prevActiveAddr,gCkptSvr->ckptIdlHdl,0);       
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Ckpt: Idl Handle updation failed rc[0x %x]\n", rc), rc);

    /*
     * Pull the checkpoint info from the previous active.
     */
    rc = VDECL_VER(clCkptRemSvrCkptInfoGetClientSync, 5, 0, 0)(gCkptSvr->ckptIdlHdl,
            &version,
            ckptActHdl,
            gCkptSvr->localAddr,
            &ckptInfo);
    if(CL_RMD_VERSION_ERROR(rc))
    {
        /*
         * Fire a second query with the minimum rmd version.
         */
        scaleDown = CL_TRUE;
        rc = VDECL_VER(clCkptRemSvrCkptInfoGetClientSync, 4, 0, 0)(gCkptSvr->ckptIdlHdl,
                                                                   &version,
                                                                   ckptActHdl,
                                                                   gCkptSvr->localAddr,
                                                                   &ckptInfoBase);
    }
    if( CL_OK != rc )
    {
        clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_REPL_UPDATE, 
                "Checkpoint info get from server [%d] failed for checkpoint handle [%#llx] error [%#x]",
                prevActiveAddr, ckptActHdl, rc); 
        goto exitOnError;
    }
    if(scaleDown)
    {
        ckptInfo.pCpInfo = NULL;
        ckptInfo.pDpInfo = NULL;
        ckptInfoVersionConvertForward(&ckptInfo, &ckptInfoBase);
    }
    /* 
     * Unpack the received info.
     */
    rc = _ckptReplicaInfoUpdate(ckptActHdl, &ckptInfo);
    if( CL_OK != rc )
    {
        clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN, 
                "Updating the active server database failed handle [%#llX]",
                ckptActHdl);
        goto ckptInfoFreeNExit;
    }
    ckptPackInfoFree( &ckptInfo );
    return CL_OK;
ckptInfoFreeNExit:    
    ckptPackInfoFree( &ckptInfo );
exitOnError:
    return rc;
}


/*
 * Server side implementation of the _ckptCheckpointStatusGet call.
 */
 
ClRcT VDECL_VER(_ckptCheckpointStatusGet, 4, 0, 0)(ClCkptHdlT                  ckptHdl,
                               ClCkptCheckpointDescriptorT *pCheckpointStatus,
                               ClVersionT                  *pVersion)
{
    CkptT             *pCkpt     = NULL;
    ClUint32T         memUsed    = 0;
    ClRcT             rc         = CL_OK;
//    CkptSectionT      *pSec      = NULL;
    ClTimeT           cltime     = {0};
    ClIocNodeAddressT actAddr    = 0;  /* Needed only to contact master */
    ClUint32T         refCount   = 0;  /* Needed only to contact master */
    ClUint8T          delFlag    = 0;  /* Needed only to contact master */
    
    /*
     * Check whether the server is fully up or not.
     */
    CL_CKPT_SVR_EXISTENCE_CHECK;
    
    /*
     * Verify the input parameters.
     */
    if (!pCheckpointStatus)
        rc = CL_CKPT_ERR_NULL_POINTER;
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("StatusDescriptor is NULL Pointer rc[0x %x]",rc), rc);

    /*
     * Verify the version.
     */
    rc = clVersionVerify(&gCkptSvr->versionDatabase,
                         (ClVersionT *)pVersion);
    if( CL_GET_ERROR_CODE(rc) == CL_ERR_VERSION_MISMATCH)
    {
        return CL_OK;
    }
    
    memset(pVersion, '\0', sizeof(ClVersionT));
    
    CKPT_ERR_CHECK(CL_CKPT_SVR, CL_DEBUG_ERROR,
            ("Ckpt: Version b/w C/S is incompatablie rc[0x %x]",rc),
             rc);

    /*
     * Retrieve the data associated with the active handle.
     */
    rc = clHandleCheckout(gCkptSvr->ckptHdl, ckptHdl, 
                          (void **)&pCkpt);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Failed to get ckpt from handle rc[0x %x]\n",rc), rc);

    CL_ASSERT(pCkpt != NULL);

    if(pCkpt->isPending == CL_TRUE)
    {
        rc = CL_CKPT_RC(CL_ERR_TRY_AGAIN);
        CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                    ("_ckptSectionCreate failed rc[0x %x]\n",
                     rc), rc);
    }
    
    memset(pCheckpointStatus, '\0', sizeof(ClCkptCheckpointDescriptorT));

    /*
     * Lock the checkpoint's mutex.
     */
    CKPT_LOCK(pCkpt->ckptMutex);           
    if(!pCkpt->ckptMutex)
    {
        clHandleCheckin(gCkptSvr->ckptHdl, ckptHdl);
        clLogWarning("STATUS", "GET", "Ckpt with handle [%#llx] already delete", ckptHdl);
        return CL_CKPT_ERR_NOT_EXIST;
    }
    
    /* 
     * Read the control and data plane information and copy the desired 
     * information into the user provided output buffer.
     */
    if(pCkpt != NULL)
    {    
      if(pCkpt->pCpInfo != NULL)
      {
          /*
           * Contro plane info.
           */
          pCheckpointStatus->checkpointCreationAttributes.creationFlags  =
              pCkpt->pCpInfo->updateOption;
      }
      if(pCkpt->pDpInfo != NULL)
      {
          /*
           * Data plane info.
           */
          pCheckpointStatus->checkpointCreationAttributes.checkpointSize    =
              pCkpt->pDpInfo->maxCkptSize;
          pCheckpointStatus->checkpointCreationAttributes.maxSections       =
              pCkpt->pDpInfo->maxScns;
          pCheckpointStatus->checkpointCreationAttributes.maxSectionSize    =
              pCkpt->pDpInfo->maxScnSize;
          pCheckpointStatus->checkpointCreationAttributes.maxSectionIdSize  =
              pCkpt->pDpInfo->maxScnIdSize;
          pCheckpointStatus->numberOfSections                               =
              pCkpt->pDpInfo->numScns;
              
          /*
           * Calculate memory usage.Walk thru the section table and find out
           * the memory usage.
           *FIXME
           */
#if 0
          if(pCkpt->pDpInfo->pSections != NULL)
          {
              pSec = pCkpt->pDpInfo->pSections;
              for(secCount = 0; secCount < pCkpt->pDpInfo->maxScns; 
                  secCount++)
              {
                 if(pSec->used == CL_TRUE)
                 {
                   memUsed = memUsed +(ClUint32T)pSec->size;
                 }  
                 pSec++;
              }   
          }    
#endif
          pCheckpointStatus->memoryUsed        = memUsed;
       }
    } 

    /*
     * Get retention duration from master.
     */
    rc = ckptIdlHandleUpdate(gCkptSvr->masterInfo.masterAddr, 
                             gCkptSvr->ckptIdlHdl,0);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Cant update the idl handle rc[0x %x]\n", rc), rc);

    rc = VDECL_VER(clCkptMasterStatusInfoGetClientSync, 4, 0, 0)(gCkptSvr->ckptIdlHdl, ckptHdl, 
                                   &cltime, &actAddr, 
                                   &refCount, &delFlag);
    if(rc == CL_OK)
    {
        pCheckpointStatus->checkpointCreationAttributes.retentionDuration 
                = cltime;
    }
                                     
    /*
     * Unlock the checkpoint's mutex.
     */
    CKPT_UNLOCK(pCkpt->ckptMutex);           
    
    rc = clHandleCheckin(gCkptSvr->ckptHdl,ckptHdl);
exitOnError:
    {
        return rc;
    }
}

ClRcT
clCkptSectionTimeUpdate(ClTimeT   *pLastUpdate)
{
    ClRcT            rc          =  CL_OK;
    ClTimerTimeOutT  currentTime = {0,0};

    rc = clOsalTimeOfDayGet(&currentTime); 
    if( CL_OK != rc )
    {
        clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN, 
                   "Failed to get the time of the day rc [0x %x]\n", rc);
        return rc;         
    }
    /* Convert the current time into NanoSeconds. */
    *pLastUpdate = currentTime.tsSec * CL_CKPT_NANOS_IN_SEC;
    *pLastUpdate += currentTime.tsMilliSec * CL_CKPT_NANOS_IN_MILI;

    return rc;
}

ClRcT
clCkptSectionChkNAdd(ClCkptHdlT  ckptHdl,
             CkptT               *pCkpt, 
             ClCkptSectionIdT    *pSectionId, 
             ClTimeT             expiryTime,
             ClSizeT             initialDataSize,
             ClUint8T            *pInitialData)
{
    ClRcT              rc       = CL_OK;   
    ClCkptSectionKeyT  *pKey    = NULL;
    CkptSectionT       *pSec    = NULL;
    ClUint32T          cksum    = 0;
    ClCntNodeHandleT   hNodeSec = CL_HANDLE_INVALID_VALUE;
    ClTimerTimeOutT    timeOut  = {0};

    pKey = clHeapCalloc(1, sizeof(ClCkptSectionKeyT));
    if( NULL == pKey )
    {
        rc = CL_CKPT_ERR_NO_MEMORY;
        clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN, 
                "Allocation failed during section creation");
        goto exitOnError;
    }
    pSec = clHeapCalloc(1, sizeof(CkptSectionT));
    if( NULL == pSec )
    {
        rc = CL_CKPT_ERR_NO_MEMORY;
        clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN, 
                "Allocation failed during section creation");
        goto keyFreeNExit;
    }
    /* copy the key */   
    pKey->scnId.id    = pSectionId->id; 
    pKey->scnId.idLen = pSectionId->idLen;
    rc = clCksm32bitCompute(pKey->scnId.id, pKey->scnId.idLen, &cksum);
    if( CL_OK != rc )
    {
        clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN, 
                "Cksum computation failed rc [0x %x]", rc);
        goto dataFreeNExit;
    }
    pKey->hash = cksum % pCkpt->pDpInfo->numOfBukts;
    clLogTrace(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN, 
               "Hash value of section [%.*s] is %d with idLen %d", 
               pKey->scnId.idLen, pKey->scnId.id, pKey->hash, 
               pKey->scnId.idLen);

    /* Find the section already exist or not */
    rc = clCntNodeFind(pCkpt->pDpInfo->secHashTbl, (ClCntKeyHandleT) pKey,
                       &hNodeSec);
    if( CL_OK == rc )
    {
        /* Section already exists -- this is an expected result for this fn
           since is function means "check and add if it does not exist".
           Therefore the log level is "debug".
         */
        rc = CL_CKPT_ERR_ALREADY_EXIST;
        clLogDebug(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN, 
                "Section [%.*s] of ckpt [%.*s] already exists", 
                pCkpt->ckptName.length, pCkpt->ckptName.value, 
                pKey->scnId.idLen, pKey->scnId.id);
        goto dataFreeNExit;
    }
    if( pCkpt->pDpInfo->maxScns <= pCkpt->pDpInfo->numScns )
    {
        rc = CL_CKPT_ERR_NO_SPACE;
        clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN, 
                   "While creating section [%.*s], numSections [%d]"
                   " are exceeding maximum sections [%d]",
                   pKey->scnId.idLen, pKey->scnId.id, pCkpt->pDpInfo->numScns, 
                   pCkpt->pDpInfo->maxScns);
        goto dataFreeNExit;
    }
    /* Section doesnot exist, need to create */
    pSec->pData = clHeapCalloc(initialDataSize, sizeof(ClUint8T));
    if( NULL == pSec->pData )
    {
        rc = CL_CKPT_ERR_NO_MEMORY;
        clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN,
                "Allocation failed while creating section");
        goto dataFreeNExit;
    }
    memcpy(pSec->pData, pInitialData, initialDataSize);
    pSec->size     = initialDataSize;
    pSec->state    = CL_CKPT_SECTION_VALID;
    pSec->exprTime = expiryTime;   
    pSec->timerHdl = 0;
    
    pKey->scnId.id = clHeapCalloc(1, pKey->scnId.idLen);
    if( NULL == pKey->scnId.id ) 
    {
        rc = CL_CKPT_ERR_NO_MEMORY;
        clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN,
                "Allocation failed while creating section");
        goto dataFreeNExit;
    }
    memcpy(pKey->scnId.id, pSectionId->id, pKey->scnId.idLen);

    /* Add the section to the checkpoint */
    rc = clCntNodeAdd(pCkpt->pDpInfo->secHashTbl, (ClCntKeyHandleT) pKey, 
            (ClCntDataHandleT) pSec, NULL);
    if( CL_OK != rc )
    {
        clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN, 
                "Failed to add the section [%.*s]", pKey->scnId.idLen,
                pKey->scnId.id);
        goto dataFreeNExit;
    }

    /* 
     * Here after need to delete the node which will internally delete the
     * data pointer & key pointer 
     */
    if(pSec->exprTime != CL_TIME_END)
    {
        rc = _ckptSectionExpiryTimeoutGet(
                    pSec->exprTime, &timeOut);
        if( CL_OK != rc )
        {
            clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN,
                    "Expiry timeout get failed rc [0x %x]",
                    rc);
            /* this will internally delete the pointers */
            clCntAllNodesForKeyDelete(pCkpt->pDpInfo->secHashTbl, (ClCntKeyHandleT) pKey);
            return rc;
        }
        /*
         * Create timer for each section.
         */
        rc = _ckptExpiryTimerStart(ckptHdl, &pKey->scnId, pSec, timeOut);
        if( CL_OK != rc )
        {
            clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN,
                    "Failed to start timer for section [%.*s] of ckpt[%.*s]"
                    "with rc [0x %x]", pSectionId->idLen, pSectionId->id, 
                    pCkpt->ckptName.length, pCkpt->ckptName.value, rc);
            /* this will internally delete the pointers */
            clCntAllNodesForKeyDelete(pCkpt->pDpInfo->secHashTbl, (ClCntKeyHandleT) pKey);
            return rc;
        }
    }  
    /*
     * Update lastUpdate field of section, not checking error as this is not as important for
     * Section creation.
     */
     clCkptSectionTimeUpdate(&pSec->lastUpdated);
    
     
    clLogInfo(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN, 
              "Section [%.*s] of [%.*s] created successfully",
               pSectionId->idLen, pSectionId->id, pCkpt->ckptName.length,
               pCkpt->ckptName.value);
    return CL_OK;
dataFreeNExit:
    if( NULL != pSec->pData ) clHeapFree(pSec->pData); pSec->pData = NULL;
    clHeapFree(pSec);
keyFreeNExit:
    clHeapFree(pKey);
exitOnError:
    return rc;
}

ClRcT
clCkptDefaultSectionAdd(CkptT      *pCkpt,
                        ClUint32T  initialDataSize,
                        ClUint8T   *pInitialData)
{
    ClRcT              rc    = CL_OK;
    ClCkptSectionKeyT  *pKey = NULL;
    CkptSectionT       *pSec = NULL;

    /* copy the key */
    pKey = clHeapCalloc(1, sizeof(ClCkptSectionKeyT));
    if( NULL == pKey )
    {
        clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN,
                "Failed to allocate memory");
        return CL_CKPT_ERR_NO_MEMORY;
    }
    pKey->scnId.id = clHeapCalloc(1, strlen("defaultSection") + 1);
    if( NULL == pKey->scnId.id )
    {
        clHeapFree(pKey);
        clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN,
                "Failed to allocate memory");
        return CL_CKPT_ERR_NO_MEMORY;
    }
    pKey->scnId.idLen = strlen("defaultSection") + 1;
    strncpy((ClCharT *) pKey->scnId.id, "defaultSection", pKey->scnId.idLen);

    pKey->hash = 0;
    /* copy the data */
    pSec = clHeapCalloc(1, sizeof(CkptSectionT));
    if( NULL == pSec )
    {
        clHeapFree(pKey->scnId.id);
        clHeapFree(pKey);
        clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN,
                "Failed to allocate memory");
        return CL_CKPT_ERR_NO_MEMORY;
    }

    pSec->size  = initialDataSize;
    pSec->pData = (ClAddrT *)clHeapCalloc(pSec->size,
            sizeof(ClCharT));
    if( NULL == pSec->pData )
    {
        clHeapFree(pKey->scnId.id);
        clHeapFree(pKey);
        clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN,
                "Failed to allocate memory");
        return CL_CKPT_ERR_NO_MEMORY;
    }
    memcpy(pSec->pData,pInitialData,pSec->size);
    pSec->exprTime = CL_TIME_END; 
    pSec->state    = CL_CKPT_SECTION_VALID;
    pSec->timerHdl = 0;

    rc = clCntNodeAdd(pCkpt->pDpInfo->secHashTbl, (ClCntKeyHandleT) pKey,
                      (ClCntDataHandleT) pSec, NULL);
    if( CL_OK != rc )
    {
        if( CL_GET_ERROR_CODE(rc) == CL_ERR_DUPLICATE )
        {
            clLogWarning(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN,
                         "Default section alreay exist rc[0x %x]", rc);
        }
        else
        {
            clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN,
                "Creating default section failed rc [0x %x]", rc);
        }
        clHeapFree(pSec->pData);
        clHeapFree(pSec);
        clHeapFree(pKey->scnId.id);
        clHeapFree(pKey);
        return rc;
    }
    /*
     * Update lastUpdate field of section, not checking error as this is not as important for
     * Section creation.
     */
     clCkptSectionTimeUpdate(&pSec->lastUpdated);

    clLogInfo(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN,
              "Created default section for ckpt [%.*s]",
              pCkpt->ckptName.length, pCkpt->ckptName.value);

    return CL_OK;
}

/*
 * Server side implementation of section creation.
 */
ClRcT VDECL_VER(_ckptSectionCreate, 4, 0, 0)(ClCkptHdlT         ckptHdl,
        ClBoolT                             srcClient, 
        ClCkptSectionCreationAttributesT    *pSecCreateAttr,
        ClUint8T                            *pInitialData,
        ClSizeT                             initialDataSize,
        ClVersionT                          *pVersion,
        ClUint32T                           *index)
{
    CkptT      *pCkpt     = NULL; 
    ClRcT      rc         = CL_OK; 
    ClUint32T  peerCount  = 0;

    /*
     * Check whether the server is fully up or not.
     */
    CL_CKPT_SVR_EXISTENCE_CHECK;

    if( pSecCreateAttr->sectionId != NULL )
    {
        clLogDebug(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_SEC_CREATE, 
               "Creating section [%.*s] at address [%d] ...", 
                pSecCreateAttr->sectionId->idLen,
                pSecCreateAttr->sectionId->id, srcClient 
                );
    }
            
    /*
     * Verify the version.
     */
    if( CL_OK != (rc = clVersionVerify(&gCkptSvr->versionDatabase,
                    pVersion)))
    {
        return CL_OK;
    }

    /*
     * Retrieve the data associated with the active handle.
     */
    if( CL_OK  != (rc = ckptSvrHdlCheckout( gCkptSvr->ckptHdl, 
                    ckptHdl, (void **)&pCkpt)))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("Checkpoint Handle Create failed rc[0x %x]\n"
                 ,rc));
        return rc;      
    }

    /*
     * Lock the checkpoint's mutex.
     */
    CKPT_LOCK(pCkpt->ckptMutex);           
    if(!pCkpt->ckptMutex)
    {
        clHandleCheckin(gCkptSvr->ckptHdl, ckptHdl);
        clLogWarning("SEC", "CREATE", "Ckpt handle [%#llx] already deleted", ckptHdl);
        memset(pVersion,0,sizeof(ClVersionT));
        return CL_CKPT_ERR_NOT_EXIST;
    }

    if(pCkpt->isPending == CL_TRUE)
    {
        rc = CL_CKPT_RC(CL_ERR_TRY_AGAIN);
        CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                ("_ckptSectionCreate failed rc[0x %x]\n",
                 rc), rc);
    }

    /*
     * Verify the sanity of the checkpoint data.
     */
    if (pCkpt->pDpInfo == NULL)
        rc = CL_CKPT_ERR_INVALID_STATE;
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR, 
            ("Dataplane info is absent for %s rc[0x %x]\n",
             pCkpt->ckptName.value,rc), rc);

    /*
     * Return ERROR if passed sectionId length is > max sectionId length.
     */
    if( pSecCreateAttr->sectionId != NULL)
    {
        if (pSecCreateAttr->sectionId->idLen > pCkpt->pDpInfo->maxScnIdSize)
            rc = CL_CKPT_ERR_INVALID_PARAMETER;
        CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                ("Section Id size [%d] is greater than maximum "
                 "Section Id size [%llu] Checkpoint [%s]. rc[0x %x]\n", pSecCreateAttr->sectionId->idLen, pCkpt->pDpInfo->maxScnIdSize, pCkpt->ckptName.value,rc), rc);
    }

    /*
     * Return ERROR if passed data size is > max data size.
     */
    if ((pCkpt->pDpInfo->maxScnSize != 0) && (initialDataSize > pCkpt->pDpInfo->maxScnSize) )
    {
        rc = CL_CKPT_ERR_INVALID_PARAMETER;
        CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                ("Section size is greater than maximum size %s rc[0x %x]\n",
                 pCkpt->ckptName.value,rc), rc);
    }

    /* 
     * Default section related operation.
     */
    if( pSecCreateAttr->sectionId == NULL)
    {
        if(pCkpt->pDpInfo->maxScns == 1) 
        {
            /*
             * sectionId == NULL and maxSection == 1 means defaultsection.
             */
            rc = clCkptDefaultSectionAdd(pCkpt, initialDataSize,
                    pInitialData);

            /*
             * Unlock the checkpoint's mutex.
             */
            CKPT_UNLOCK(pCkpt->ckptMutex);           

            rc = clHandleCheckin(gCkptSvr->ckptHdl,ckptHdl);
            return rc; 
        }
    }   
    else if( pSecCreateAttr->sectionId->id == NULL )
    {
        /* 
         * This is CL_CKPT_GENERATED SECTION_ID concept.
         */
        if (index == NULL)
        {
            rc = CL_CKPT_ERR_NULL_POINTER;
            CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                    ("index parameter is NULL for a generated section."),rc);
        }
        pSecCreateAttr->sectionId->id =
            clHeapCalloc(CL_CKPT_GEN_SEC_LENGTH, sizeof(ClCharT));
        if( NULL == pSecCreateAttr->sectionId->id )
        {
            clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN, 
                    "Failed to allocate memory for generated section");
            goto exitOnError;
        }
        snprintf((ClCharT *) pSecCreateAttr->sectionId->id, CL_CKPT_GEN_SEC_LENGTH,"generatedSection%d", 
                pCkpt->pDpInfo->indexGenScns);
        pSecCreateAttr->sectionId->idLen =
            strlen((ClCharT *) pSecCreateAttr->sectionId->id) + 1;
        
        /* Return the index to the client based on which we can regenerate
         * the sectionID in client side and return to application */
        *index = pCkpt->pDpInfo->indexGenScns;
        pCkpt->pDpInfo->indexGenScns++;
    }
    else
    {
        /* 
         * Overwriting of default section is not allowed. Return ERROR.
         */
        if ( pCkpt->pDpInfo->maxScns == 1 )
        {
            rc = CL_CKPT_ERR_ALREADY_EXIST;
            CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_TRACE,
                    ("Default Section is already exist %s rc[0x %x]\n",
                     pCkpt->ckptName.value,rc), rc);
        }
    }
    /* 
     * Verify that the section does not exist. Dont check this in case of
     * GENERATED SECTION.
     */
    rc = clCkptSectionChkNAdd(ckptHdl, pCkpt, pSecCreateAttr->sectionId,
                              pSecCreateAttr->expirationTime, initialDataSize,
                              pInitialData);
    if( CL_OK != rc )
    {
        goto exitOnError;
    }
    /*
     * Increment the section count.
     */
    pCkpt->pDpInfo->numScns++;

    /*
     * Inform the replica nodes as well. Presence list 
     * contains active replica address and other replicas.
     * Hence peerCount > 1 means that the checkpoint is 
     * replicated on other nodes.
     */
    if( CL_TRUE == srcClient )
    {
        clCntSizeGet(pCkpt->pCpInfo->presenceList, &peerCount);
        if(peerCount > 1)
        {
            /* 
             * Inform replica nodes.
             */
            rc = clCkptRemSvrSectionCreate(ckptHdl, pCkpt, pSecCreateAttr, 
                                           initialDataSize, pInitialData);
            if( CL_OK != rc )
            {
               /* FIXME, if it is SYNC ckpt, revert back the changes. */ 
                clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN, 
                           "Replication of section over write failed rc [0x %x]", rc);
            }
        }   
    }
exitOnError:
    if( pSecCreateAttr->sectionId != NULL)
    {
        clHeapFree(pSecCreateAttr->sectionId->id);
    }
    {
        /*
         * Unlock the checkpoint's mutex.
         */
        CKPT_UNLOCK(pCkpt->ckptMutex);           

        clHandleCheckin(gCkptSvr->ckptHdl,ckptHdl);
        memset(pVersion,'\0',sizeof(ClVersionT));
        return rc;
    }
}

ClRcT
clCkptSecFindNDelete(CkptT             *pCkpt, 
                     ClCkptSectionIdT  *pSecId)
{
    ClRcT              rc    = CL_OK;
    ClCkptSectionKeyT  *pKey = NULL;
    ClUint32T          cksum = 0;

    pKey = clHeapCalloc(1, sizeof(ClCkptSectionKeyT));
    if( NULL == pKey )
    {
        clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN,
                   "Failed to allocate memory while finding section [%.*s]", 
                   pSecId->idLen, pSecId->id);
        return CL_CKPT_ERR_NO_MEMORY;
    }
    pKey->scnId.id    = pSecId->id;
    pKey->scnId.idLen = pSecId->idLen;
    rc = clCksm32bitCompute(pKey->scnId.id, pKey->scnId.idLen, &cksum);
    if( CL_OK != rc )
    {
        clHeapFree(pKey);
        clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN, 
                   "Failed to find cksum while finding section [%.*s]",
                   pKey->scnId.idLen, pKey->scnId.id);
        return rc;
    }
    pKey->hash        = cksum % pCkpt->pDpInfo->numOfBukts;

    /* Delete the section */
    rc = clCntAllNodesForKeyDelete(pCkpt->pDpInfo->secHashTbl, 
                                   (ClCntDataHandleT) pKey);
    if( CL_OK != rc )
    {
        clHeapFree(pKey);
        clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN, 
                   "Failed to delete section [%.*s] rc [0x %x]",
                   pKey->scnId.idLen, pKey->scnId.id, rc);
        if( CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST )
        {
            rc = CL_CKPT_ERR_NOT_EXIST;
        }
        return rc;
    }

    clHeapFree(pKey);
    return rc;
}
/*
 * Deletes a section in the given checkpoint
 */
ClRcT VDECL_VER(_ckptSectionDelete, 4, 0, 0)( ClCkptHdlT 	    ckptHdl,
                          ClBoolT           srcClient, 
                    	  ClCkptSectionIdT  *pSecId,
                          ClVersionT        *pVersion)
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
    rc = clVersionVerify(&gCkptSvr->versionDatabase,(ClVersionT *)pVersion);
    if( CL_GET_ERROR_CODE(rc) == CL_ERR_VERSION_MISMATCH)
    {
        return CL_OK;
    }
    memset(pVersion,'\0',sizeof(ClVersionT));
        
    /*
     * Retrieve the data associated with the active handle.
     */
    rc = clHandleCheckout(gCkptSvr->ckptHdl,ckptHdl,(void **)&pCkpt);  
    CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Failed to get ckpt from handle rc[0x %x]\n",rc), rc);

    CL_ASSERT(pCkpt != NULL);
    /* Take the Ckpt Mutex */
    CKPT_LOCK(pCkpt->ckptMutex);
    if(!pCkpt->ckptMutex)
    {
        goto exitOnError;
    }

    if(pCkpt->isPending == CL_TRUE)
    {
        rc = CL_CKPT_RC(CL_ERR_TRY_AGAIN);
        clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN, 
                "Active server doesn't have the proper data");
        goto exitOnErrorUnlock;
    }
    rc = clCkptSectionLevelDelete(ckptHdl, pCkpt, pSecId, srcClient);
    if( CL_OK != rc )
    {
        clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN, 
                "Failed to delete the section [%.*s]", 
                pSecId->idLen, pSecId->id);
    }
    exitOnErrorUnlock:
    CKPT_UNLOCK(pCkpt->ckptMutex);

exitOnError:
    {
        clHandleCheckin(gCkptSvr->ckptHdl,ckptHdl);
    }
exitOnErrorBeforeHdlCheckout:
    {
        return rc;
    }
}

/*
 * Updates the section expiration time locally.
 */

ClRcT _ckptLocalSectionExpirationTimeSet(ClVersionT        *pVersion,
                                         ClCkptHdlT        ckptHdl,
                                         ClCkptSectionIdT  *pSectionId,
                                         ClTimeT           exprTime)
{
    ClTimerTimeOutT    timeOut        = {0} ;
    CkptT              *pCkpt         = NULL; 
    ClRcT              rc             = CL_OK;
    CkptSectionT       *pSec          = NULL;

    /*
     * Check whether the server is fully up or not.
     */
    CL_CKPT_SVR_EXISTENCE_CHECK;

    /*
     * Verify the version.
     */
    rc = clVersionVerify(&gCkptSvr->versionDatabase,(ClVersionT *)pVersion);
    if( CL_GET_ERROR_CODE(rc) == CL_ERR_VERSION_MISMATCH)
    {
     return CL_OK;
    }
    memset(pVersion,'\0',sizeof(ClVersionT));

    /*
     * Retrieve the data associated with the active handle.
     */
    rc = ckptSvrHdlCheckout(gCkptSvr->ckptHdl, ckptHdl, (void **)&pCkpt);
    CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_DEBUG_ERROR,
         ("Handle [%#llX] checkout failed rc[0x %x]\n", ckptHdl, rc), rc);
    
    /*
     * Lock the checkpoint's mutex.
     */
    CKPT_LOCK(pCkpt->ckptMutex);           
    if(!pCkpt->ckptMutex)
    {
        clHandleCheckin(gCkptSvr->ckptHdl, ckptHdl);
        clLogWarning("EXP", "SET", "Ckpt with handle [%#llx] already deleted", ckptHdl);
        return CL_OK;
    }
    /* 
     * Verify the sanity of the data length.
     */
    if (pCkpt->pDpInfo == NULL) rc = CL_CKPT_ERR_INVALID_STATE;
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Dataplane info is absent for %s rc[0x %x]\n",
             pCkpt->ckptName.value,rc), rc);

    /*
     * Return ERROR is section is is default section.
     */
    if(pCkpt->pDpInfo->maxScns == 1)
    {
        rc = CL_CKPT_ERR_INVALID_PARAMETER;
    }
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Cannot update exp time if DEFAULT section rc[0x %x]\n",
             rc), rc);
    /* 
     * Verify that the section not exist.
     */
    rc = clCkptSectionInfoGet(pCkpt, pSectionId, &pSec);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Section does not exist %s rc[0x %x]\n",
             pCkpt->ckptName.value,rc), rc);

    /* 
     * Free the existing data and attach the new data.
     */
    if(pSec->exprTime != CL_TIME_END && exprTime == CL_TIME_END)
    {
        /*
         * CL_TIME_END. Hence delete the running timer.
         */
        rc = clTimerDelete(&pSec->timerHdl);
        pSec->timerHdl = 0;
    }

    pSec->exprTime     = exprTime;

    /*
     * If not CL_TIME_END start the timer.
     */
    if(pSec->exprTime != CL_TIME_END)
    {
        rc = _ckptSectionExpiryTimeoutGet(pSec->exprTime,
                                          &timeOut);
        /*
         * If timer not running previously, create the timer . Else
         * update the timer.
         */
        if( !pSec->timerHdl )
        {
            rc = _ckptExpiryTimerStart(ckptHdl, pSectionId, pSec, timeOut);
        }
        else
        {
            rc = clTimerUpdate(pSec->timerHdl,timeOut);        
        }    
    }
exitOnError:
    /*
     * Unlock the checkpoint's mutex.
     */
    CKPT_UNLOCK(pCkpt->ckptMutex);           
    
    clHandleCheckin(gCkptSvr->ckptHdl, ckptHdl); 
exitOnErrorBeforeHdlCheckout:
   return rc;
}


/*
 * Server side implementation of  expiration time set.
 */
 
ClRcT VDECL_VER(_ckptSectionExpirationTimeSet, 4, 0, 0)(ClCkptHdlT        ckptHdl,
                                    ClCkptSectionIdT  *pSectionId,
                                    ClTimeT           exprTime,
                                    ClVersionT        *pVersion)
                                    
{
    CkptT              *pCkpt      = NULL; 
    ClRcT              rc          = CL_OK;
    ClCntNodeHandleT   nodeHdl     = 0;
    ClCntDataHandleT   dataHdl     = 0;
    ClIocNodeAddressT  peerAddr    = 0;
    ClVersionT         ckptVersion = {0};
    
    /*
     * Set the new value locally.
     */
    rc =  _ckptLocalSectionExpirationTimeSet(pVersion, ckptHdl, 
                                             pSectionId, exprTime);
    CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_DEBUG_ERROR,
         ("Failed to update the expiration timr, rc[0x %x]\n",rc), rc);
    
    /*
     * Retrieve the data associated with the active handle.
     */
    rc = ckptSvrHdlCheckout(gCkptSvr->ckptHdl, ckptHdl, (void **)&pCkpt);
    CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_DEBUG_ERROR,
       ("Failed to retrieve data associated with handle rc[0x %x]\n",rc), rc);

    /*
     * Lock the checkpoint's mutex.
     */
    CKPT_LOCK(pCkpt->ckptMutex);           
    if(!pCkpt->ckptMutex)
    {
        clHandleCheckin(gCkptSvr->ckptHdl, ckptHdl);
        clLogWarning("EXP", "SET", "Ckpt with handle [%#llx] already deleted", ckptHdl);
        return CL_OK;
    }

    /*
     * Walk through the presence list and update all the replica
     * nodes present in the checkpoint's presence list.
     */
    clCntFirstNodeGet (pCkpt->pCpInfo->presenceList, &nodeHdl);
    while (nodeHdl != 0)
    {
        rc = clCntNodeUserKeyGet (pCkpt->pCpInfo->presenceList, nodeHdl, &dataHdl);
        CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR, 
                ("Cant update peer rc[0x %x]\n", rc), rc);
        peerAddr = (ClIocNodeAddressT)(ClWordT) dataHdl;
        clCntNextNodeGet(pCkpt->pCpInfo->presenceList, nodeHdl, &nodeHdl);
        
        /*
         * Send the RMD to the replica node.
         */
        if (peerAddr != gCkptSvr->localAddr)
        {
            rc = ckptIdlHandleUpdate(peerAddr,gCkptSvr->ckptIdlHdl,0);
            CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
               ("Cant update the idl handle rc[0x %x]\n", rc), rc);

            memcpy(&ckptVersion, gCkptSvr->versionDatabase.versionsSupported,
                   sizeof(ClVersionT));

            rc = VDECL_VER(clCkptRemSvrSectionExpTimeSetClientAsync, 4, 0, 0)(gCkptSvr->ckptIdlHdl,
                                                          &ckptVersion,
                                                          ckptHdl, 
                                                          pSectionId,
                                                          exprTime, NULL, 0);
            CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                  ("Cant update peer rc[0x %x]\n", rc), rc);
        }
    }

exitOnError:
    /*
     * Unlock the checkpoint's mutex.
     */
    CKPT_UNLOCK(pCkpt->ckptMutex);           
    
    clHandleCheckin(gCkptSvr->ckptHdl, ckptHdl); 
exitOnErrorBeforeHdlCheckout:
   return rc;
}



/*
 * Copy the section Id related info.
 */
 
ClRcT _ckptSectionCopy(ClCkptSectionKeyT  *pKey,
                       ClCkptSectionIdT   *pSecId)
{
    ClRcT    rc = CL_OK;
    
    pSecId->idLen = pKey->scnId.idLen;
    pSecId->id    = (ClUint8T *)clHeapCalloc(1, pKey->scnId.idLen);
    if((pSecId)->id == NULL)
    {
        return CL_CKPT_ERR_NO_MEMORY;
    }
    memcpy( (pSecId)->id, pKey->scnId.id, pKey->scnId.idLen);
    return rc;
}



/*
 * Enable the application to iterate thru sections in a checkpoint
 */
 
ClRcT 
VDECL_VER(clCkptSvrIterationInitialize, 4, 0, 0)(ClVersionT        *pVersion, 
                             ClCkptHdlT        ckptHdl,
                             ClInt32T          sectionsChosen,
                             ClTimeT           exprTime,
                             ClUint32T         *pSecCount,
                             ClCkptSectionIdT  **pSecId)
{
    CkptT             *pCkpt    = NULL;   
    ClRcT             rc        = CL_OK; 
    CkptSectionT      *pSec     = NULL;
    ClCkptSectionIdT  *pTempSec = NULL;
    ClCntNodeHandleT  secNode   = CL_HANDLE_INVALID_VALUE;
    ClCntKeyHandleT   nextNode  = CL_HANDLE_INVALID_VALUE;
    ClCkptSectionKeyT *pKey     = NULL;

    *pSecId    = NULL;
    *pSecCount = 0;
    /* 
     * Verify the server's existence.
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
    memset(pVersion, '\0', sizeof(ClVersionT));

    /*
     * Retrieve the data associated with the active handle.
     */
    rc = ckptSvrHdlCheckout(gCkptSvr->ckptHdl,ckptHdl,(void **)&pCkpt);  
    CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Failed to get ckpt from handle rc[0x %x]\n",rc), rc);

    CL_ASSERT(pCkpt != NULL);

    if(pCkpt->isPending == CL_TRUE)
    {
        rc = CL_CKPT_RC(CL_ERR_TRY_AGAIN);
        clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN, 
                "Active server doesn't have the proper data");
        clHandleCheckin(gCkptSvr->ckptHdl, ckptHdl);
        return rc;
    }
    /*
     * Lock the checkpoint's mutex.
     */
    CKPT_LOCK(pCkpt->ckptMutex);           
    if(!pCkpt->ckptMutex)
    {
        clHandleCheckin(gCkptSvr->ckptHdl, ckptHdl);
        clLogWarning(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN, "Ckpt handle [%#llx] already got deleted", ckptHdl);
        return CL_CKPT_ERR_NOT_EXIST;
    }

    /*
     * Copy the no. of sections and allocate memory for max no. of sections.
     */
    rc = clCntSizeGet(pCkpt->pDpInfo->secHashTbl, pSecCount);
    if( CL_OK != rc )
    {
        clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN,
                "Container size get failed rc[0x %x]", rc);
        goto exitOnError;
    }
    if(*pSecCount != pCkpt->pDpInfo->numScns)
    {
        rc = CL_CKPT_ERR_OP_NOT_PERMITTED;
        clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN, 
                   "CKPT section has only the default section which cannot be iterated");
        goto exitOnError;
    }
    pTempSec   = (ClCkptSectionIdT *)clHeapCalloc(1,
            (*pSecCount) * sizeof(ClCkptSectionIdT));
    if( pTempSec == NULL )
    {
        rc = CL_CKPT_ERR_NO_MEMORY;
        CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                ("Failed to allocate memory %s rc[0x %x]\n",
                 pCkpt->ckptName.value,rc), rc);
    }
    *pSecId  = pTempSec;

    /*
     * Iterate through the used sections of the checkpoint.
     */
    rc = clCntFirstNodeGet(pCkpt->pDpInfo->secHashTbl, &secNode);
    if( CL_OK != rc )
    {
        rc = CL_OK;
        clHeapFree(pTempSec);
        *pSecCount = 0;
        *pSecId = NULL;
        clLogWarning(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN, 
                  "Sections have to be created to iterate for ckpt [%.*s]",
                  pCkpt->ckptName.length, pCkpt->ckptName.value);
        goto exitOnError;
    }
    while( secNode != 0 )
    {
        nextNode = 0;
        rc = clCntNodeUserKeyGet(pCkpt->pDpInfo->secHashTbl, secNode, 
                (ClCntKeyHandleT *) &pKey);
        if( CL_OK != rc )
        {
            break;
        }
        rc = clCntNodeUserDataGet(pCkpt->pDpInfo->secHashTbl, secNode, 
                (ClCntDataHandleT *) &pSec);
        if( CL_OK != rc )
        {
            break;
        }
        if( NULL == pSec ) 
        {
            clLogWarning(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN, 
                    "Section data doesn't exist, continuing...");
            break;
        }
        /*
         * Take appropriate actions based on the type of operation.
         */
        switch(sectionsChosen)
        { 
            case CL_CKPT_SECTIONS_FOREVER:
                /*
                 * Sections with infinite expiry time.
                 */
                if(pSec->exprTime == CL_TIME_END)
                {
                    rc = _ckptSectionCopy(pKey, pTempSec);
                    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                            ("_ckptSectionCopy failed rc[0x %x]\n",
                             rc), rc);
                    pTempSec++;        
                }
                break;
            case CL_CKPT_SECTIONS_LEQ_EXPIRATION_TIME:
                /*
                 * Sections with expiry time < passed value.
                 */
                if(pSec->exprTime <= exprTime)
                { 
                    rc = _ckptSectionCopy(pKey, pTempSec);
                    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                            ("_ckptSectionCopy failed rc[0x %x]\n",
                             rc), rc);
                    (pTempSec)++;        
                }
                break;
            case CL_CKPT_SECTIONS_GEQ_EXPIRATION_TIME:
                /*
                 * Sections with expiry time >= passed value.
                 */
                if(pSec->exprTime >= exprTime)
                { 
                    rc = _ckptSectionCopy(pKey, pTempSec);
                    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                            ("_ckptSectionCopy failed rc[0x %x]\n",
                             rc), rc);
                    (pTempSec)++;        
                }
                break;
            case CL_CKPT_SECTIONS_CORRUPTED:
                /*
                 * Corrupted sections.
                 */
                if(pSec->state == CL_CKPT_SECTION_CORRUPTED)
                { 
                    rc = _ckptSectionCopy(pKey, pTempSec);
                    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                            ("_ckptSectionCopy failed rc[0x %x]\n",
                             rc), rc);
                    (pTempSec)++;        
                }
                break;
            case CL_CKPT_SECTIONS_ANY:
                /*
                 * All Sections.
                 */
                {
                    rc = _ckptSectionCopy(pKey, pTempSec);
                    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                            ("_ckptSectionCopy failed rc[0x %x]\n",
                             rc), rc);
                    (pTempSec)++;        
                }
                break;
            default:
                /*
                 * Invalid sections chosen.
                 */
                rc = CL_CKPT_ERR_INVALID_PARAMETER;
                break; 
        }
        rc = clCntNextNodeGet(pCkpt->pDpInfo->secHashTbl, secNode, &nextNode);
        if( CL_OK != rc )
        {
            rc = CL_OK;
            break;
        }
        secNode = nextNode;
    }
    if( CL_OK != rc )
    {
        clHeapFree(pTempSec);
        clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN, 
                "While iterating sections of ckpt [%.*s] failed with rc [0x%x]",
                pCkpt->ckptName.length, pCkpt->ckptName.value, rc);
        goto exitOnError;
    }
exitOnError:
    /*
     * Unlock the checkpoint's mutex.
     */
    CKPT_UNLOCK(pCkpt->ckptMutex);           

    clHandleCheckin(gCkptSvr->ckptHdl,ckptHdl);  
exitOnErrorBeforeHdlCheckout:
    return rc;
}

/*
 * Gets the next section in the list of sections matching the 
 * ckptSectionIterationInitialize call.
 */
ClRcT VDECL_VER(_ckptIterationNextGet, 4, 0, 0)(ClCkptHdlT               ckptHdl,
                                                ClCkptSectionIdT         *pSecId, 
                                                ClCkptSectionDescriptorT *pSecDescriptor,
                                                ClVersionT               *pVersion)
{
    CkptT         *pCkpt = NULL; 
    ClRcT         rc     = CL_OK; 
    CkptSectionT  *pSec  = NULL;

    /* 
     * Verify the server's existence.
     */
    CL_CKPT_SVR_EXISTENCE_CHECK;

    /*
     * Verify the version.
     */
    rc = clVersionVerify(&gCkptSvr->versionDatabase,(ClVersionT *)pVersion);
    if( CL_GET_ERROR_CODE(rc) == CL_ERR_VERSION_MISMATCH)
    {
        return CL_OK;
    }
    memset(pVersion,'\0',sizeof(ClVersionT));

    /*
     * Retrieve the data associated with the active handle.
     */
    rc = ckptSvrHdlCheckout(gCkptSvr->ckptHdl,ckptHdl,(void **)&pCkpt);  
    if(rc != CL_OK)
    {
        clLogError("ITER", "NEXT", "Failed to get ckpt from handle rc[0x %x]", rc);
        return rc;
    }

    CKPT_LOCK(pCkpt->ckptMutex);
    if(!pCkpt->ckptMutex)
    {
        clLogWarning("ITER", "NEXT", "Ckpt handle [%#llx] deleted", ckptHdl);
        rc = CL_CKPT_ERR_NOT_EXIST;
        goto exitOnErrorWithoutUnlock;
    }

    /* 
     * Verify the sanity of the data plane info.
     */
    if (pCkpt->pDpInfo == NULL) rc = CL_CKPT_ERR_INVALID_STATE;
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                   ("Dataplane info is absent for %s rc[0x %x]\n",
                    pCkpt->ckptName.value,rc), rc);

    /*
     * Get the pointer to the section.
     */
    if(pSecId == NULL)
    {
        rc = clCkptDefaultSectionInfoGet(pCkpt, &pSec);
        if ( CL_OK != rc )
        {
            clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN,
                       "Failed to get default section info. rc 0x%x",rc);
            goto exitOnError;
        }
    }
    else
    {
        rc = clCkptSectionInfoGet(pCkpt, pSecId, &pSec);
        if( CL_OK != rc )
        {
            clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN, 
                       "Section [%.*s] does not exist in ckpt [%.*s]", 
                       pSecId->idLen, pSecId->id,
                       pCkpt->ckptName.length, pCkpt->ckptName.value);
            goto exitOnError;
        }
    }        

    /*
     * Get the data.
     */
    if(pSecDescriptor != NULL && pSec != NULL)
    {
        if (pSecId != NULL)
        {
            pSecDescriptor->sectionId.idLen = pSecId->idLen;
        } 
        else 
        {
            pSecDescriptor->sectionId.idLen = strlen("defaultSection") + 1;
        }
        
        pSecDescriptor->sectionId.id = (ClUint8T *)clHeapCalloc(
                                                                1, pSecDescriptor->sectionId.idLen ?
                                                                pSecDescriptor->sectionId.idLen : 1);
        if(pSecDescriptor->sectionId.id == NULL)
        {
            rc = CL_CKPT_ERR_NO_MEMORY;
            CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                           ("Memory allocation failed %s rc[0x %x]\n",
                            pCkpt->ckptName.value,rc), rc);
        }

        if (pSecId != NULL)
        {
            if(pSecId->idLen != 0)
                memcpy(pSecDescriptor->sectionId.id, pSecId->id,
                       pSecId->idLen);
        }
        else 
        {
            strncpy((ClCharT*)pSecDescriptor->sectionId.id, "defaultSection", pSecDescriptor->sectionId.idLen);
        }

        pSecDescriptor->expirationTime = pSec->exprTime;
        pSecDescriptor->sectionSize    = pSec->size;
        pSecDescriptor->sectionState   = pSec->state;
        pSecDescriptor->lastUpdate     = pSec->lastUpdated;
    } 
 
    exitOnError:
    CKPT_UNLOCK(pCkpt->ckptMutex);

    exitOnErrorWithoutUnlock:
    clHandleCheckin(gCkptSvr->ckptHdl, ckptHdl);

    return rc;
}


/*
 *  Frees resources associated with the iteration identified by 
 *  sectionIterationHandle.
 */
ClRcT _ckptSectionIterationFinalize(ClHandleT sectionIterationHandle)
{
    return CL_OK;
}


static void  _ckptRemSvrWriteVectorCallback(ClHandleT              ckptIdlHdl,
                                            ClVersionT             *pVersion,
                                            ClCkptHdlT             ckptHdl,
                                            ClIocNodeAddressT      nodeAddr,
                                            ClIocPortT             portId,
                                            ClUint32T              numberOfElements,
                                            ClCkptIOVectorElementT *pIoVector,
                                            ClCkptDifferenceIOVectorElementT *pDifferenceIoVector,
                                            ClRcT                  retCode,
                                            void                   *pData)
{
    ClIocAddressT           srcAddr     = {{0}};
    ClRcT                   rc          = CL_OK;
    ClVersionT              ckptVersion = {0}; 
    CkptCbInfoT             *pCbInfo    = (CkptCbInfoT *)pData;
    ClUint32T               count       = 0; 
    CkptT *pCkpt = NULL;

    rc = clHandleCheckout(gCkptSvr->ckptHdl, ckptHdl, (void **)&pCkpt);   
    if(rc != CL_OK)
    {
        clLogError("WRITE", "CB", "Handle checkout for ckpt handle [%llx] returned [%#x]",
                   ckptHdl, rc);
        retCode = rc;
        goto send_resp;
    }
    CKPT_LOCK(pCkpt->ckptMutex);
    if(!pCkpt->ckptMutex)
    {
        clHandleCheckin(gCkptSvr->ckptHdl, ckptHdl);
        clLogWarning("SEC", "OVERWRITE", "Ckpt handle [%#llx] already deleted", ckptHdl);
        pCkpt = NULL;
    }
    /*
     * Decrement the callback count associated with checkpoint write
     * If the count reaches 0, inform the client about SUCCESS
     * or FAILURE.
     */
    (pCbInfo->cbCount)--;
    if((retCode != CL_OK) || pCbInfo->cbCount == 0)
    {
        pCbInfo->cbCount = 0;
        if(CL_GET_ERROR_CODE(retCode) == CL_ERR_VERSION_MISMATCH)
        {
            memcpy(&ckptVersion,pVersion,sizeof(ClVersionT));
            rc = clRmdSourceAddressGet(&srcAddr.iocPhyAddress);
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL,
                       CL_CKPT_LOG_6_VERSION_NACK, "CkptRemSvrAdd",
                       srcAddr.iocPhyAddress.nodeAddress,
                       srcAddr.iocPhyAddress.portId, pVersion->releaseCode,
                       pVersion->majorVersion, pVersion->minorVersion);

            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Ckpt nack recieved from "
                                            "NODE[0x%x:0x%x], rc=[0x%x]\n", 
                                            srcAddr.iocPhyAddress.nodeAddress,
                                            srcAddr.iocPhyAddress.portId,
                                            CL_CKPT_ERR_VERSION_MISMATCH));
        }
        send_resp:
        rc = VDECL_VER(_ckptCheckpointWriteResponseSend, 4, 0, 0)( pCbInfo->rmdHdl,
                                                                   retCode,
                                                                   0,ckptVersion);
        clHeapFree(pCbInfo); 
    }
    if(pCkpt)
    {
        CKPT_UNLOCK(pCkpt->ckptMutex);
    }
    if(pIoVector)
    {
        for( count = 0; count < numberOfElements; ++count )
        {
            clHeapFree(pIoVector[count].sectionId.id);
            clHeapFree(pIoVector[count].dataBuffer);
        }
        clHeapFree(pIoVector);
    }
    else if(pDifferenceIoVector)
    {
        clCkptDifferenceIOVectorFree(pDifferenceIoVector, numberOfElements);
        clHeapFree(pDifferenceIoVector);
    }
    return;
}

/*
 * Defer sync funda for writing  multiple sections on to a given checkpoint.
 */
static void ckptRemSvrWriteCallback(ClHandleT              ckptIdlHdl,
                                     ClVersionT             *pVersion,
                                     ClCkptHdlT             ckptHdl,
                                     ClIocNodeAddressT      nodeAddr,
                                     ClIocPortT             portId,
                                     ClUint32T              numberOfElements,
                                     ClCkptIOVectorElementT *pIoVector,
                                     ClRcT                  retCode,
                                     void                   *pData)
{
    _ckptRemSvrWriteVectorCallback(ckptIdlHdl, pVersion, ckptHdl, nodeAddr, portId, numberOfElements,
                                   pIoVector, NULL, retCode, pData);
}

static void  ckptRemSvrWriteVectorCallback(ClHandleT              ckptIdlHdl,
                                           ClVersionT             *pVersion,
                                           ClCkptHdlT             ckptHdl,
                                           ClIocNodeAddressT      nodeAddr,
                                           ClIocPortT             portId,
                                           ClUint32T              numberOfElements,
                                           ClCkptDifferenceIOVectorElementT *pDifferenceIoVector,
                                           ClRcT                  retCode,
                                           void                   *pData)
{
    _ckptRemSvrWriteVectorCallback(ckptIdlHdl, pVersion, ckptHdl, nodeAddr, portId, numberOfElements,
                                   NULL, pDifferenceIoVector, retCode, pData);
}

ClRcT VDECL_VER(_ckptCheckpointWriteVector, 4, 0, 0)(ClCkptHdlT             ckptHdl,
                                                     ClIocNodeAddressT      nodeAddr,
                                                     ClIocPortT             portId,
                                                     ClUint32T              numberOfElements,
                                                     ClCkptDifferenceIOVectorElementT *pDifferenceIoVector,
                                                     ClUint32T              *pError,
                                                     ClVersionT             *pVersion)
{
    ClUint32T               count        = 0;
    ClRcT                   rc           = CL_OK;
    CkptT                   *pCkpt       = NULL;
    ClUint32T               peerCount    = 0;
    ClIdlHandleT            idlDeferHdl  = 0;
    CkptCbInfoT             *pCallInfo   = NULL;
    ClCntNodeHandleT        nodeHdl      = 0;
    ClCntDataHandleT        dataHdl      = 0;
    ClIocNodeAddressT       peerAddr     = 0;
    ClVersionT              ckptVersion  = {0};
    ClCkptDifferenceIOVectorElementT *pDifferenceReplicaVector = NULL;
    ClBoolT replicate = CL_FALSE;
    ClPtrT *pStaleSectionsData = NULL;
    /*
     * Check whether the server is fully up or not.
     */
    if (gCkptSvr == NULL || gCkptSvr->serverUp == CL_FALSE) 
    {
        rc = CL_CKPT_ERR_TRY_AGAIN;
        goto out_free;
    }
    
    /*
     * Verify the version.
     */
    rc = clVersionVerify(&gCkptSvr->versionDatabase,(ClVersionT *)pVersion);
    if( CL_GET_ERROR_CODE(rc) == CL_ERR_VERSION_MISMATCH)
    {
        rc = CL_OK;
        goto out_free;
    }
    
    memset(pVersion,'\0',sizeof(ClVersionT));
        
    /*
     * Retrieve the data associated with the active handle.
     */
    rc = clHandleCheckout(gCkptSvr->ckptHdl,ckptHdl,(void **)&pCkpt);
    CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                                  ("Handle [%#llX] checkout failed during checkpoint write rc[0x %x]\n", ckptHdl, rc), rc);
    CL_ASSERT(pCkpt != NULL);

    if(pCkpt->isPending == CL_TRUE)
    {
        rc = CL_CKPT_RC(CL_ERR_TRY_AGAIN);
        clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN, 
                   "Checkpoint server doesn't have proper data for this handle [%#llX]", 
                   ckptHdl);
        goto exitOnErrorWithoutUnlock;
    }

    pStaleSectionsData = clHeapCalloc(numberOfElements, sizeof(*pStaleSectionsData));
    CL_ASSERT(pStaleSectionsData != NULL);
    /*
     * Validate the input difference iovector and copy in to the replica difference vector.
     */
    pDifferenceReplicaVector = clHeapCalloc(numberOfElements, sizeof(*pDifferenceReplicaVector));
    CL_ASSERT(pDifferenceReplicaVector != NULL);
    for(count = 0; count < numberOfElements; ++count)
    {
        memcpy(pDifferenceReplicaVector+count, pDifferenceIoVector+count, sizeof(*pDifferenceReplicaVector));
        pDifferenceReplicaVector[count].differenceVector = clHeapCalloc(1, sizeof(*pDifferenceReplicaVector[count].differenceVector));
        CL_ASSERT(pDifferenceReplicaVector[count].differenceVector != NULL);
        clDifferenceVectorCopy(pDifferenceReplicaVector[count].differenceVector,
                               pDifferenceIoVector[count].differenceVector);
    }

    /*
     * Lock the checkpoint's mutex.
     */
    CKPT_LOCK(pCkpt->ckptMutex);           
    if(!pCkpt->ckptMutex)
    {
        clLogWarning("SEC", "WRITE", "Ckpt with handle [%#llx] already deleted", ckptHdl);
        rc = CL_CKPT_ERR_NOT_EXIST;
        goto exitOnErrorWithoutUnlock;
    }
    rc = _ckptCheckpointLocalWriteVector(ckptHdl,
                                         numberOfElements,
                                         pDifferenceIoVector,pError,
                                         nodeAddr, portId,
                                         pDifferenceReplicaVector,
                                         &replicate,
                                         pStaleSectionsData);
    if( CL_GET_ERROR_CODE(rc) == CL_ERR_INVALID_STATE )
    {
        /*
         * Handle checkout failed at that function which didn't release the
         * mutex so release and exit 
         */
        clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_SEC_OVERWRITE,
                   "Failed to write on the local copy rc [0x %x]" ,rc);
        goto exitOnError;
    }
    if( CL_OK != rc )
    {
        /* any other error, this would have unheld the lock there so just go
         * out, return from here 
         */
        goto exitOnErrorWithoutUnlock;
    }
    
    /*
     * Inform the replica nodes as well. Presence list 
     * contains active replica address and other replicas.
     * Hence peerCount > 1 means that the checkpoint is 
     * replicated on other nodes.
     */
    if(replicate)
    {
        clCntSizeGet(pCkpt->pCpInfo->presenceList,&peerCount);
        if(peerCount > 1)
        {
            if((pCkpt->pCpInfo->updateOption & CL_CKPT_WR_ALL_REPLICAS))
            {
                pCallInfo = (CkptCbInfoT *)clHeapAllocate(sizeof(CkptCbInfoT));
                if(pCallInfo == NULL)
                {
                    rc = CL_CKPT_ERR_NO_MEMORY;
                    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                                   ("Memory allocation failed %s rc[0x %x]\n",
                                    pCkpt->ckptName.value,rc), rc);
                }         
                memset(pCallInfo,'\0',sizeof(CkptCbInfoT));
                rc = clCkptEoIdlSyncDefer(&idlDeferHdl);
                CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                               ("Failed to Pack section rc[0x %x]\n", rc), rc);
                pCallInfo->rmdHdl    = idlDeferHdl;
                pCallInfo->delFlag   = CL_TRUE;
                pCallInfo->cbCount   = peerCount - 1;
            }    
            memcpy(&ckptVersion,gCkptSvr->versionDatabase.versionsSupported,
                   sizeof(ClVersionT));
            clCntFirstNodeGet (pCkpt->pCpInfo->presenceList, &nodeHdl);
            while (nodeHdl != 0)
            {
                rc = clCntNodeUserKeyGet (pCkpt->pCpInfo->presenceList, nodeHdl,
                                          &dataHdl);
                CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                               ("Cant update peer rc[0x %x]\n", rc), rc);
                peerAddr = (ClIocNodeAddressT)(ClWordT)dataHdl;
                clCntNextNodeGet(pCkpt->pCpInfo->presenceList, nodeHdl, &nodeHdl);

                rc = ckptIdlHandleUpdate(peerAddr,gCkptSvr->ckptIdlHdl,0);
                CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                               ("Failed to update the handle rc[0x %x]\n", rc), rc);

                if (peerAddr != gCkptSvr->localAddr)
                {
                    if((pCkpt->pCpInfo->updateOption & CL_CKPT_WR_ALL_REPLICAS))
                    {
                        rc = VDECL_VER(clCkptRemSvrCkptWriteVectorClientAsync, 4, 0, 0)(
                                                                                        gCkptSvr->ckptIdlHdl,
                                                                                        &ckptVersion,
                                                                                        ckptHdl,
                                                                                        nodeAddr,
                                                                                        portId,
                                                                                        numberOfElements,
                                                                                        pDifferenceReplicaVector,
                                                                                        ckptRemSvrWriteVectorCallback,
                                                                                        pCallInfo);
                        if( rc != CL_OK)
                        {
                            (pCallInfo->cbCount)--;
                        }
                    }
                    else
                    {
                        rc = VDECL_VER(clCkptRemSvrCkptWriteVectorClientAsync, 4, 0, 0)(
                                                                                        gCkptSvr->ckptIdlHdl,
                                                                                        &ckptVersion,
                                                                                        ckptHdl,
                                                                                        nodeAddr,
                                                                                        portId,
                                                                                        numberOfElements,
                                                                                        pDifferenceReplicaVector,
                                                                                        NULL,0);
                    }
                    CL_CKPT_UNREACHABEL_ERR_CHK(rc, peerAddr);
                }                           
            }
        }
    }
    clLogWrite(CL_LOG_HANDLE_APP,CL_LOG_INFORMATIONAL,CL_LOG_CKPT_SVR_NAME,
               CL_CKPT_LOG_1_CKPT_WRITTEN, pCkpt->ckptName.value);

    /*
     * Inform the clients about immediate consumption.
     */
    //_ckptUpdateClientImmConsmptn(&pCkpt->ckptName);
    exitOnError:
    {
        /*
         * Unlock the checkpoint's mutex.
         */
        CKPT_UNLOCK(pCkpt->ckptMutex);           
        
        clHandleCheckin(gCkptSvr->ckptHdl,ckptHdl);
        
        if( CL_OK != rc )
        {
            *pError = count;
        }
        goto out_free;
    }        
    exitOnErrorWithoutUnlock:
    {
        clHandleCheckin(gCkptSvr->ckptHdl,ckptHdl);
        *pError = count;
    }
    exitOnErrorBeforeHdlCheckout:

    out_free:
    if(pDifferenceReplicaVector)
    {
        for(count = 0; count < numberOfElements; ++count)
        {
            if(pDifferenceReplicaVector[count].differenceVector)
            {
                clDifferenceVectorFree(pDifferenceReplicaVector[count].differenceVector, CL_FALSE);
                clHeapFree(pDifferenceReplicaVector[count].differenceVector);
            }
        }
        clHeapFree(pDifferenceReplicaVector);
    }
    /*
     * Free stale sections data that have been extended
     */
    if(pStaleSectionsData)
    {
        for(count = 0; count < numberOfElements; ++count)
            if(pStaleSectionsData[count])
                clHeapFree(pStaleSectionsData[count]);

        clHeapFree(pStaleSectionsData);
    }
    clCkptDifferenceIOVectorFree(pDifferenceIoVector, numberOfElements);
    return rc;
}

/*
 * Server side implementation of checkpoint write functionality.
 */
ClRcT VDECL_VER(_ckptCheckpointWrite, 4, 0, 0)(ClCkptHdlT             ckptHdl,
                                               ClIocNodeAddressT      nodeAddr,
                                               ClIocPortT             portId,
                                               ClUint32T              numberOfElements,
                                               ClCkptIOVectorElementT *pIoVector,
                                               ClUint32T              *pError,
                                               ClVersionT             *pVersion)
{
    ClUint32T               count        = 0;
    ClRcT                   rc           = CL_OK;
    CkptT                   *pCkpt       = NULL;
    ClUint32T               peerCount    = 0;
    ClIdlHandleT            idlDeferHdl  = 0;
    CkptCbInfoT             *pCallInfo   = NULL;
    ClCntNodeHandleT        nodeHdl      = 0;
    ClCntDataHandleT        dataHdl      = 0;
    ClIocNodeAddressT       peerAddr     = 0;
    ClVersionT              ckptVersion  = {0};

    /*
     * Check whether the server is fully up or not.
     */
    if (gCkptSvr == NULL || gCkptSvr->serverUp == CL_FALSE) 
    {
        rc = CL_CKPT_ERR_TRY_AGAIN;
        goto out_free;
    }
    
    /*
     * Verify the version.
     */
    rc = clVersionVerify(&gCkptSvr->versionDatabase,(ClVersionT *)pVersion);
    if( CL_GET_ERROR_CODE(rc) == CL_ERR_VERSION_MISMATCH)
    {
        rc = CL_OK;
        goto out_free;
    }

    memset(pVersion,'\0',sizeof(ClVersionT));
        
    /*
     * Retrieve the data associated with the active handle.
     */
    rc = clHandleCheckout(gCkptSvr->ckptHdl,ckptHdl,(void **)&pCkpt);
    CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                                  ("Handle [%#llX] checkout failed during checkpoint write rc[0x %x]\n", ckptHdl, rc), rc);
    CL_ASSERT(pCkpt != NULL);

    if(pCkpt->isPending == CL_TRUE)
    {
        rc = CL_CKPT_RC(CL_ERR_TRY_AGAIN);
        clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN, 
                   "Checkpoint server doesn't have proper data for this handle [%#llX]", 
                   ckptHdl);
        goto exitOnErrorWithoutUnlock;
    }
    
    /*
     * Lock the checkpoint's mutex.
     */
    CKPT_LOCK(pCkpt->ckptMutex);           
    if(!pCkpt->ckptMutex)
    {
        clLogWarning("SEC", "WRITE", "Ckpt with handle [%#llx] already deleted", ckptHdl);
        rc = CL_CKPT_ERR_NOT_EXIST;
        goto exitOnErrorWithoutUnlock;
    }
    rc = _ckptCheckpointLocalWrite(ckptHdl,
                                   numberOfElements,
                                   pIoVector,pError,
                                   nodeAddr, portId);
    if( CL_GET_ERROR_CODE(rc) == CL_ERR_INVALID_STATE )
    {
        /*
         * Handle checkout failed at that function which didn't release the
         * mutex so release and exit 
         */
        clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_SEC_OVERWRITE,
                   "Failed to write on the local copy rc [0x %x]" ,rc);
        goto exitOnError;
    }
    if( CL_OK != rc )
    {
        /* any other error, this would have unheld the lock there so just go
         * out, return from here 
         */
        goto exitOnErrorWithoutUnlock;
    }
    
    /*
     * Inform the replica nodes as well. Presence list 
     * contains active replica address and other replicas.
     * Hence peerCount > 1 means that the checkpoint is 
     * replicated on other nodes.
     */
    clCntSizeGet(pCkpt->pCpInfo->presenceList,&peerCount);
    if(peerCount > 1)
    {
        if((pCkpt->pCpInfo->updateOption & CL_CKPT_WR_ALL_REPLICAS))
        {
            pCallInfo = (CkptCbInfoT *)clHeapAllocate(sizeof(CkptCbInfoT));
            if(pCallInfo == NULL)
            {
                rc = CL_CKPT_ERR_NO_MEMORY;
                CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                               ("Memory allocation failed %s rc[0x %x]\n",
                                pCkpt->ckptName.value,rc), rc);
            }         
            memset(pCallInfo,'\0',sizeof(CkptCbInfoT));
            rc = clCkptEoIdlSyncDefer(&idlDeferHdl);
            CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                           ("Failed to Pack section rc[0x %x]\n", rc), rc);
            pCallInfo->rmdHdl    = idlDeferHdl;
            pCallInfo->delFlag   = CL_TRUE;
            pCallInfo->cbCount   = peerCount - 1 ;
        }    
        memcpy(&ckptVersion,gCkptSvr->versionDatabase.versionsSupported,
               sizeof(ClVersionT));
        clCntFirstNodeGet (pCkpt->pCpInfo->presenceList, &nodeHdl);
        while (nodeHdl != 0)
        {
            rc = clCntNodeUserKeyGet (pCkpt->pCpInfo->presenceList, nodeHdl,
                                      &dataHdl);
            CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                           ("Cant update peer rc[0x %x]\n", rc), rc);
            peerAddr = (ClIocNodeAddressT)(ClWordT)dataHdl;
            clCntNextNodeGet(pCkpt->pCpInfo->presenceList, nodeHdl, &nodeHdl);

            rc = ckptIdlHandleUpdate(peerAddr,gCkptSvr->ckptIdlHdl,0);
            CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                           ("Failed to update the handle rc[0x %x]\n", rc), rc);

            if (peerAddr != gCkptSvr->localAddr)
            {
                if((pCkpt->pCpInfo->updateOption & CL_CKPT_WR_ALL_REPLICAS))
                {
                    rc = VDECL_VER(clCkptRemSvrCkptWriteClientAsync, 4, 0, 0)(
                                                                              gCkptSvr->ckptIdlHdl,
                                                                              &ckptVersion,
                                                                              ckptHdl,
                                                                              nodeAddr,
                                                                              portId,
                                                                              numberOfElements,
                                                                              pIoVector,
                                                                              ckptRemSvrWriteCallback,
                                                                              pCallInfo);
                    if( rc != CL_OK)
                    {
                        (pCallInfo->cbCount)--;
                    }
                }
                else
                {
                    rc = VDECL_VER(clCkptRemSvrCkptWriteClientAsync, 4, 0, 0)(
                                                                              gCkptSvr->ckptIdlHdl,
                                                                              &ckptVersion,
                                                                              ckptHdl,
                                                                              nodeAddr,
                                                                              portId,
                                                                              numberOfElements,
                                                                              pIoVector,
                                                                              NULL,0);
                }
                CL_CKPT_UNREACHABEL_ERR_CHK(rc, peerAddr);
            }                           
        }
    }

    clLogWrite(CL_LOG_HANDLE_APP,CL_LOG_INFORMATIONAL,CL_LOG_CKPT_SVR_NAME,
               CL_CKPT_LOG_1_CKPT_WRITTEN, pCkpt->ckptName.value);

    /*
     * Inform the clients about immediate consumption.
     */
    //_ckptUpdateClientImmConsmptn(&pCkpt->ckptName);
    exitOnError:
    {
        /*
         * Unlock the checkpoint's mutex.
         */
        CKPT_UNLOCK(pCkpt->ckptMutex);           
        
        clHandleCheckin(gCkptSvr->ckptHdl,ckptHdl);
        
        if( CL_OK != rc )
        {
            *pError = count;
        }
        goto out_free;
    }        
    exitOnErrorWithoutUnlock:
    {
        clHandleCheckin(gCkptSvr->ckptHdl,ckptHdl);
        *pError = count;
    }
    exitOnErrorBeforeHdlCheckout:

    out_free:
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

ClRcT
clCkptDefaultSectionInfoGet(CkptT         *pCkpt,
                            CkptSectionT  **ppSec)
{
    ClCkptSectionKeyT  key = {{0}}; 
    ClRcT              rc  = CL_OK;

    key.scnId.id = clHeapCalloc(1, strlen("defaultSection") + 1);
    if( NULL == key.scnId.id )
    {
        clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN,
                "Failed to allocate memory");
        return CL_CKPT_ERR_NO_MEMORY;
    }
    key.scnId.idLen = strlen("defaultSection") + 1;
    strncpy((ClCharT *) key.scnId.id, "defaultSection", key.scnId.idLen);
    key.hash = 0;

    rc = clCntDataForKeyGet(pCkpt->pDpInfo->secHashTbl, (ClCntKeyHandleT)
                            &key, (ClCntDataHandleT *) ppSec);
    if( CL_OK != rc )
    {
        clHeapFree(key.scnId.id);
        clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN, 
                   "Section [%.*s] doesnot exist in ckpt [%.*s]",
                   key.scnId.idLen, key.scnId.id, pCkpt->ckptName.length,
                   pCkpt->ckptName.value);
        if( CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST )
        {
            rc = CL_CKPT_ERR_NOT_EXIST;
        }
        return rc;
    }
    clHeapFree(key.scnId.id);
        
    return CL_OK;
}

ClRcT
clCkptSectionInfoGet(CkptT             *pCkpt,
                     ClCkptSectionIdT  *pSectionId,
                     CkptSectionT      **ppSec)
{
    ClCkptSectionKeyT  key   = {{0}}; 
    ClRcT              rc    = CL_OK;
    ClUint32T          cksum = 0;

    key.scnId.id    = pSectionId->id;
    key.scnId.idLen = pSectionId->idLen;
    rc = clCksm32bitCompute(key.scnId.id, key.scnId.idLen, &cksum);
    if( CL_OK != rc )
    {
        clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN, 
                   "Failed to find cksum while finding section [%.*s]",
                   key.scnId.idLen, key.scnId.id);
        return rc;
    }
    key.hash        = cksum % pCkpt->pDpInfo->numOfBukts;
    clLogTrace(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN, 
               "Hash value of this section [%.*s] is %d with idLen %d", 
               key.scnId.idLen, key.scnId.id, key.hash, 
               key.scnId.idLen);

    rc = clCntDataForKeyGet(pCkpt->pDpInfo->secHashTbl, (ClCntKeyHandleT)
                            &key, (ClCntDataHandleT *) ppSec);
    if( CL_OK != rc )
    {
        if( CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST )
        {
            rc = CL_CKPT_ERR_NOT_EXIST;
        }
        return rc;
    }
        
    return CL_OK;
}

static ClRcT _ckptSectionOverwriteWithVector(ClCkptHdlT         ckptHdl,
                                             ClIocNodeAddressT  nodeAddr,
                                             ClIocPortT        portId,
                                             ClBoolT           srcClient, 
                                             ClCkptSectionIdT  *pSectionId,
                                             ClTimeT           expirationTime, 
                                             ClSizeT           dataSize,
                                             ClUint8T          *pData,
                                             ClDifferenceVectorT    *differenceVector,
                                             ClVersionT        *pVersion)
{
    CkptT         *pCkpt    = NULL;  
    ClRcT         rc        = CL_OK; 
    CkptSectionT  *pSec     = NULL;
    ClUint32T     peerCount = 0;
    ClBoolT sectionLockTaken = CL_FALSE;
    ClDifferenceVectorT replicaDifferenceVector = {0};
    ClDifferenceVectorT *pReplicaDifferenceVector = NULL;
    ClBoolT replicate = CL_TRUE;
    /* 
     * Verify the server existence.
     */
    CL_CKPT_SVR_EXISTENCE_CHECK;

    /*
     * Verify the version.
     */
    rc = clVersionVerify(&gCkptSvr->versionDatabase,(ClVersionT *)pVersion);
    if( CL_GET_ERROR_CODE(rc) == CL_ERR_VERSION_MISMATCH)
    {
        return CL_OK;
    }
    memset(pVersion,'\0',sizeof(ClVersionT));
    /*
     * Retrieve the data associated with the active handle.
     */
    rc = clHandleCheckout(gCkptSvr->ckptHdl,ckptHdl,(void **)&pCkpt);
    CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                                  ("Failed to checkout the active handle [%llu] rc[0x %x]\n",(unsigned long long int) ckptHdl,rc), rc);

    CL_ASSERT(pCkpt != NULL);

    if(pCkpt->isPending == CL_TRUE)
    {
        rc = CL_CKPT_RC(CL_ERR_TRY_AGAIN);
        clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN, 
                   "Active server doesn't have the proper data");
        clHandleCheckin(gCkptSvr->ckptHdl, ckptHdl);
        return rc;
    }

    /*
     * Lock the checkpoint's mutex.
     */
    CKPT_LOCK(pCkpt->ckptMutex);           
    if(!pCkpt->ckptMutex)
    {
        clLogWarning("SEC", "WRITE", "Ckpt handle [%#llx] deleted", ckptHdl);
        sectionLockTaken = CL_TRUE;
        rc = CL_CKPT_ERR_NOT_EXIST;
        goto exitOnWithoutUnlock;
    }

    /*
     * Return ERROR if data size exceeds the max section
     * size.
     */
    if ((pCkpt->pDpInfo->maxScnSize != 0) && (dataSize > pCkpt->pDpInfo->maxScnSize) )
    {
        rc = CL_CKPT_ERR_INVALID_PARAMETER;
        clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_SEC_OVERWRITE, 
                   "Passed dataSize [%lld] is greater than stored size [%lld]", 
                   dataSize, pCkpt->pDpInfo->maxScnSize);
        goto exitOnWithoutUnlock;
    }
    /* take section level mutex */
    rc = clCkptSectionLevelLock(pCkpt, pSectionId, &sectionLockTaken);
    if( CL_OK != rc )
    {
        goto exitOnWithoutUnlock;
    }
    if(pSectionId->id == NULL)
    {
        if(pCkpt->pDpInfo->maxScns > 1)
        {
            /*
             * Return ERROR as id == NULL and max sec > 1
             */
            rc = CL_CKPT_ERR_INVALID_PARAMETER;
            clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_SEC_OVERWRITE, 
                       "Passed section id is NULL");
            /* Release section level mutex, exit */
            clCkptSectionLevelUnlock(pCkpt, pSectionId, sectionLockTaken);
            sectionLockTaken = CL_FALSE;
            goto exitOnWithoutUnlock;
        }
        rc = clCkptDefaultSectionInfoGet(pCkpt, &pSec);
    }
    else
    {
        /*
         * Find the section and return ERROR if the section doesnt exist.
         */
        rc = clCkptSectionInfoGet(pCkpt, pSectionId, &pSec);
        if( (CL_ERR_NOT_EXIST == CL_GET_ERROR_CODE(rc)) &&
            (srcClient == CL_FALSE) )
        {
            /* 
             * section creation happened while active is replicating the info
             * with peer address, due to active address didn't create the
             * sections at peer addr. so add the section with update
             * information of this section info and exit.
             */
            clLogInfo(CL_CKPT_AREA_PEER, CL_CKPT_CTX_CKPT_OPEN,
                      "Section [%.*s] in ckpt [%.*s] has not been created at"
                      "peer [%d], asking active address about that section",
                      pSectionId->idLen, pSectionId->id,
                      pCkpt->ckptName.length, pCkpt->ckptName.value, 
                      gCkptSvr->localAddr);
            rc = clCkptSectionChkNAdd(ckptHdl, pCkpt, pSectionId, expirationTime,
                                      dataSize, pData);
            if( CL_OK != rc )
            {
                clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN, 
                           "While creating section [%.*s] in ckpt [%.*s] failed rc[0x%x]", 
                           pSectionId->idLen, pSectionId->id,
                           pCkpt->ckptName.length, pCkpt->ckptName.value, rc);
            }
            else
            {
                /*
                 * Increment the number of sections, so we are sure that we
                 * have added the section
                 */
                ++pCkpt->pDpInfo->numScns;
            }
            /* Release section level mutex, exit */
            clCkptSectionLevelUnlock(pCkpt, pSectionId, sectionLockTaken);
            sectionLockTaken = CL_FALSE;
            goto exitOnWithoutUnlock;
        }
    }
    if( CL_OK != rc )
    {
        clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN, 
                   "Failed to find section [%.*s] in ckpt [%.*s] rc[0x %x]", 
                   pSectionId->idLen, pSectionId->id,
                   pCkpt->ckptName.length, pCkpt->ckptName.value, rc);
        /* Release section level mutex, exit */
        clCkptSectionLevelUnlock(pCkpt, pSectionId, sectionLockTaken);
        sectionLockTaken = CL_FALSE;
        goto exitOnWithoutUnlock;
    }

    if(sectionLockTaken == CL_TRUE)
    {
        /* release the ckpt mutex */
        CKPT_UNLOCK(pCkpt->ckptMutex);
    }

    if(differenceVector)
    {
        /* 
         * Free the existing data and attach the new data. But first merge the incoming data
         * with the existing one
         */
        pData = ckptDifferenceVectorMerge(pCkpt, pSectionId, pSec, differenceVector, 0, dataSize);
        CL_ASSERT(pData != NULL);
        /* 
         * Only for the first src. update, fetch the replica difference vector
         */
        if(srcClient) 
        {
            clDifferenceVectorCopy(&replicaDifferenceVector, differenceVector);
            pReplicaDifferenceVector = &replicaDifferenceVector; 
        }
        /*get the vector to replicate*/
        ckptDifferenceVectorGet(pCkpt, pSectionId, pSec, pData, 0, dataSize, pReplicaDifferenceVector, CL_TRUE); 
        if(pData != pSec->pData)
        {
            clHeapFree(pSec->pData);
            pSec->pData = pData;
        }
        pSec->size  = dataSize;
        /*
         * Skip replica update if there is nothing to update.
         */
        if(!replicaDifferenceVector.numDataVectors) 
            replicate = CL_FALSE;
    }
    else
    {
        /*
         * While transitioning from high version to lower version, rip off the difference
         * vector keys so the flip would start all over again instead of merging with stale data blocks
         */
        if(pSec->differenceVectorKey)
        {
            clDifferenceVectorDelete(pSec->differenceVectorKey);
            clDifferenceVectorKeyFree(pSec->differenceVectorKey);
            clHeapFree(pSec->differenceVectorKey);
            pSec->differenceVectorKey = NULL;
        }
        /* 
         * Free the existing data and attach the new data.
         */
        clHeapFree(pSec->pData);
        pSec->pData = (ClAddrT)clHeapCalloc(dataSize, sizeof(ClUint8T));
        if( NULL == pSec->pData ) 
        {
            clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN, 
                       "Memory allocation failed during section overwrite");
            /* Release section level mutex, exit */
            clCkptSectionLevelUnlock(pCkpt, pSectionId, sectionLockTaken);
            goto exitOnWithoutUnlock;
        }
        memcpy(pSec->pData, pData, dataSize);
        pSec->size  = dataSize;
    }

    /*
     * Update lastUpdate field of section, not checking error as this is not as important for
     * Section creation.
     */
    clCkptSectionTimeUpdate(&pSec->lastUpdated);
    /*
     * Inform the replica nodes as well. Presence list 
     * contains active replica address and other replicas.
     * Hence peerCount > 1 means that the checkpoint is 
     * replicated on other nodes.
     */
    if( srcClient == CL_TRUE && replicate)
    {
        clCntSizeGet(pCkpt->pCpInfo->presenceList,&peerCount);
        if(peerCount > 1)
        {
            rc = clCkptRemSvrSectionOverwriteVector(ckptHdl, pCkpt, pSectionId,
                                                    pSec, pData, dataSize,
                                                    nodeAddr, portId, pReplicaDifferenceVector);
            if( CL_OK != rc )
            {
                /* FIXME in case of sync checkpointing revert back */
            }
        }
    }

    /*
     * Inform the clients about immediate consumption.
     */
    if( (pCkpt->pCpInfo->updateOption & CL_CKPT_DISTRIBUTED ) )
    {
        clCkptClntSecOverwriteNotify(pCkpt, pSectionId, pSec, nodeAddr,
                                     portId);
    }
    /* Release section level mutex */
    clCkptSectionLevelUnlock(pCkpt, pSectionId, sectionLockTaken);

    exitOnWithoutUnlock:    
    {
        clHandleCheckin(gCkptSvr->ckptHdl,ckptHdl);
        if(sectionLockTaken == CL_FALSE)
        {
            CKPT_UNLOCK(pCkpt->ckptMutex);
        }
    }
    exitOnErrorBeforeHdlCheckout:
    {   
        if(pReplicaDifferenceVector)
            clDifferenceVectorFree(pReplicaDifferenceVector, CL_FALSE);
        return rc;
    }    
}

ClRcT VDECL_VER(_ckptSectionOverwriteVector, 4, 0, 0)(ClCkptHdlT         ckptHdl,
                                                      ClIocNodeAddressT  nodeAddr,
                                                      ClIocPortT        portId,
                                                      ClBoolT           srcClient, 
                                                      ClCkptSectionIdT  *pSectionId,
                                                      ClTimeT           expirationTime, 
                                                      ClSizeT           dataSize,
                                                      ClDifferenceVectorT    *differenceVector,
                                                      ClVersionT        *pVersion)
{
    ClUint8T *pData = NULL;
    ClRcT rc = CL_OK;
    if(!differenceVector)
        return CL_CKPT_RC(CL_ERR_INVALID_PARAMETER);
    if(!differenceVector->numDataVectors)
        goto out_free;
    pData = differenceVector->dataVectors[0].dataBase;
    /*
     * We take the clustersem only for the section overwrite call typically used by amf
     * which can get in while the ckpt server is shutting down.
     */
    clOsalMutexLock(&gCkptSvr->ckptClusterSem);
    rc = _ckptSectionOverwriteWithVector(ckptHdl, nodeAddr, portId, srcClient, pSectionId,
                                         expirationTime, dataSize, pData, differenceVector, pVersion);
    clOsalMutexUnlock(&gCkptSvr->ckptClusterSem);
    out_free:
    {
        ClUint32T i;
        for(i = 0; i < differenceVector->numDataVectors; ++i)
            clHeapFree(differenceVector->dataVectors[i].dataBase);
    }
    return rc;
}

/* 
 * Writes a single section in a given checkpoint.
 */

ClRcT VDECL_VER(_ckptSectionOverwrite, 4, 0, 0)(ClCkptHdlT         ckptHdl,
                                                ClIocNodeAddressT  nodeAddr,
                                                ClIocPortT        portId,
                                                ClBoolT           srcClient, 
                                                ClCkptSectionIdT  *pSectionId,
                                                ClTimeT           expirationTime, 
                                                ClSizeT           dataSize,
                                                ClUint8T          *pData,
                                                ClVersionT        *pVersion)
{
    ClRcT rc = CL_OK;
    /*
     * We take the clustersem only for the section overwrite call typically used by amf
     * which can get in while the ckpt server is shutting down.
     */
    clOsalMutexLock(&gCkptSvr->ckptClusterSem);
    rc = _ckptSectionOverwriteWithVector(ckptHdl, nodeAddr, portId, srcClient, pSectionId,
                                         expirationTime, dataSize, pData, NULL, pVersion);
    clOsalMutexUnlock(&gCkptSvr->ckptClusterSem);
    return rc;
}

/*
 * Reads multiple sections at a time. Can be used to read a single section
 */
ClRcT VDECL_VER(_ckptCheckpointRead, 4, 0, 0)(ClCkptHdlT               ckptHdl,
                          ClCkptIOVectorElementT   *pVec,
                          ClUint32T                numberOfElements,
                          ClCkptIOVectorElementT   *pOutVec,
                          ClUint32T                *pError,
                          ClVersionT               *pVersion)
{
    CkptT                   *pCkpt    = NULL;
    ClRcT                   rc        = CL_OK;
    CkptSectionT            *pSec     = NULL;
    ClUint32T               vecCount  = 0;
    ClCkptIOVectorElementT  *pTempVec = pOutVec;
    ClBoolT sectionLockTaken = CL_TRUE;

    /*
     * Check whether the server is fully up or not.
     */
    CL_CKPT_SVR_EXISTENCE_CHECK;

    /*
     * Verify the version.
     */
    rc = clVersionVerify(&gCkptSvr->versionDatabase,(ClVersionT *)pVersion);
    if( CL_GET_ERROR_CODE(rc) == CL_ERR_VERSION_MISMATCH)
    {
        return CL_OK;
    }

    memset(pVersion,'\0',sizeof(ClVersionT));

    CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Ckpt: Version b/w C/S is incompatablie rc[0x %x]",rc), rc);

    /*
     * Retrieve the data associated with the active handle.
     */
    rc = ckptSvrHdlCheckout(gCkptSvr->ckptHdl, ckptHdl, (void **)&pCkpt);
    CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Failed to get ckpt from handle rc[0x %x] ckptHdl [%#llX]\n",rc, ckptHdl), rc);

    if(pCkpt->isPending == CL_TRUE)
    {
        rc = CL_CKPT_RC(CL_ERR_TRY_AGAIN);
        (void)clHandleCheckin(gCkptSvr->ckptHdl, ckptHdl);
        clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN, 
             "Data for handle [%#llX] is not available rc[0x %x]", ckptHdl, rc);
        return rc;
    }
    /*
     * Lock the checkpoint's mutex here as the CKPT_ERR_CHECK below
     * would land in exitOnError label
     */

    CKPT_LOCK(pCkpt->ckptMutex);
    if(!pCkpt->ckptMutex)
    {
        clLogWarning("SEC", "READ", "Ckpt with handle [%#llx] already deleted", ckptHdl);
        rc = CL_CKPT_ERR_NOT_EXIST;
        sectionLockTaken = CL_TRUE;
        goto exitOnErrorWithoutUnlock;
    }

    for (vecCount = 0; vecCount < numberOfElements; vecCount++)
    {
        ClPtrT        pTmp     = 0;
        ClUint32T     readSize = 0;

        rc = clCkptSectionLevelLock(pCkpt, &pVec->sectionId, &sectionLockTaken);
        if( CL_OK != rc )
        {
            /* Not able to take section level mutex */
            clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN, 
                       "Not able to take section level mutex for section [%.*s]",
                        pVec->sectionId.idLen, pVec->sectionId.id);
            goto exitOnError;
        }
        /*
         * Locate the appropriate section.
         */
        if(pVec->sectionId.id == NULL)
        {
            /*
             * Default section.
             */
            if(pCkpt->pDpInfo->maxScns > 1)
            {
                /*
                 * ERROR as max Section>1 and id = NULL.
                 */
                rc = CL_CKPT_ERR_NULL_POINTER;
                clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN,
                        "Invalid SectionId:not found in ckpt %.*s rc[0x %x]\n",
                        pCkpt->ckptName.length, pCkpt->ckptName.value,rc);
                /* Leave section mutex & take ckpt mutex */
                clCkptSectionLevelUnlock(pCkpt, &pVec->sectionId, 
                                         sectionLockTaken);
                sectionLockTaken = CL_FALSE;
                goto exitOnErrorWithoutUnlock;
            }
            rc = clCkptDefaultSectionInfoGet(pCkpt, &pSec);
        }
        else
        {
            rc = clCkptSectionInfoGet(pCkpt, &pVec->sectionId, &pSec);
        }
        if( CL_OK != rc )
        {
            clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN, 
                       "Section [%.*s] does not exist in ckpt [%.*s]", 
                       pVec->sectionId.idLen, pVec->sectionId.id,
                       pCkpt->ckptName.length, pCkpt->ckptName.value);
            clCkptSectionLevelUnlock(pCkpt, &pVec->sectionId, sectionLockTaken);
            sectionLockTaken = CL_FALSE;
            goto exitOnErrorWithoutUnlock;
        }

        if(sectionLockTaken == CL_TRUE)
        {
            /* acquired section level mutex, leaving ckpt mutex */
            CKPT_UNLOCK(pCkpt->ckptMutex);
        }

        /*
         * Return ERROR if section boundary is voilated.
         */

        if((pCkpt->pDpInfo->maxScnSize != 0) && (pVec->dataSize > pCkpt->pDpInfo->maxScnSize))
        {
            rc = CL_CKPT_ERR_INVALID_PARAMETER;
            clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN,
                    "Invalid data size [%lld] maxSectionSize [%lld] for ckpt [%.*s] rc [0x %x]", 
                     pVec->dataSize, pCkpt->pDpInfo->maxScnSize, pCkpt->ckptName.length,
                     pCkpt->ckptName.value, rc);
            /* Leave section mutex & take ckpt mutex */
            clCkptSectionLevelUnlock(pCkpt, &pVec->sectionId, sectionLockTaken);
            goto exitOnErrorWithoutUnlock;
        }

        /*
         * Return ERROR in case offset > maxSecSize.
         */
        if((pCkpt->pDpInfo->maxScnSize != 0) && (pVec->dataOffset > pCkpt->pDpInfo->maxScnSize))
        {
            rc = CL_CKPT_ERR_INVALID_PARAMETER;
            clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN, 
                    "Data offset [%lld] is greater than maxScnSize [%lld] for"
                    "ckpt [%.*s] rc[0x %x]\n",
                     pVec->dataOffset, pCkpt->pDpInfo->maxScnSize, 
                     pCkpt->ckptName.length, pCkpt->ckptName.value, rc);
            /* Leave section mutex & take ckpt mutex */
            clCkptSectionLevelUnlock(pCkpt, &pVec->sectionId, sectionLockTaken);
            goto exitOnErrorWithoutUnlock;
        }
        
        /*
         * Check if the required quantity of data can be read or not.
         */
        if(pVec->dataBuffer == NULL)
        {
            readSize = pSec->size;
        }
        else
        { 
            readSize = pVec->dataSize;
        }

        if(pVec->dataOffset > pSec->size)
        {
            readSize = 0;
        }
        else if((pVec->dataSize + pVec->dataOffset) > pSec->size)
        {
            readSize = pSec->size - pVec->dataOffset;
        }

        /*
         * Allocate buffer if reader hasnt specified any.
         */
        if (pOutVec->dataBuffer == NULL)
        {
            if((readSize > 0) &&
               (NULL == ( pOutVec->dataBuffer = (ClPtrT) clHeapCalloc(1,
                            readSize))))
            {
                rc = CL_CKPT_ERR_NO_MEMORY;
                clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN,
                    "Memory allocation failed %.*s rc[0x %x]\n",
                    pCkpt->ckptName.length, pCkpt->ckptName.value,rc);
                /* Leave section mutex & take ckpt mutex */
                clCkptSectionLevelUnlock(pCkpt, &pVec->sectionId, 
                                         sectionLockTaken);
                goto exitOnErrorWithoutUnlock;
            }
        }

        pTmp = (ClCharT *)pSec->pData + pVec->dataOffset;
        if(readSize > 0)
        {
            memcpy(pOutVec->dataBuffer, pTmp, readSize);
        }
        pOutVec->readSize = readSize;
        pOutVec->dataSize = readSize;
        /*
         * Done with copying the data, so leave section mutex acquire CKPT
         * mutex 
         */
        clCkptSectionLevelUnlock(pCkpt, &pVec->sectionId, sectionLockTaken);
        if(sectionLockTaken == CL_TRUE)
        {
            CKPT_LOCK(pCkpt->ckptMutex);
            if(!pCkpt->ckptMutex)
            {
                clLogWarning("SEC", "READ", "Ckpt with handle [%#llx] already deleted", ckptHdl);
                rc = CL_CKPT_ERR_NOT_EXIST;
                sectionLockTaken = CL_TRUE;
                goto exitOnErrorWithoutUnlock;
            }
        }

        /*
         * Section copy is not necessary here, coz in the client side, 
         * we are copying the section back from the output received from 
         * server side, so removing this part passing NULL & 0 
         */
        if(pVec->sectionId.id != NULL)
        {
            if(pVec->sectionId.id != NULL)
            {
                clHeapFree(pVec->sectionId.id);
                pVec->sectionId.id = NULL;
            }
            if(pVec->dataBuffer != NULL)
            {
                clHeapFree(pVec->dataBuffer);
                pVec->dataBuffer = NULL;
            }
        }
        pOutVec->sectionId.idLen = 0;
        pOutVec->sectionId.id    = NULL;
        pVec++;
        pOutVec++;
    }
    pOutVec = pTempVec;

    rc = clHandleCheckin(gCkptSvr->ckptHdl,ckptHdl);
    clLogWrite(CL_LOG_HANDLE_APP,CL_LOG_INFORMATIONAL,CL_LOG_CKPT_SVR_NAME,
            CL_CKPT_LOG_1_CKPT_READ,  pCkpt->ckptName.value);
    /*
     * Unlock the checkpoint's mutex.
     */
    CKPT_UNLOCK(pCkpt->ckptMutex);
    return rc;
exitOnError:
    {
        /*
         * Unock the checkpoint's mutex.
         */
        CKPT_UNLOCK(pCkpt->ckptMutex);
    }
exitOnErrorWithoutUnlock:    
    {
        clHandleCheckin(gCkptSvr->ckptHdl,ckptHdl);
        if(sectionLockTaken == CL_FALSE)
        {
            CKPT_UNLOCK(pCkpt->ckptMutex);
        }
        *pError = vecCount;
    }
exitOnErrorBeforeHdlCheckout:
    return rc;
}

/*
 * Synchronizes the replicas of a checkpoint 
 */
ClRcT VDECL_VER(_ckptCheckpointSynchronize, 4, 0, 0)(ClCkptHdlT      ckptHdl,
                                 ClTimeT         timeout,
                                 ClBoolT         flag,
                                 ClCkptHdlT      clntHdl,
                                 ClVersionT      *pVersion)
{
    CkptT           *pCkpt    = NULL;
    ClRcT           rc        = CL_OK;
    ClRcT           retCode   = CL_OK;
    ClRcT*          retCodePtr = NULL;
    ClOsalCondIdT   condVar   = 0;
    ClOsalMutexIdT  mutexVar  = 0;
    ClTimerTimeOutT timeOut   = {0};
    ClUint32T       peerCount = 0;
    
    /*
     * Check whether the server is fully up or not.
     */
    CL_CKPT_SVR_EXISTENCE_CHECK;
    
    /*
     * Verify the version.
     */
    rc = clVersionVerify(&gCkptSvr->versionDatabase,(ClVersionT *)pVersion);
    if( CL_GET_ERROR_CODE(rc) == CL_ERR_VERSION_MISMATCH)
    {
     return CL_OK;
    }
    
    memset(pVersion,'\0',sizeof(ClVersionT));
    
    /*
     * Retrieve the data associated with the active handle.
     */
    rc = clHandleCheckout(gCkptSvr->ckptHdl,ckptHdl,(void **)&pCkpt);
    if(CL_OK != rc )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
         ("Handle [%#llX] checkout failed rc[0x %x]\n", ckptHdl, rc));
        return rc;
    }
    
    /*
     * Lock the checkpoint's mutex.
     */
    CKPT_LOCK(pCkpt->ckptMutex);
    if(!pCkpt->ckptMutex)
    {
        clHandleCheckin(gCkptSvr->ckptHdl, ckptHdl);
        clLogWarning("SEC", "SYNC", "Ckpt with handle [%#llx] already deleted", ckptHdl);
        return CL_CKPT_ERR_NOT_EXIST;
    }
    
    /*
     * Validate control plane info.
     */
    if( pCkpt->pCpInfo == NULL ) rc = CL_CKPT_ERR_INVALID_STATE;
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Failed to the active handle from handle rc[0x %x]\n",rc), rc);
       
    /*
     * Return ERROR if the checkpoint type is SYNC.
     */
    if( (pCkpt->pCpInfo->updateOption & CL_CKPT_WR_ALL_REPLICAS ))
        rc = CL_CKPT_ERR_INVALID_STATE;
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Failed to the active handle from handle rc[0x %x]\n",rc), rc);
            
    /*
     * Syncup all replicas associated with the given ckpt hdl. peerCount > 1
     * means replicas for the checkpoints exist.
     */
    clCntSizeGet(pCkpt->pCpInfo->presenceList,&peerCount);
    if(peerCount > 1 ) 
    {
        if( flag != 1 )
        {
            /*
             * Synchronous flavour of the call. Initialize the condition
             * variable.
             */
            rc = clOsalMutexCreate(&mutexVar);
            CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                    ("Mutex creation failed rc[0x %x]\n",rc), rc);
            rc = clOsalCondCreate(&condVar);
            CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                ("Condition variable creation failed rc[0x %x]\n",rc), rc);
            /* Only set the retcode if I'm doing a synchronous call */
            retCodePtr = &retCode;
        }
        
        /*
         * Push the ckpt info to the replicas.
         */
        rc = _ckptRemSvrCheckpointAdd(pCkpt,ckptHdl,retCodePtr,condVar);
        CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                ("Failed to add in remote server rc[0x %x]\n",rc), rc);
                
        if(flag != 1)
        {
            /*
             * Synchronous flavour of the call. Wait till the calls to all
             * the replicas have been made.
             */
            timeOut.tsSec = 0;
            timeOut.tsMilliSec = 0;
            clOsalMutexLock(mutexVar);
            clOsalCondWait(condVar,mutexVar,timeOut);
            clOsalMutexUnlock(mutexVar);
            clOsalMutexDelete(mutexVar);
            clOsalCondDelete(condVar);
            rc = retCode;
        }
    }
exitOnError:
   {
        /*
         * Unock the checkpoint's mutex.
         */
        CKPT_UNLOCK(pCkpt->ckptMutex);
        
        clHandleCheckin(gCkptSvr->ckptHdl,ckptHdl);
        return rc;
   }
}
 
/*
 * Updates the client that data is ready for consumption
 */
ClRcT _ckptUpdateClientImmConsmptn(ClNameT *pName)
{
    ClRcT                rc        = CL_OK;
    ClCkptClientUpdInfoT eventInfo = {0};
    ClEventIdT           eventId   = 0;

    /*
     * Publish the immediate consumption replated event.
     */
    memset(&eventInfo, 0, sizeof(ClCkptClientUpdInfoT));
    eventInfo.eventType = htonl(CL_CKPT_IMMEDIATE_CONS_EVENT);
    clNameCopy(&eventInfo.name, pName);
    eventInfo.name.length = htons(pName->length);
    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n Ckpt: Publishing an event for"
          " immediate consumption\n"));
    rc = clEventPublish(gCkptSvr->clntUpdEvtHdl, (const void*)&eventInfo,
                        sizeof(ClCkptClientUpdInfoT), &eventId);
    return rc;
}



/*
 * Synchronize related Callback function that will be called 
 * after the replica node has processed the checkpoint information
 * provided by the active replica. Defer sync funda for synchronize.
 */

static
void VDECL_VER(ckptRemSvrCkptAddCallback, 5, 0, 0)( ClIdlHandleT  ckptIdlHdl,
                                                    ClVersionT    *pVersion,
                                                    ClHandleT     ckptActHdl,
                                                    ClNameT       *pName,
                                                    VDECL_VER(CkptCPInfoT, 5, 0, 0)   *pCpInfo,
                                                    CkptDPInfoT   *pDpInfo,
                                                    ClRcT         retCode,
                                                    void          *pData)
{
    ClIocAddressT  srcAddr = {{0}};
    ClRcT          rc      = CL_OK;
    CkptMutxInfoT  *pInfo  = (CkptMutxInfoT *)pData;
    
    /*
     * Decrement the callback count associated with the synchronize
     * call. If the count reaches 0, inform the client about SUCCESS
     * or FAILURE.
     */
    (pInfo->cbCount)--;

    if (pInfo->pRetCode) /* Make sure the retcode is being tracked */
    {
        /* Any failure should return a failure, so if the retCode is currently showing failure
           don't overwrite it */
        if (*(pInfo->pRetCode) == CL_OK) *(pInfo->pRetCode) = retCode;
    }

    if (pInfo->cbCount == 0)
    {
        clOsalCondSignal(pInfo->condVar);
        if(CL_GET_ERROR_CODE(retCode) == CL_ERR_VERSION_MISMATCH)
        {
            rc = clRmdSourceAddressGet(&srcAddr.iocPhyAddress);
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL,
                       CL_CKPT_LOG_6_VERSION_NACK, "CkptRemSvrAdd",
                       srcAddr.iocPhyAddress.nodeAddress,
                       srcAddr.iocPhyAddress.portId, pVersion->releaseCode,
                       pVersion->majorVersion, pVersion->minorVersion);

            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Ckpt nack recieved from "
                                            "NODE[0x%x:0x%x], rc=[0x%x]\n", 
                                            srcAddr.iocPhyAddress.nodeAddress,
                                            srcAddr.iocPhyAddress.portId,
                                            CL_CKPT_ERR_VERSION_MISMATCH));
        }
        clHeapFree(pInfo); 
    }
    /*call signal to wake condition*/
    return;
}

static
void VDECL_VER(ckptRemSvrCkptAddCallback, 4, 0, 0)( ClIdlHandleT  ckptIdlHdl,
                                                    ClVersionT    *pVersion,
                                                    ClHandleT     ckptActHdl,
                                                    ClNameT       *pName,
                                                    CkptCPInfoT   *pCpInfo,
                                                    CkptDPInfoT   *pDpInfo,
                                                    ClRcT         retCode,
                                                    void          *pData)
{
    VDECL_VER(CkptInfoT, 5, 0, 0) ckptInfo = {0};
    VDECL_VER(CkptCPInfoT, 5, 0, 0) cpInfo = {0};
    CkptInfoT ckptInfoBase = {0};
    CkptDPInfoT dpInfo = {0};
    ckptInfo.pCpInfo = &cpInfo;
    ckptInfo.pDpInfo = &dpInfo;
    ckptInfoBase.pCpInfo = pCpInfo;
    ckptInfoBase.pDpInfo = pDpInfo;
    ckptInfoVersionConvertForward(&ckptInfo, &ckptInfoBase);
    VDECL_VER(ckptRemSvrCkptAddCallback, 5, 0, 0)(ckptIdlHdl, pVersion, ckptActHdl, pName, &cpInfo, &dpInfo,
                                                  retCode, pData);
}


/*
 * Function to push checkpoint info to the replicas. Called from synchronize
 * implementation.
 */
 
ClRcT  _ckptRemSvrCheckpointAdd(CkptT         *pCkpt,
                                ClCkptHdlT    ckptHdl,
                                ClRcT         *pRetPtr,
                                ClOsalCondIdT condVar)
{
    ClRcT             rc          = CL_OK;
    ClCntNodeHandleT  nodeHdl     = 0;
    ClCntDataHandleT  dataHdl     = 0;
    ClIocNodeAddressT peerAddr    = 0;
    CkptMutxInfoT     *pCondInfo  = NULL;
    ClVersionT        ckptVersion = {0};
    VDECL_VER(CkptInfoT, 5, 0, 0) ckptInfo = {0};

    if( condVar != 0)
    {
        /*
         * Synchronous flavour.
         */
        pCondInfo = (CkptMutxInfoT *)clHeapCalloc(1,sizeof(CkptMutxInfoT));
        if(pCondInfo == NULL)
        {
            rc = CL_CKPT_ERR_NO_MEMORY;
            CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                           ("Failed to allocate memory rc[0x %x]\n",rc), rc);
        }
        CL_ASSERT(pCondInfo);
        /*
         * Store the info related to the particular synchronize call.
         */
        pCondInfo->cbCount  = 0 ;
        pCondInfo->pRetCode = pRetPtr; 
        pCondInfo->condVar  = condVar;
    }    

    ckptInfo.pCpInfo = clHeapCalloc(1, sizeof(*ckptInfo.pCpInfo));
    ckptInfo.pDpInfo = clHeapCalloc(1, sizeof(*ckptInfo.pDpInfo));
    CL_ASSERT(ckptInfo.pCpInfo != NULL);
    CL_ASSERT(ckptInfo.pDpInfo != NULL);

    /*
     * Pack the control plane and the data plane info.
     */
    rc = _ckptCheckpointInfoPack( pCkpt, 
                                  ckptInfo.pCpInfo,
                                  ckptInfo.pDpInfo);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                   ("Failed: Marshalling rc[0x %x]\n",rc), rc);

    /*
     * Copy the supported version.
     */
    memcpy( &ckptVersion,gCkptSvr->versionDatabase.versionsSupported,
            sizeof(ClVersionT));
    
    /*
     * Iterate through the checkpoint's presence list and push the checkpoint
     * info to all the replicas.
     */
    clCntFirstNodeGet (pCkpt->pCpInfo->presenceList, &nodeHdl);
    while (nodeHdl != 0)
    {
        rc = clCntNodeUserKeyGet (pCkpt->pCpInfo->presenceList, nodeHdl, &dataHdl);
        CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                       ("Cant update peer rc[0x %x]\n", rc), rc);
        peerAddr = (ClIocNodeAddressT)(ClWordT) dataHdl;
        clCntNextNodeGet(pCkpt->pCpInfo->presenceList, nodeHdl, &nodeHdl);
        
        /*
         * Send the RMD only if peerAddress is not localAddress.
         */
        if (peerAddr != gCkptSvr->localAddr)
        {
            /*
             * Update the idl handle.
             */
            rc = ckptIdlHandleUpdate(peerAddr,gCkptSvr->ckptIdlHdl,0);
            CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                           ("Idl handle update failed rc[0x %x]\n", rc), rc);
                    
            if(condVar != 0)
            {
                /*
                 * Synchronous flavour.
                 */
                
                /* 
                 * Increment the callback count associated with the given 
                 * synchronize call.
                 */
                pCondInfo->cbCount++;
                VERSIONED_CKPT_INFO_SYNC_CLIENT_ASYNC(gCkptSvr->ckptIdlHdl,
                                                      &ckptVersion,
                                                      ckptHdl,
                                                      &(pCkpt->ckptName),
                                                      &ckptInfo,
                                                      ckptRemSvrCkptAddCallback,
                                                      pCondInfo, 
                                                      rc);
                if( rc != CL_OK)
                {
                    /* 
                     * Decrement the associated callback count as the call 
                     * failed.
                     */
                    pCondInfo->cbCount--;
                }
            }
            else
            {
                /*
                 * Asynchronous flavour. No need to maintain any state info.
                 * Just send the message.
                 */
                VERSIONED_CKPT_INFO_SYNC_CLIENT_ASYNC_NOCB(gCkptSvr->ckptIdlHdl,
                                                           &ckptVersion,
                                                           ckptHdl,
                                                           &pCkpt->ckptName,
                                                           &ckptInfo, callbackDummy, NULL, rc);
            }
            CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                           ("Failed to add the checkpoint in the remote Server "
                            "rc[0x %x]\n", rc), rc);
        }
    }
    exitOnError:
    {
        ckptPackInfoFree(&ckptInfo);
        return rc;
    }
}


/*
 * Defer sync funda for section updation.
 */
void _ckptRemSvrSectionUpdateCallback(ClIdlHandleT      ckptIdlHdl,
                                      ClVersionT        *pVersion,
                                      ClHandleT         ckptHdl,
                                      CkptUpdateFlagT   updateFlag,
                                      CkptSectionInfoT  *pSecInfo,
                                      ClRcT             retCode,
                                      void              *pData)
{
    ClIocAddressT   srcAddr     = {{0}};
    ClRcT           rc          =  CL_OK;
    ClVersionT      ckptVersion =  {0}; 
    CkptCbInfoT    *pCbInfo     = (CkptCbInfoT *)pData;
    ClUint32T       index       = 0;
    
    /*
     * Decrement the callback count associated with the section
     * updation. If the count reaches 0, inform the client about SUCCESS
     * or FAILURE.
     */
    (pCbInfo->cbCount)--;
    CKPT_DEBUG_T(("UpdateFlag: %d, Retcode : rc [0x %x],CbCount:%d\n", 
                updateFlag, retCode, pCbInfo->cbCount));
    
    if((retCode != CL_OK) || pCbInfo->cbCount == 0)
    {
        pCbInfo->cbCount = 0;
        if(CL_GET_ERROR_CODE(retCode) == CL_ERR_VERSION_MISMATCH)
        {
            memcpy(&ckptVersion, pVersion, sizeof(ClVersionT));
            rc = clRmdSourceAddressGet(&srcAddr.iocPhyAddress);
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL,
                    CL_CKPT_LOG_6_VERSION_NACK, "CkptRemSvrAdd",
                    srcAddr.iocPhyAddress.nodeAddress,
                    srcAddr.iocPhyAddress.portId, pVersion->releaseCode,
                    pVersion->majorVersion, pVersion->minorVersion);

            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Ckpt nack recieved from "
                        "NODE[0x%x:0x%x], rc=[0x%x]\n", 
                        srcAddr.iocPhyAddress.nodeAddress,
                        srcAddr.iocPhyAddress.portId,
                        CL_CKPT_ERR_VERSION_MISMATCH));
        }
        memset(&ckptVersion, '\0', sizeof(ClVersionT));

        /*
         * Based on updateFlag call the appropriate function.
         */
        switch(updateFlag)
        {
            case CKPT_SECTION_CREAT:
                rc = VDECL_VER(_ckptSectionCreateResponseSend, 4, 0, 0)( pCbInfo->rmdHdl,
                        retCode,
                        ckptVersion, index);
                break;                                                     
            case CKPT_SECTION_OVERWRITE:
                rc = VDECL_VER(_ckptSectionOverwriteResponseSend, 4, 0, 0)( pCbInfo->rmdHdl,
                        retCode,
                        ckptVersion);
                break;                                                     
            case CKPT_SECTION_DELETE:
                rc = VDECL_VER(_ckptSectionDeleteResponseSend, 4, 0, 0)( pCbInfo->rmdHdl,
                        retCode,
                        ckptVersion);
                break;                                                     
            default:
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                        ("Wrong callback has been called \n"));
                break;
        }
        clHeapFree(pCbInfo); 
    }
    if( NULL != pSecInfo )
    {
        clHeapFree(pSecInfo->secId.id);
        clHeapFree(pSecInfo->pData);
    }
    return;
}

#if 0
/*
 * Function to update the checkpoint's replicas whenever there is a change in
 * section at active replica. "updateFlag" will tell the replicas about the
 * nature of the change (e.g section creation, deletion etc..)
 */
 
ClRcT  _ckptRemSvrSectionInfoUpdate(ClHandleT      ckptHdl, 
                                    CkptT          *pCkpt,
                                    CkptSectionT   *pSec,
                                    CkptUpdateFlagT updateFlag,
                                    ClBoolT         isDefer,
                                    ClUint32T       count)
{
    ClRcT              rc          = CL_OK;
    ClCntNodeHandleT   nodeHdl     = 0;
    ClIocNodeAddressT  peerAddr    = 0;
    CkptCbInfoT        *pCallInfo  = NULL;
    ClVersionT         ckptVersion = {0};
    CkptSectionInfoT   *pSecInfo   = NULL;

    /*
     * Check if defer sync is to be used.
     */
    if(isDefer)
    {
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
            rc = clckptEoIdlSyncDefer((ClIdlHandleT *)&pCallInfo->rmdHdl);
            CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                    ("Failed to defer the call rc[0x %x]\n", rc), rc);
        }
    }

    /*
     * Inform all the replica nodes of the checkpoint.
     */

    /*
     * Set the supported version information.
     */
    memcpy( &ckptVersion, gCkptSvr->versionDatabase.versionsSupported,
            sizeof(ClVersionT));

    /* 
     * Walk through the checkpoint's presence list.
     */
    clCntFirstNodeGet (pCkpt->pCpInfo->presenceList, &nodeHdl);
    while (nodeHdl != 0)
    {
        rc = clCntNodeUserKeyGet (pCkpt->pCpInfo->presenceList, nodeHdl, 
                                 (ClCntKeyHandleT *)&peerAddr);
        CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                ("Cant update peer rc[0x %x]\n", rc), rc);
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
            pSecInfo = (CkptSectionInfoT *)clHeapCalloc(1,
                                            sizeof(CkptSectionInfoT));
            if(pSecInfo == NULL)
            {
                rc = CL_CKPT_ERR_NO_MEMORY;
                CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                   ("Failed to allocate the memory rc[0x %x]\n", rc), rc);
            }

            /*
             * Pack the section data to be sent to the replicas.
             */
            rc = _ckptSectionDataPack(pSec, pSecInfo, updateFlag, count);
            CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                    ("Failed to pack the data rc[0x %x]\n", rc), rc);

            rc = ckptIdlHandleUpdate(peerAddr,gCkptSvr->ckptIdlHdl,0);
            CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                    ("Failed to update the handle rc[0x %x]\n", rc), rc);
            
            /*
             * Update the replica.
             */
            if((pCkpt->pCpInfo->updateOption & CL_CKPT_WR_ALL_REPLICAS) 
               &&
               isDefer == CL_TRUE)
            {
                /*
                 * Use the defer sync concept.
                 */
                pCallInfo->cbCount++;
                rc = VDECL_VER(clCkptRemSvrSectionInfoUpdateClientAsync, 4, 0, 0)(
                        gCkptSvr->ckptIdlHdl,
                        &ckptVersion,
                        ckptHdl,
                        updateFlag,
                        pSecInfo,
                        _ckptRemSvrSectionUpdateCallback,
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
                rc = VDECL_VER(clCkptRemSvrSectionInfoUpdateClientAsync, 4, 0, 0)(
                        gCkptSvr->ckptIdlHdl,
                        &ckptVersion,
                        ckptHdl,
                        updateFlag,
                        pSecInfo,
                        NULL,NULL);
            }
            clHeapFree(pSecInfo->secId.id);
            clHeapFree(pSecInfo->pData);
            clHeapFree(pSecInfo);
            pSecInfo = NULL;
        }
        CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                ("Failed to add the section in remSvr rc[0x %x]\n", rc), rc);
    }

    return rc;
exitOnError:
    {
        if(pSecInfo != NULL)
        {    
            if( pSecInfo->secId.id != NULL ) 
                clHeapFree(pSecInfo->secId.id);
            if( pSecInfo->pData != NULL )
                clHeapFree(pSecInfo->pData);
            clHeapFree(pSecInfo);
        }    
        /*
         * pCallInfo should be freed incase of failure or in the Callback.
         */
        if(pCallInfo != NULL)
            clHeapFree(pCallInfo);
            
    }
exitOnErrorBeforeHdlCheckout:
    return rc;
}
#endif

/*
 * Funciton to pull the checkpoint from the active replica.
 */
 
ClRcT  VDECL_VER(clCkptReplicaNotify, 4, 0, 0)(ClVersionT        *pVersion,
                                               ClCkptHdlT        ckptActHdl,
                                               ClIocNodeAddressT activeAddr)
{
    ClRcT      rc        = CL_OK;
    CkptInfoT ckptInfoBase = {0};
    VDECL_VER(CkptInfoT, 5, 0, 0)  ckptInfo  = {0}; 
    ClBoolT scaleDown = CL_FALSE;

    clLogDebug(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN, 
               "Pulling ckpt info from activeAddr [%d] for ckptHdl [%#llX]", 
               activeAddr, ckptActHdl);
    /*
     * Version verification.
     */
    rc = clVersionVerify(&gCkptSvr->versionDatabase,pVersion);
    if( rc != CL_OK)
    {
        /*
         * Idl will not pack the INOUT variable,if return value is not CL_OK.
         */
        return CL_OK;
    }

    /*
     * Update the idl handle with the active replica's address.
     * retry increased to 2. if it timeouts, it can again try to pull info
     * the active address
     */
    rc = ckptIdlHandleUpdate(activeAddr,gCkptSvr->ckptIdlHdl, 2);       
    if( CL_OK != rc )
    {
        clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN, 
                   "Replica notify failed while updating idl handle rc [0x %x]",
                   rc);
        goto exitOnError;
    }
    /*
     * make call to active server to pull the info 
     */
    rc = VDECL_VER(clCkptRemSvrCkptInfoGetClientSync, 5, 0, 0)(gCkptSvr->ckptIdlHdl,
                                                               pVersion,
                                                               ckptActHdl,
                                                               gCkptSvr->localAddr,
                                                               &ckptInfo);
    if(CL_RMD_VERSION_ERROR(rc))
    {
        scaleDown = CL_TRUE;
        rc = VDECL_VER(clCkptRemSvrCkptInfoGetClientSync, 4, 0, 0)(gCkptSvr->ckptIdlHdl,
                                                                   pVersion,
                                                                   ckptActHdl,
                                                                   gCkptSvr->localAddr,
                                                                   &ckptInfoBase);
    }
    /*
     * Verison mismatch check.
     */
    if( (pVersion->releaseCode  != '\0') && (rc == CL_OK))
    {
        rc = CL_CKPT_ERR_VERSION_MISMATCH;
    }
    if( CL_OK != rc )
    {
        clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN, 
                   "Failed to pull the info from active address [%d]"
                   "rc [0x %x]", activeAddr, rc);
        goto exitOnError;
    }
    if(scaleDown)
    {
        ckptInfo.pCpInfo = NULL;
        ckptInfo.pDpInfo = NULL;
        ckptInfoVersionConvertForward(&ckptInfo, &ckptInfoBase);
    }
    /* 
     * Unpack the received info.
     */
    rc = _ckptReplicaInfoUpdate(ckptActHdl, &ckptInfo);
    if( CL_OK != rc )
    {
        clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN, 
                   "Updating the active server database failed handle [%#llX]",
                   ckptActHdl);
        goto ckptInfoFreeNExit;
    }
    /*
     * resetting Version info.
     */ 
    memset(pVersion,'\0',sizeof(ClVersionT));
    ckptPackInfoFree( &ckptInfo );
    return CL_OK;
    ckptInfoFreeNExit:    
    /*
     * Free the output variable pCkptInfo
     */
    memset(pVersion,'\0',sizeof(ClVersionT));
    ckptPackInfoFree( &ckptInfo );
    exitOnError:
    return rc;
}

ClRcT _ckptActiveCkptDeleteCall(ClVersionT version,
                                ClHandleT  ckptHdl)
{   
    return (VDECL_VER(clCkptActiveCkptDelete, 4, 0, 0)(version, ckptHdl));
    
}

/* 
 * Function to walk through th epresence list of all the checkpoints and
 * delete the address that is passed in the cookie filed.
 */

ClRcT ckptPresenceListNodeDelete( ClCntKeyHandleT   key,
                                  ClCntDataHandleT  hdl,
                                  void              *pData,
                                  ClUint32T         dataLength)
{
    CkptT           *pCkpt     = NULL;
    ClCkptHdlT      ckptHdl    = *(ClCkptHdlT *)hdl;
    ClRcT           rc         = CL_OK;
    ClCkptAppInfoT  *pAappInfo = NULL;   

    if (pData == NULL) return CL_CKPT_ERR_NULL_POINTER;

    /*
     * Obtain the address of the node/ckptserver going down.
     */
    pAappInfo = (ClCkptAppInfoT *)pData;

    /*
     * Retrieve the information associated with the checkpoint handle.
     */
    rc = clHandleCheckout(gCkptSvr->ckptHdl, ckptHdl, (void **)&pCkpt);
    if (rc == CL_OK && pCkpt != NULL)
    {
        CKPT_LOCK(pCkpt->ckptMutex);
        if(!pCkpt->ckptMutex)
        {
            clHandleCheckin(gCkptSvr->ckptHdl, ckptHdl);
            clLogWarning("NODE", "DELETE", "Ckpt with handle [%#llx] already deleted", ckptHdl);
            return rc;
        }
        if (pCkpt->pCpInfo != NULL)
        {
            if (pCkpt->pCpInfo->presenceList != 0)
            {
                /*
                 * Delete the container node associated with the passed
                 * node address.
                 */
                clCntAllNodesForKeyDelete(pCkpt->pCpInfo->presenceList,
                                          (ClCntKeyHandleT)(ClWordT)
                                          pAappInfo->nodeAddress);
            }
            if( pCkpt->pCpInfo->appInfoList != 0 )
            {
                clCntAllNodesForKeyDelete(pCkpt->pCpInfo->appInfoList, 
                                          (ClCntKeyHandleT)(ClWordT)
                                          pAappInfo);
            }
        }
        CKPT_UNLOCK(pCkpt->ckptMutex);
        rc = clHandleCheckin(gCkptSvr->ckptHdl,ckptHdl);
    }
    return rc;
}

ClRcT
clCkptSectionCreateInfoPull(ClCkptHdlT        ckptHdl, 
                            CkptT             *pCkpt,
                            ClCkptSectionIdT  *pSecId)
{
    ClRcT                  rc      = CL_OK;
    ClIocPhysicalAddressT  srcAddr = {0};
    CkptSectionInfoT       secInfo = {0};
    ClVersionT             version = {0};

    rc = clRmdSourceAddressGet(&srcAddr);
    if( CL_OK != rc )
    {
        clLogError(CL_CKPT_AREA_PEER, CL_CKPT_CTX_CKPT_OPEN, 
                   "Failed to get the src address rc[0x %x]", rc);
        return rc;
    }
    rc = ckptIdlHandleUpdate(srcAddr.nodeAddress, gCkptSvr->ckptIdlHdl,0);
    if( CL_OK != rc )
    {
        clLogError(CL_CKPT_AREA_PEER, CL_CKPT_CTX_CKPT_OPEN, 
                   "Idl handle updation failed rc [0x %x]", rc);
        return rc;
    }
    secInfo.secId.id = pSecId->id;
    secInfo.secId.idLen = pSecId->idLen;

    memcpy( &version, gCkptSvr->versionDatabase.versionsSupported,
            sizeof(ClVersionT));
    rc = VDECL_VER(clCkptRemSvrSectionInfoUpdateClientAsync, 4, 0, 0)(gCkptSvr->ckptIdlHdl, 
                                                  &version,
                                                  ckptHdl, 
                                                  CKPT_SECTION_CREAT,
                                                  &secInfo,
                                                  NULL, NULL);
    if( CL_OK != rc )
    {
        clLogError(CL_CKPT_AREA_PEER, CL_CKPT_CTX_CKPT_OPEN, 
                   "Section creation info get from active failed rc [0x %x]", rc);
    }
    return rc;
}

ClRcT
clCkptClntWriteNotify(CkptT                   *pCkpt,
                      ClCkptIOVectorElementT  *pVec,
                      ClUint32T               numSections,
                      ClIocNodeAddressT       nodeAddress,
                      ClIocPortT              portId)
{
    ClRcT             rc     = CL_OK;
    ClCntNodeHandleT  node   = CL_HANDLE_INVALID_VALUE;
    ClCntKeyHandleT   key    = CL_HANDLE_INVALID_VALUE;
    ClCntDataHandleT  data   = CL_HANDLE_INVALID_VALUE;
    ClIdlHandleT      idlHdl = CL_HANDLE_INVALID_VALUE;
    ClCkptAppInfoT   *pAappInfo = NULL;
    ClUint32T        doSend  = 0;

    rc = clCkptAppIdlHandleInit(&idlHdl);
    if( CL_OK != rc )
    {
        clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_SEC_OVERWRITE, 
                   "Idl handle updation failed rc[0x %x]", rc);
        goto exitOnError;
    }
    rc = clCntFirstNodeGet(pCkpt->pCpInfo->appInfoList, &node);
    if( CL_OK != rc )
    {
        clLogError(CL_CKPT_AREA_PEER, CL_CKPT_CTX_REPL_UPDATE,
                   "Failed to get app info from list rc[0x %x]", rc);
        goto finNexit;
    }
    while( node != 0 )
    {
        rc = clCntNodeUserKeyGet(pCkpt->pCpInfo->appInfoList, node, 
                                 &key);
        if( CL_OK != rc )
        {
            clLogError(CL_CKPT_AREA_PEER, CL_CKPT_CTX_REPL_UPDATE,
                    "Failed to get app info from list rc[0x %x]", rc);
            goto finNexit;
        }
        pAappInfo = (ClCkptAppInfoT *)(ClWordT) key;
        rc = clCntNodeUserDataGet(pCkpt->pCpInfo->appInfoList, node,
                                  &data);
        if( CL_OK != rc )
        {
            clLogError(CL_CKPT_AREA_PEER, CL_CKPT_CTX_REPL_UPDATE,
                    "Failed to get app info from list rc[0x %x]", rc);
            goto finNexit;
        }
        doSend = *((ClUint32T *)(ClWordT) data);

        /*
         * If the application is running on local node and its not the one who
         * wrote this info, then notify that particular application
         * about this overwrite info 
         */
        if( (doSend) && (pAappInfo->nodeAddress == gCkptSvr->localAddr) && 
            (nodeAddress != pAappInfo->nodeAddress || portId != pAappInfo->portId) )
        {
            rc = clCkptAppIdlHandleUpdate(idlHdl, pAappInfo->nodeAddress,
                                        pAappInfo->portId, 0);
            if( CL_OK != rc )
            {
                clLogError(CL_CKPT_AREA_PEER, CL_CKPT_CTX_REPL_UPDATE,
                        "Failed to get app info from list rc[0x %x]", rc);
                goto finNexit;
            }
            VDECL_VER(clCkptWriteUpdationNotificationClientAsync, 4, 0, 0)(idlHdl,  
                                          &pCkpt->ckptName, 
                                          numSections, 
                                          pVec, 
                                          NULL, NULL);
        }
        rc = clCntNextNodeGet(pCkpt->pCpInfo->appInfoList, node, &node);
        if( CL_OK != rc )
        {
            rc = CL_OK;
            break;
        }
    }
finNexit:
    (void)clIdlHandleFinalize(idlHdl);
exitOnError:    
    return rc;
}

ClRcT
clCkptClntSecOverwriteNotify(CkptT             *pCkpt,
                             ClCkptSectionIdT  *pSecId,
                             CkptSectionT      *pSec,
                             ClIocNodeAddressT nodeAddr,
                             ClIocPortT        portId)
{
    ClRcT             rc         = CL_OK;
    ClCntNodeHandleT  node       = CL_HANDLE_INVALID_VALUE;
    ClCntKeyHandleT   key        = CL_HANDLE_INVALID_VALUE;
    ClCntDataHandleT  data       = CL_HANDLE_INVALID_VALUE;
    ClIdlHandleT      idlHdl     = CL_HANDLE_INVALID_VALUE;
    ClCkptAppInfoT    *pAappInfo = NULL;
    ClUint32T        doSend      = 0;

    rc = clCkptAppIdlHandleInit(&idlHdl);
    if( CL_OK != rc )
    {
        clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_SEC_OVERWRITE, 
                   "Idl handle updation failed rc[0x %x]", rc);
        goto exitOnError;
    }
    rc = clCntFirstNodeGet(pCkpt->pCpInfo->appInfoList, &node);
    if( CL_OK != rc )
    {
        clLogError(CL_CKPT_AREA_PEER, CL_CKPT_CTX_REPL_UPDATE,
                   "Failed to get app info from list rc[0x %x]", rc);
        goto finNexit;
    }
    while( node != 0 )
    {
        rc = clCntNodeUserKeyGet(pCkpt->pCpInfo->appInfoList, node, 
                                 &key);
        if( CL_OK != rc )
        {
            clLogError(CL_CKPT_AREA_PEER, CL_CKPT_CTX_REPL_UPDATE,
                    "Failed to get app info from list rc[0x %x]", rc);
            goto finNexit;
        }
        pAappInfo = (ClCkptAppInfoT *)(ClWordT) key;

        rc = clCntNodeUserDataGet(pCkpt->pCpInfo->appInfoList, node,
                                  &data);
        if( CL_OK != rc )
        {
            clLogError(CL_CKPT_AREA_PEER, CL_CKPT_CTX_REPL_UPDATE,
                    "Failed to get app info from list rc[0x %x]", rc);
            goto finNexit;
        }
        doSend = *((ClUint32T *)(ClWordT) data);

        /*
         * If the application is running on local node and its not the one who
         * wrote this info, then notify that particular application
         * about this overwrite info 
         */
        if( (doSend) && (pAappInfo->nodeAddress == gCkptSvr->localAddr) &&
            (nodeAddr != pAappInfo->nodeAddress || portId != pAappInfo->portId) )
        {
            clLogTrace(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_REPL_UPDATE, 
                       "Sending notification to application [%d:%d]...",
                        pAappInfo->nodeAddress, pAappInfo->portId);
            rc = clCkptAppIdlHandleUpdate(idlHdl, pAappInfo->nodeAddress, 
                                          pAappInfo->portId, 0);
            if( CL_OK != rc )
            {
                clLogError(CL_CKPT_AREA_PEER, CL_CKPT_CTX_REPL_UPDATE,
                        "Failed to get app info from list rc[0x %x]", rc);
                goto finNexit;
            }
            VDECL_VER(clCkptSectionUpdationNotificationClientAsync, 4, 0, 0)(idlHdl, 
                    &pCkpt->ckptName, 
                    pSecId,
                    pSec->size,
                    pSec->pData,
                    NULL, NULL);
        }
        rc = clCntNextNodeGet(pCkpt->pCpInfo->appInfoList, node, &node);
        if( CL_OK != rc )
        {
            rc = CL_OK;
            break;
        }
    }
finNexit:
    (void)clIdlHandleFinalize(idlHdl);
exitOnError:    
    return rc;
}

ClRcT
VDECL_VER(clCkptCheckpointReplicaRemove, 4, 0, 0)(ClHandleT  ckptHdl, 
                              ClUint32T  localAddr)
{
    ClRcT              rc          = CL_OK;
    CkptT              *pCkpt      = NULL;
    ClUint32T          cksum       = 0;
    ClOsalMutexIdT     ckptMutex   = CL_HANDLE_INVALID_VALUE;
    ClNameT            ckptName    = {0};

    /*
     * Check whether the server is fully up or not.
     */
    CL_CKPT_SVR_EXISTENCE_CHECK;

    /*
     * Lock the active server ds
     */
    CKPT_LOCK(gCkptSvr->ckptActiveSem);

    /*
     * Retrieve the data associated with the active handle.
     */
    rc = ckptSvrHdlCheckout(gCkptSvr->ckptHdl,ckptHdl,(void **)&pCkpt);
    CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_DEBUG_ERROR,
              ("Failed to delete the checkpoint rc[0x %x]\n",
                     rc), rc);
    /*
     * Lock the checkpoint's mutex.
     */
    CKPT_LOCK(pCkpt->ckptMutex);           
    if(!pCkpt->ckptMutex)
    {
        clHandleCheckin(gCkptSvr->ckptHdl, ckptHdl);
        CKPT_UNLOCK(gCkptSvr->ckptActiveSem);
        clLogWarning("REP", "REM", "Ckpt with handle [%#llx] already deleted", ckptHdl);
        return CL_OK;
    }

    /*
     * to notify container destroy callback that, this function will take care
     * of deleting the mutex variable, this variable is being copied to local
     * variable.
     */
    ckptMutex = pCkpt->ckptMutex; 
    pCkpt->ckptMutex = CL_HANDLE_INVALID_VALUE;

    /*
     * Delete the checkpoint's entry from the handle list.
     */
    clCksm32bitCompute ((ClUint8T *)pCkpt->ckptName.value,
                        pCkpt->ckptName.length, &cksum);

    /* Just copying the data for debug */
     ckptName.length = pCkpt->ckptName.length;
     memcpy(ckptName.value, pCkpt->ckptName.value, ckptName.length);

     rc =  _ckptPresenceListPeerUpdate(pCkpt, ckptHdl, localAddr);
     if( CL_OK != rc )
     {
         /* Failed to update the peers, so not proceeding becuase this would
          * lead to have inconsistent data
          */
         clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_DEL,
                    "Failed to update the peers rc [0x %x]", rc);
         goto exitOnError;
     }
    
    rc = clCntNonUniqueKeyDelete(gCkptSvr->ckptHdlList,
                                 (ClCntKeyHandleT)(ClWordT)cksum,
                                 (ClPtrT)&ckptName, ckptHdlNonUniqueKeyCompare);    
    if( CL_OK != rc )
    {
        clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_DEL, 
                  "Failed to remove the ckptHdl [%#llX] from hdl list rc[0x %x]",
                  ckptHdl, rc);
    }

    /*
     * Unlock and delete the checkpoint's mutex.
     */
    CKPT_UNLOCK(ckptMutex);           
    clOsalMutexDelete(ckptMutex);
    clHandleCheckin(gCkptSvr->ckptHdl,ckptHdl);
    CKPT_UNLOCK(gCkptSvr->ckptActiveSem);

    clLogInfo(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_DEL,
        "Checkpoint [%.*s] replica has been removed from replicaAddr [%d]",
        ckptName.length, ckptName.value, localAddr);
    return rc;

exitOnError:
    /*
     * Unlock the checkpoint's mutex.
     */
    CKPT_UNLOCK(ckptMutex);           
    
    clHandleCheckin(gCkptSvr->ckptHdl,ckptHdl);

exitOnErrorBeforeHdlCheckout:

    CKPT_UNLOCK(gCkptSvr->ckptActiveSem);
    return rc;
}

ClRcT
clCkptActiveAppInfoUpdate(CkptT              *pCkpt,
                          ClIocNodeAddressT  appAddr,
                          ClIocPortT         appPort,
                          ClUint32T          **ppData)
{
    ClRcT           rc        = CL_OK;
    ClCkptAppInfoT  *pAppInfo = {0};

    pAppInfo = clHeapCalloc(1, sizeof(ClCkptAppInfoT));
    if( NULL == pAppInfo )
    {
        clLogCritical(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN,
                      "Memory allocation failed while updating app info");
        return CL_CKPT_ERR_NO_MEMORY;
    }
    *ppData = clHeapCalloc(1, sizeof(ClUint32T));
    if( NULL == *ppData )
    {
        clLogCritical(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN,
                      "Memory allocation failed while updating app info");
        clHeapFree(pAppInfo);
        return CL_CKPT_ERR_NO_MEMORY;
    }
    pAppInfo->nodeAddress = appAddr;
    pAppInfo->portId   = appPort;
    **ppData = 0;

    rc = clCntNodeAdd(pCkpt->pCpInfo->appInfoList, (ClCntKeyHandleT)(ClWordT)
            pAppInfo, *ppData, NULL);
    if( CL_OK == rc )
    {
        clLogDebug(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN, 
                   "Appinfo [%d:%d] has been added on nodeAddr [%d]"
                   "for checkpoint [%.*s]", appAddr, appPort, gCkptSvr->localAddr,
                   pCkpt->ckptName.length, pCkpt->ckptName.value);
        return CL_OK;
    }
    clHeapFree(*ppData);
    clHeapFree(pAppInfo);
    /* no problem if the info is already there */
    if( CL_GET_ERROR_CODE(rc) == CL_ERR_DUPLICATE ) 
    {
        rc = CL_OK;
    }
    if( CL_OK != rc )
    {
        clLogCritical(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN,
                      "While adding appInfo [%d:%d] to ckpt [%.*s] failed"
                      "rc [0x %x]", appAddr, appPort, pCkpt->ckptName.length,
                      pCkpt->ckptName.value, rc);
    }
        
    return rc;
}


ClRcT
VDECL_VER(clCkptReplicaAppInfoNotify, 4, 0, 0)(ClCkptHdlT  ckptHdl, 
                           ClIocNodeAddressT  nodeAddr, 
                           ClIocPortT         portId)
{
    ClRcT  rc     = CL_OK;
    CkptT  *pCkpt = NULL;
    ClUint32T *pData = NULL;

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
        clLogWarning("APP", "NOTIFY", "Ckpt with handle [%#llx] already deleted", ckptHdl);
        return CL_OK;
    }

    rc = clCkptActiveAppInfoUpdate(pCkpt, nodeAddr, portId, &pData);
    if( CL_OK != rc )
    {
        goto exitOnError;
    }
    clLogDebug(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN,
               "App info got updated for masterHdl [%#llX]", 
               ckptHdl);
exitOnError:
    /*
     * Unlock the checkpoint's mutex.
     */
    CKPT_UNLOCK(pCkpt->ckptMutex);           
    
    clHandleCheckin(gCkptSvr->ckptHdl,ckptHdl);
exitOnErrorBeforeHdlCheckout:
    return rc;
}

/*
 * Called whenever the peer server is going down.
 */
ClRcT  VDECL_VER(clCkptRemSvrBye, 4, 0, 0)(ClVersionT         version,
                       ClIocNodeAddressT  peerAddr,
                       ClIocPortT         portId,
                       ClUint32T          flag)
{
    ClRcT    rc = CL_OK;
    
    /*
     * Check whether the server is fully up or not.
     */
    CL_CKPT_SVR_EXISTENCE_CHECK; 
    
    /*
     * Version verification.
     */
    rc = clVersionVerify(&gCkptSvr->versionDatabase,&version);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Ckpt: VersionMismatch rc[0x %x]\n", rc), rc);

    /*
     * Call peer down with CL_CKPT_SVR_DOWN flag.
     */
    ckptPeerDown(peerAddr, flag, portId); 
exitOnError:    
    return rc;
}

/*
 * Called with the ckpt mutex held
 */
ClRcT
clCkptSectionLevelDelete(ClCkptHdlT        ckptHdl,
                         CkptT             *pCkpt, 
                         ClCkptSectionIdT  *pSecId,
                         ClBoolT           srcClient)
{
    ClRcT      rc        = CL_OK;
    ClUint32T  peerCount = 0;
    ClBoolT sectionLockTaken = CL_TRUE;

    /* take the section level mutex */
    rc = clCkptSectionLevelLock(pCkpt, pSecId, &sectionLockTaken);
    if( CL_OK != rc )
    {
        return rc;
    }

    /* 
     * Delete the section.
     */
    rc = clCkptSecFindNDelete(pCkpt, pSecId);
    if( CL_OK != rc )
    {
        clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN, 
                "Deletion of section [%.*s] of ckpt [%.*s] failed", 
                pSecId->idLen, pSecId->id, pCkpt->ckptName.length,
                pCkpt->ckptName.value);
        clCkptSectionLevelUnlock(pCkpt, pSecId, sectionLockTaken);
        return rc;
    }
    pCkpt->pDpInfo->numScns--;
             
    /* 
     * Inform replica nodes.
     */
    if( srcClient == CL_TRUE )
    {
        clCntSizeGet(pCkpt->pCpInfo->presenceList,&peerCount);
        if(peerCount > 1)
        {
            rc = clCkptRemSvrSectionDelete(ckptHdl, pCkpt, pSecId);
            if( CL_OK != rc )
            {
                /*FIXME -in case of Sync checkpointing revert back */
            }
        }
    }
    clLogInfo(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN, 
              "Section [%.*s] of ckpt [%.*s] has been deleted", 
              pSecId->idLen, pSecId->id, pCkpt->ckptName.length,
              pCkpt->ckptName.value);
    /* release section level mutex */
    clCkptSectionLevelUnlock(pCkpt, pSecId, sectionLockTaken);
    return rc;
}

/*
 * Called with ckpt server activeSem held.
 */
ClRcT
clCkptSvrReplicaDelete(CkptT       *pCkpt, 
                       ClCkptHdlT  ckptHdl,
                       ClBoolT     isActive)
{
    ClRcT              rc          = CL_OK;
    ClCkptSectionKeyT  *pSecKey    = NULL;
    ClUint32T          cksum       = 0;
    ClOsalMutexIdT     ckptMutex   = CL_HANDLE_INVALID_VALUE; 
    ClCntNodeHandleT   nodeHdl     = CL_HANDLE_INVALID_VALUE;
    ClCntDataHandleT   dataHdl     = 0;
    ClCntNodeHandleT   tempNodeHdl = CL_HANDLE_INVALID_VALUE;
    ClUint32T          index       = 0;
    ClVersionT         ckptVersion = {0};
    ClIocNodeAddressT  peerAddr    = 0;
    ClBoolT sectionLockTaken = CL_TRUE;

    /* acquire the mutex */
    CKPT_LOCK(pCkpt->ckptMutex);
    if(!pCkpt->ckptMutex)
    {
        clLogWarning("REP", "DEL", "Ckpt with handle [%#llx] already deleted", ckptHdl);
        return CL_OK;
    }

    /* walk thru section table */
    rc = clCntFirstNodeGet(pCkpt->pDpInfo->secHashTbl, &nodeHdl);
    if( CL_OK != rc )
    {
        /* no section exist, just delete the ckpt*/
        goto freeCkpt;
    }
    while( nodeHdl != CL_HANDLE_INVALID_VALUE )
    {
        tempNodeHdl = CL_HANDLE_INVALID_VALUE;
        rc = clCntNodeUserKeyGet(pCkpt->pDpInfo->secHashTbl, nodeHdl, (ClCntKeyHandleT *)(ClWordT)&pSecKey);
        if( CL_OK != rc )
        {
            goto freeCkpt;
        }
        /* take a section level lock */
        rc = clCkptSectionLevelLock(pCkpt, &pSecKey->scnId, &sectionLockTaken);
        if( CL_OK != rc )
        {
            goto freeCkpt;
        }
        if( pCkpt->numMutex != 0 )
        {
            index = clCkptSectionIndexGet(pCkpt, &pSecKey->scnId); 
            CL_ASSERT(index != -1);
        }
        if(sectionLockTaken == CL_TRUE)
        {
            /* release ckpt lock */
            CKPT_UNLOCK(pCkpt->ckptMutex);
        }
        /* get the next node & delete this node */
        clCntNextNodeGet(pCkpt->pDpInfo->secHashTbl, nodeHdl, &tempNodeHdl);
        clCntNodeDelete(pCkpt->pDpInfo->secHashTbl, nodeHdl);
        /* release the section level mutex & acquire ckpt mutex */
        nodeHdl = tempNodeHdl;
        if( sectionLockTaken == CL_TRUE 
            &&
            pCkpt->secMutex[index] != CL_HANDLE_INVALID_VALUE )
            clOsalMutexUnlock(pCkpt->secMutex[index]);
        
        if(sectionLockTaken == CL_TRUE)
        {
            CKPT_LOCK(pCkpt->ckptMutex);
        }
    }
    /* freed all sectionsi, if the call is from active, then inform peers */
   
    if( isActive == CL_TRUE )
    {
        /*
         * Walk through the presence list and update all the replica
         * nodes present in the checkpoint's presence list.
         */
        nodeHdl = CL_HANDLE_INVALID_VALUE;
        clCntFirstNodeGet (pCkpt->pCpInfo->presenceList, &nodeHdl);
        while (nodeHdl != 0)
        {
            tempNodeHdl = 0;
            rc = clCntNodeUserKeyGet (pCkpt->pCpInfo->presenceList, nodeHdl, &dataHdl);
            if( CL_OK != rc )
            {
                goto freeCkpt;
            }
            peerAddr = (ClIocNodeAddressT)(ClWordT) dataHdl;
            clCntNextNodeGet(pCkpt->pCpInfo->presenceList, nodeHdl, &tempNodeHdl);

            /*
             * Send the RMD to the replica node.
             */
            if (peerAddr != gCkptSvr->localAddr)
            {
                clLogDebug(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_DEL, 
                        "Deleting the replica on node address [%d]", 
                        peerAddr);
                rc = ckptIdlHandleUpdate(peerAddr,gCkptSvr->ckptIdlHdl,0);
                if( CL_OK != rc )
                {
                    goto freeCkpt;
                }
                memcpy(&ckptVersion, gCkptSvr->versionDatabase.versionsSupported,
                   sizeof(ClVersionT));
                rc = VDECL_VER(clCkptRemSvrCkptDeleteClientAsync, 4, 0, 0)( gCkptSvr->ckptIdlHdl,
                        &ckptVersion,
                        ckptHdl,NULL,0);
                if( CL_GET_ERROR_CODE(rc) == CL_IOC_ERR_COMP_UNREACHABLE || CL_IOC_ERR_HOST_UNREACHABLE == CL_GET_ERROR_CODE(rc) )
                {
                    clLogWarning(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_DEL, 
                            "Node addr [%d] does not exist, deleting from list...",
                            peerAddr);
                    clCntNodeDelete(pCkpt->pCpInfo->presenceList, nodeHdl);
                    rc = CL_OK;
                }
            }
            nodeHdl = tempNodeHdl;
        }
    }
freeCkpt:    
    ckptMutex = pCkpt->ckptMutex;
    pCkpt->ckptMutex = CL_HANDLE_INVALID_VALUE;
    
    /*
     * Locate and delete the handle from the ckpt handle list.
     */
    clCksm32bitCompute ((ClUint8T *)pCkpt->ckptName.value,
                        pCkpt->ckptName.length, &cksum);
    
    rc = clCntNonUniqueKeyDelete(gCkptSvr->ckptHdlList, (ClCntKeyHandleT)(ClWordT)cksum,
                                 (ClPtrT)&pCkpt->ckptName, ckptHdlNonUniqueKeyCompare);
                                    
    /*
     * Unlock and delete the checkpoint's mutex.
     */
    CKPT_UNLOCK(ckptMutex);
    clOsalMutexDelete(ckptMutex);
    /* return */
    return rc;
}
