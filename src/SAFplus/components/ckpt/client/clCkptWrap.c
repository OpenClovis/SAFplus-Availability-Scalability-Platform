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
/***************************************************************************
 * ModuleName  : ckpt                                                          
 * File        : clCkptWrap.c
 ****************************************************************************/

/*****************************************************************************
* Description :                                                               
*
*   This file contains clovis checkpoint service related APIs
*
*
*****************************************************************************/
#include  <netinet/in.h>
#include  <string.h>
#include  <unistd.h>
#include  <clCommon.h>
#include  <clIocApi.h>
#include <clIocIpi.h>
#include  <clLogApi.h>
#include  <clHandleApi.h>
#include  <clCkptCommon.h>
#include  <clCkptUtils.h>
#include  <clCkptClient.h>
#include <clCkptIpi.h>
#include  <clCpmApi.h>
#include  "ckptEockptServerCliServerFuncClient.h"
#include  "ckptEockptServerExtServerFuncClient.h"
#include  "ckptEockptServerExtCliServerFuncClient.h"
#include  "ckptEockptServerExtCliServerFuncPeerClient.h"
#include "ckptEockptServerActivePeerClient.h"
#include "ckptClntEockptClntckptClntClient.h"
#include  "ckptClntEoServer.h"
#include  "ckptClntEoClient.h"
#include  "ckptEoClient.h"
#include <clDebugApi.h>
#include <clIocErrors.h>
#include <clCkptErrors.h>
#include <clNodeCache.h>

ClCkptClntInfoT  gClntInfo = {0};
static ClInt32T gClDifferentialCkpt = -1;
static ClCkptSectionIdT gClDefaultSection = {.idLen = sizeof("defaultSection"),
                                             .id = (ClUint8T*)"defaultSection"
};

/*
 * Supporetd version.
 */
static ClVersionT clVersionSupported[]=
{
    {'B',0x01 , 0x01}
};



/*
 * Version Database.
 */
static ClVersionDatabaseT versionDatabase=
{
    sizeof(clVersionSupported)/sizeof(ClVersionT),
    clVersionSupported
}; 

static ClRcT (*gClCkptReplicaChangeCallback)(const ClNameT *pCkptName, ClIocNodeAddressT replicaAddr);

static ClRcT ckptVerifyAndCheckout(ClCkptHdlT    ckptHdl, ClCkptSvcHdlT*      ckptSvcHdl, CkptInitInfoT  **pInitInfo)
{
  ClRcT rc;

  *ckptSvcHdl   = CL_CKPT_INVALID_HDL;
  *pInitInfo    = NULL;
    /*
     * Check whether the client library is initialized or not.
     */
  if( gClntInfo.ckptDbHdl == 0)
      {
        CKPT_DEBUG_E (("CKPT Client is not yet initialized"));
        return CL_CKPT_ERR_NOT_INITIALIZED;
      }

  /*
   * Get the service handle from the checkpoint handle. 
   * Information assocaited with the service handle will be 
   * used while contacting the server.
   */
  rc = ckptSvcHandleGet(ckptHdl,ckptSvcHdl);
  if(rc != CL_OK)
    {
      CKPT_DEBUG_E(("\nPassed handle %#llX is invalid\n", ckptHdl));
      return CL_CKPT_ERR_INVALID_HANDLE;
    }
    
  /* 
   * Checkout the data associated with the service handle. 
   */
  rc = ckptHandleCheckout(*ckptSvcHdl, CL_CKPT_SERVICE_HDL, (void **)pInitInfo);
  if(rc != CL_OK)
    {
      CKPT_DEBUG_E(("\nPassed handle %#llX is invalid\n", ckptHdl));
      return CL_CKPT_ERR_INVALID_HANDLE;
    }
  return CL_OK;
}


/*
 * Iteration handle related callback function.
 */
static void    ckptSecHdlDeleteCallback(ClCntKeyHandleT userKey, 
                                        ClCntDataHandleT userData)
{
    ClHandleT  ckptSecHdl = (ClHandleT)(ClWordT)userKey;
    clHandleDestroy(gClntInfo.ckptDbHdl, ckptSecHdl);
    return;
}



/*
 * Dequeue callback. The function is kept empty and read element is freed
 * after it is passed to the user. If free was done in this flow then the 
 * message couldnt have been passed to the user.
 */

static void ckptQueueCallback(ClQueueDataT userData)
{
    return;
}



/*
 * Selection object related queue delete callback function.
 */
 
static void ckptQueueDestroyCallback(ClQueueDataT userData)
{
    CkptQueueInfoT  *pInfo     = (CkptQueueInfoT *)userData;
    CkptOpenCbInfoT *pOpenInfo = NULL;
    CkptSyncCbInfoT *pSyncInfo = NULL;

    if(pInfo->callbackId == CL_CKPT_OPEN_CALLBACK)
    {
        pOpenInfo = (CkptOpenCbInfoT *)pInfo->pData ;
        clHeapFree(pOpenInfo);
    }
    else
    {
        pSyncInfo = (CkptSyncCbInfoT *)pInfo->pData ;
        clHeapFree(pSyncInfo);
    }
    
    clHeapFree(pInfo);
    return;
}



/*
 * Handle list key create function
 */
 
ClInt32T ckptHdlKeyCompare(ClCntKeyHandleT   key1,  
                           ClCntKeyHandleT   key2)
{
    return  ((ClWordT)key1 - (ClWordT)key2);
}


static void ckptEventCallback( ClEventSubscriptionIdT subscriptionId,
                     ClEventHandleT eventHandle, ClSizeT eventDataSize );



/*
 * Event callback functions.
 */
 
ClEventCallbacksT ckptEvtCallbacks =
{
    NULL,
    ckptEventCallback,
};



/*
 * Client side event channel name.
 */
 
ClNameT ckptSubChannelName = 
{
    sizeof(CL_CKPT_CLNTUPD_EVENT_CHANNEL)-1,
    CL_CKPT_CLNTUPD_EVENT_CHANNEL
};



/*
 * Checkpoint open related callback function that would be
 * invoked after asynchronous open passes.
 */
 
void  _ckptCheckpointOpenAsyncCallback(ClIdlHandleT ckptIdlHdl,
        ClVersionT                          *pVersion,
        ClCkptSvcHdlT                       ckptSvcHdl,
        ClNameT                             *pName,
        ClCkptCheckpointCreationAttributesT *pCreateAttr,
        ClCkptOpenFlagsT                    ckptOpenFlags,
        ClIocNodeAddressT                   localAddr,
        ClIocPortT                          localPort, 
        CkptHdlDbT                          *pHdlInfo,
        ClRcT                               retCode,
        void                                *pInitData)
{
    ClCkptHdlT       *pCkptHdl      = NULL;
    ClCkptHdlT       tempHdl      = CL_CKPT_INVALID_HDL;
    CkptInitInfoT    *pInitInfo    = NULL;
    ClUint32T        cksum         = 0;
    ClRcT            rc            = CL_OK;
    ClCharT          chr           = 'c';
    CkptQueueInfoT   *pQueueData   = NULL;
    CkptOpenCbInfoT  *pOpenCbInfo  = NULL;
    ClInvocationT    invocation    = *(ClInvocationT *)pInitData;

    /* 
     * Checkout the data associated with the service handle 
     */
    rc = ckptHandleCheckout(ckptSvcHdl, CL_CKPT_SERVICE_HDL,
            (void **)&pInitInfo);
    if( rc != CL_OK)
    {
        CKPT_DEBUG_E(("Passed Checkpoint service handle is invalid\n"));    
        return ;
    }       
    
    CL_ASSERT(pInitInfo != NULL);

    /*
     * Lock the mutex.
     */
    clOsalMutexLock(pInitInfo->ckptSvcMutex);
    
    if(retCode == CL_OK)
    {
        pCkptHdl = clHeapAllocate(sizeof(*pCkptHdl));

        /*
         * Checkpoint client has to generate and return pCkptHdl to
         * the caller.
         */
        ckptLocalHandleCreate(pCkptHdl);

        /*
         * Add pCkptHdl to the Handle list 
         */
        clCksm32bitCompute ((ClUint8T *)pName->value,
                pName->length, &cksum);
        clCntNodeAdd( gClntInfo.ckptHdlList,
                (ClCntKeyHandleT)(ClWordT)cksum,
                pCkptHdl,
                NULL);
        pHdlInfo->cksum = cksum ;                 
        clNameCopy(&pHdlInfo->ckptName, pName);
        pHdlInfo->openFlag = ckptOpenFlags;
        pHdlInfo->hdlType     = CL_CKPT_CHECKPOINT_HDL;
        ckptHandleInfoSet(*pCkptHdl,pHdlInfo);
        ckptSvcHandleSet(*pCkptHdl,ckptSvcHdl);
        tempHdl = *pCkptHdl;
    }

    /*
     * Unlock the mutex.
     */
    clOsalMutexUnlock(pInitInfo->ckptSvcMutex);
    
    /*
     * Selection object related processing.
     */
     
    /*
     * Lock the selection obj related mutex.
     */
    clOsalMutexLock(pInitInfo->ckptSelObjMutex);
    
    if( pInitInfo != NULL && pInitInfo->pCallback != NULL)
    {
        if(pInitInfo->cbFlag == CL_TRUE)
        {
            /*
             * User has called selection object get.
             */

            /*
             * Store the relevent info into the queue
             */
            pQueueData  = (CkptQueueInfoT *)clHeapAllocate(
                    sizeof(CkptQueueInfoT));
            memset(pQueueData , 0 ,sizeof(CkptQueueInfoT));

            pQueueData->callbackId = CL_CKPT_OPEN_CALLBACK;
            pOpenCbInfo = (CkptOpenCbInfoT *)clHeapAllocate(
                    sizeof(CkptOpenCbInfoT)); 
            if(pOpenCbInfo == NULL) 
            {
                CL_DEBUG_PRINT (CL_DEBUG_CRITICAL,
                        ("Ckpt:memory allocation is failed rc[0x %x]\n",rc));
                clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_CRITICAL,
                           CL_LOG_CKPT_LIB_NAME,
                           CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
                /*
                 * Unlock the selection obj related mutex.
                 */
                clOsalMutexUnlock(pInitInfo->ckptSelObjMutex);

                /* 
                 * Checkin the data associated with the service handle 
                 */
                clHandleCheckin(gClntInfo.ckptDbHdl,ckptSvcHdl);
                return;
            }
            memset(pOpenCbInfo , 0 ,sizeof(CkptOpenCbInfoT));
            pOpenCbInfo->invocation   = invocation;
            pOpenCbInfo->ckptHdl      = tempHdl;
            pOpenCbInfo->error        = retCode;
            pQueueData->pData         = (CkptOpenCbInfoT *)pOpenCbInfo; 
            rc = clQueueNodeInsert(pInitInfo->cbQueue,
                                   (ClQueueDataT)pQueueData);
            if(write(pInitInfo->writeFd,(void *)&chr, 1) != 1)
                clLogError("CKP", "OPEN", "Write to dispatch pipe returned [%s]",
                           strerror(errno));
        }	
        else
        {
            /*
             * User has NOT called selection object get. Invoke the callback
             * immediately.
             */
            pInitInfo->pCallback->checkpointOpenCallback(invocation,tempHdl,
                    retCode);
        }
    }

    /*
     * Unlock the selection obj related mutex.
     */
    clOsalMutexUnlock(pInitInfo->ckptSelObjMutex);
    
    /* 
     * Checkin the data associated with the service handle 
     */
    clHandleCheckin(gClntInfo.ckptDbHdl,ckptSvcHdl);
    return ;
}



/*
 * Function for validating checkpoint open related parameters.
 */
 
ClRcT  ckptOpenParamValidate(ClCkptOpenFlagsT  checkpointOpenFlags,
                             ClNameT           *pCkptName,
           ClCkptCheckpointCreationAttributesT *pCkptCreationAttributes)
{
    ClRcT rc  = CL_OK;
    
    /* 
     * Validate the name 
     */
    CKPT_DEBUG_T(("pCkptName: %p,checkpointOpenFlags: %d"
                   "pCkptCreationAttributes : %p\n",
                   (void *)pCkptName, checkpointOpenFlags, 
                   (void *)pCkptCreationAttributes));
    CL_CKPT_NAME_VALIDATE(pCkptName);    
    
    /* 
     * Validate open flags and creation attributes 
     */
    if (checkpointOpenFlags > CL_CKPT_MAX_FLAGS)
    {    
        CKPT_DEBUG_E(("Exit:Error \n"));
        return CL_CKPT_ERR_BAD_FLAG;
    }    
    if ((CL_CKPT_CHECKPOINT_CREATE ==
        (checkpointOpenFlags & CL_CKPT_CHECKPOINT_CREATE)) &&
        (NULL == pCkptCreationAttributes))
    {
        CKPT_DEBUG_E(("Exit:Error \n"));
         return CL_CKPT_ERR_NULL_POINTER;
    }
    if( CL_CKPT_CHECKPOINT_CREATE != 
        (checkpointOpenFlags & CL_CKPT_CHECKPOINT_CREATE) &&
        (NULL != pCkptCreationAttributes))
    {    
        CKPT_DEBUG_E(("Exit:Error \n"));
         return CL_CKPT_ERR_INVALID_PARAMETER;
    }     
    if( (CL_CKPT_CHECKPOINT_CREATE == (checkpointOpenFlags & CL_CKPT_CHECKPOINT_CREATE)))
    {
        if( (pCkptCreationAttributes->checkpointSize != 0) && (pCkptCreationAttributes->maxSectionSize != 0) )
        {
           if( pCkptCreationAttributes->checkpointSize > 
                   (pCkptCreationAttributes->maxSections * pCkptCreationAttributes->maxSectionSize))
           {
                 clLogError("CKP", "CLI", "Checkpoint size [%lld] is more than sections [%d] size [%lld]", 
                            pCkptCreationAttributes->checkpointSize, pCkptCreationAttributes->maxSections, 
                            pCkptCreationAttributes->maxSectionSize);
                 return CL_CKPT_ERR_INVALID_PARAMETER;
           } 
        }
    }     

    CKPT_DEBUG_T(("Exit :rc [0x %x]\n",rc));
    return CL_OK;
}

static void ckptClientCacheFree(CkptHdlDbT *pHdlInfo)
{
    if(pHdlInfo->clientList.pClientInfo)
    {
        clHeapFree(pHdlInfo->clientList.pClientInfo);
        pHdlInfo->clientList.pClientInfo = NULL;
    }
    pHdlInfo->clientList.numEntries = 0;
}

static void ckptClientCacheUpdate(CkptHdlDbT *pHdlInfo, ClIocAddressT *pAddress)
{
    if(pHdlInfo->clientList.numEntries > 0 && pHdlInfo->clientList.pClientInfo)
    {
        for(ClUint32T i = 0; i < pHdlInfo->clientList.numEntries; ++i)
        {
            if(pHdlInfo->clientList.pClientInfo[i].nodeAddress == pAddress->iocPhyAddress.nodeAddress
               &&
               (!pAddress->iocPhyAddress.portId 
                ||
                pHdlInfo->clientList.pClientInfo[i].portId == pAddress->iocPhyAddress.portId))
            {
                /*
                 * invalidate cache
                 */
                clLogDebug("CACHE", "UPD", "Invalidating ckpt [%.*s] client cache "
                           "on notification for comp [%d] at node [%d]", 
                           pHdlInfo->ckptName.length, pHdlInfo->ckptName.value,
                           pAddress->iocPhyAddress.portId, pAddress->iocPhyAddress.nodeAddress);
                clHeapFree(pHdlInfo->clientList.pClientInfo);
                pHdlInfo->clientList.pClientInfo = NULL;
                pHdlInfo->clientList.numEntries = 0;
                return;
            }
        }
    }
}

static ClRcT ckptHandleWalkCallback(ClCntKeyHandleT key,
                                    ClCntDataHandleT data,
                                    ClCntArgHandleT arg,
                                    ClUint32T dataLength)
{
    ClCkptHdlT *pCkptHdl = (ClCkptHdlT*)data;
    CkptHdlDbT *pHdlInfo = NULL;
    ClRcT rc = CL_OK;

    if(!data) return CL_CKPT_ERR_INVALID_HANDLE;
    
    rc = ckptHandleCheckout(*pCkptHdl, CL_CKPT_CHECKPOINT_HDL, (void**)&pHdlInfo);
    if(rc != CL_OK)
        return rc;

    if(pHdlInfo && pHdlInfo->clientList.numEntries > 0 && arg)
        ckptClientCacheUpdate(pHdlInfo, (ClIocAddressT*)arg);

    rc = clHandleCheckin(gClntInfo.ckptDbHdl, *pCkptHdl);
    return rc;
}

static void ckptNotificationCallback(ClIocNotificationIdT id,
                                     ClPtrT unused,
                                     ClIocAddressT *pAddress)
{
    if(id != CL_IOC_NODE_LEAVE_NOTIFICATION 
       &&
       id != CL_IOC_COMP_DEATH_NOTIFICATION
       &&
       id != CL_IOC_NODE_LINK_DOWN_NOTIFICATION)
        return;

    if(!gClntInfo.ckptSvcHdlCount || !gClntInfo.ckptDbHdl || !gClntInfo.ckptHdlList) 
        return;

    clOsalMutexLock(&gClntInfo.ckptClntMutex);
    /*
     * Recheck after grabbing the lock
     */
    if(!gClntInfo.ckptSvcHdlCount || !gClntInfo.ckptDbHdl || !gClntInfo.ckptHdlList) 
    {
        goto out_unlock;
    }
    clCntWalkFailSafe(gClntInfo.ckptHdlList, ckptHandleWalkCallback, pAddress, sizeof(*pAddress));
    
    out_unlock:
    clOsalMutexUnlock(&gClntInfo.ckptClntMutex);
}

/*
 * Function for opening a checkpoint.
 */
 
ClRcT ckptLocalCallForOpen(ClCkptSvcHdlT     ckptSvcHdl,
                           ClNameT           *pCkptName,
        ClCkptCheckpointCreationAttributesT  *pCheckpointCreationAttributes,
                           ClCkptOpenFlagsT  checkpointOpenFlags,
                           ClTimeT           timeout,
                           ClCkptHdlT        *pCkptHandle,
                           ClInvocationT     invocation,
                           ClInt32T          openType)
{
    ClRcT                 rc           = CL_OK;
    CkptInitInfoT        *pInitInfo    = NULL;
    ClUint32T             cksum        = 0;
    CkptHdlDbT            hdlInfo      = {0};
    ClVersionT            version      = {0};
    ClIocPortT            portId       = 0; 
    ClInvocationT         *pInvocation = NULL;
    ClCkptHdlT            *pCkptHdl     = NULL;
    ClCkptCheckpointCreationAttributesT ckptAttr = {0};
    ClInt32T              retryCount   = 0;
    ClTimerTimeOutT t = {.tsSec = 0, .tsMilliSec=250};

    /*
     * Validate all input variables
     */
    if (NULL == pCkptHandle) return CL_CKPT_ERR_NULL_POINTER;
    rc = ckptOpenParamValidate( checkpointOpenFlags, pCkptName, pCheckpointCreationAttributes);
    if( rc != CL_OK)
    {
        CKPT_DEBUG_E(("\nInvalid Parameter [0x%x]\n",rc));
        return rc;
    } 

    /* 
     * Checkout the data associated with the service handle 
     */
    rc = ckptHandleCheckout(ckptSvcHdl,CL_CKPT_SERVICE_HDL, (void **)&pInitInfo);
    if(rc != CL_OK)
    {
        CKPT_DEBUG_E(("Passed handle is invalid rc[0x %x]\n",rc));
        return rc;
    }   

    if(pCheckpointCreationAttributes != NULL)
    {
        memcpy( &ckptAttr,pCheckpointCreationAttributes,sizeof(ClCkptCheckpointCreationAttributesT));
    }

    /*
     * Lock the mutex.
     */
    clOsalMutexLock(pInitInfo->ckptSvcMutex);

    /* 
     * Update the idl handle with the destination address 
     */
    rc =  clCkptMasterAddressGet(&pInitInfo->mastNodeAddr);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Could not get master address during checkpoint open [rc = 0x%x]\n",rc));
        rc = CL_CKPT_ERR_TRY_AGAIN;
        goto exitOnError;
    }

    rc = ckptIdlHandleUpdate(pInitInfo->mastNodeAddr,pInitInfo->ckptIdlHdl, 0);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_INFO, 
            ("Checkpoint open failed rc[0x %x].  This is expected if the app is checking to see if the checkpoint was created by another node.",rc), rc);

    /* 
     * Fill the supported version information 
     */ 
    memcpy(&version,&clVersionSupported[0],sizeof(ClVersionT));

    clEoMyEoIocPortGet(&portId);
    if( openType == CL_CKPT_OPEN_SYNC)
    {
        /* 
         * This block is executed for checkpoint open of sync flavor.
         */
         
        /*
         * Send the call to the checkpoint server. Retry if the server is not fully up.
         */
        do
        {
            rc = VDECL_VER(clCkptMasterCkptOpenClientSync, 4, 0, 0)(
                    pInitInfo->ckptIdlHdl,
                    &version,
                    ckptSvcHdl,
                    (ClNameT *)pCkptName,
                    &ckptAttr,
                    checkpointOpenFlags,
                    clIocLocalAddressGet(),
                    portId,
                    &hdlInfo);
        }while((CL_GET_ERROR_CODE(rc) == CL_ERR_TRY_AGAIN || 
                CL_GET_ERROR_CODE(rc) == CL_ERR_TIMEOUT ||
                CL_GET_ERROR_CODE(rc) == CL_IOC_ERR_COMP_UNREACHABLE ||
                CL_GET_ERROR_CODE(rc) == CL_IOC_ERR_HOST_UNREACHABLE ||
                CL_GET_ERROR_CODE(rc) == CL_ERR_INVALID_HANDLE ||
                CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST) && 
               (retryCount++ < CL_CKPT_MAX_RETRY
                &&
                clOsalTaskDelay(t) == CL_OK));

        /* 
         * Check for the version mismatch 
         */
        if(version.releaseCode != '\0' && rc == CL_OK)
        {
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL,
                    CL_CKPT_LOG_4_CLNT_VERSION_NACK, "Open call failed",
                    version.releaseCode,version.majorVersion, 
                    version.minorVersion);
            rc = CL_CKPT_ERR_VERSION_MISMATCH;             
        }

    }          
    else
    {
        /* 
         * This block is executed for checkpoint open of async flavor.
         */
         
        /* 
         * If async open related callback is NULL, return ERROR 
         */
        if( pInitInfo->pCallback == NULL ||
                (pInitInfo->pCallback->checkpointOpenCallback == NULL))
        {      
            rc = CL_CKPT_ERR_INITIALIZED;
            CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
                    ("Ckpt: Ckpt open async failed, rc=[0x %x]\n",rc), rc);
        }          

        /* 
         * Create and fill the invocation parameter to be passed to
         * async open.
         */
        pInvocation  = (ClInvocationT *)clHeapAllocate(sizeof(ClInvocationT));
        if(pInvocation == NULL)
        {
            rc =  CL_CKPT_ERR_NO_MEMORY;
            clLogWrite(CL_LOG_HANDLE_APP,CL_LOG_CRITICAL,CL_LOG_CKPT_LIB_NAME,
                    CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
            CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
                    ("Ckpt: CheckpoinAsync open failed rc[0x %x]\n",rc), rc);
        }
        memset(pInvocation , 0 , sizeof(ClInvocationT));
        *pInvocation          = invocation;
        
        /*
         * Send the call to the checkpoint server.
         */
        rc = VDECL_VER(clCkptMasterCkptOpenClientAsync, 4, 0, 0)(pInitInfo->ckptIdlHdl,
                &version,
                ckptSvcHdl,
                (ClNameT *)pCkptName,
                &ckptAttr,
                checkpointOpenFlags,
                clIocLocalAddressGet(),
                portId,
                &hdlInfo,
                _ckptCheckpointOpenAsyncCallback,
                (void *)pInvocation);

    }
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_INFO, 
            ("Checkpoint open failed rc[0x %x].  This is expected if the app is checking to see if the checkpoint was created by another node.",rc), rc);

    if( openType == CL_CKPT_OPEN_SYNC)
    {
        /* 
         * This block is executed for checkpoint open of sync flavor.
         * Checkpoint client has to generate and return ckptHdl to
         * the caller.
         */
        pCkptHdl = clHeapAllocate(sizeof(*pCkptHdl)); // Free where necessary NTC
        if(pCkptHdl == NULL)
        {
            rc =  CL_CKPT_ERR_NO_MEMORY;
            clLogWrite(CL_LOG_HANDLE_APP,CL_LOG_CRITICAL,CL_LOG_CKPT_LIB_NAME,
                    CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
            CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
                    ("Ckpt: CheckpoinAsync open failed rc[0x %x]\n",rc), rc);
        }
        /*
         * Create the local handle and Update client database.
         * This would serve as ckptHdl.
         */
        rc = ckptLocalHandleCreate(pCkptHdl);
        CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
                ("Ckpt: CheckpointOpen failed rc[0x %x]\n",rc), rc);

        *pCkptHandle = *pCkptHdl; // Update the handle provided

        /*
         * Add the ckptHdl to the Handle list 
         */
        clCksm32bitCompute ((ClUint8T *)pCkptName->value,
                pCkptName->length, &cksum);
        rc = clCntNodeAdd( gClntInfo.ckptHdlList,
                (ClCntKeyHandleT)(ClWordT)cksum,
                pCkptHdl,
                NULL);
        CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
                ("Ckpt: CheckpointOpen failed rc[0x %x]\n",rc), rc);

        hdlInfo.prevMasterAddr = pInitInfo->mastNodeAddr;
        hdlInfo.ckptSvcHdl     = ckptSvcHdl;
        hdlInfo.openFlag       = checkpointOpenFlags;
        hdlInfo.cksum          = cksum;
        hdlInfo.hdlType        = CL_CKPT_CHECKPOINT_HDL;
        clNameCopy(&hdlInfo.ckptName, pCkptName);
        clLogNotice(CL_CKPT_AREA_CLIENT, CL_LOG_CONTEXT_UNSPECIFIED, 
                "Checkpoint [%.*s] is opened act address [%d]", 
                pCkptName->length, pCkptName->value, hdlInfo.activeAddr);
        /*
         * Associate the checkpoint info with the checkpoint handle.
         */
        rc = ckptHandleInfoSet(*pCkptHdl,&hdlInfo);
        CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
                ("Ckpt:CheckpointOpen failed rc[0x %x]\n",rc), rc);
    }          
exitOnError:
    {
        /*
         * Unlock the mutex.
         */
        clOsalMutexUnlock(pInitInfo->ckptSvcMutex);
        /*
         * Initialize the notification callbacks for ckpt client cache
         */
        if(!pInitInfo->ckptNotificationHandle 
           && 
           !gClntInfo.ckptClientCacheDisable
           &&
           !(ckptAttr.creationFlags & CL_CKPT_PEER_TO_PEER_CACHE_DISABLE))
        {
            ClIocPhysicalAddressT compAddr = {.nodeAddress = CL_IOC_BROADCAST_ADDRESS,
                                              .portId = 0
            };
            clCpmNotificationCallbackInstall(compAddr, ckptNotificationCallback,
                                             NULL, &pInitInfo->ckptNotificationHandle);
        }

        /* 
         * Checkin the data associated with the service handle 
         */
        clHandleCheckin(gClntInfo.ckptDbHdl,ckptSvcHdl);
        return rc;
    }
}



/*
 * Opens/creates a checkpoint synchronously
 */

ClRcT clCkptCheckpointOpen(
   ClCkptSvcHdlT                             ckptSvcHdl,
   const ClNameT                             *pCkptName, 
   const ClCkptCheckpointCreationAttributesT *pCheckpointCreationAttributes,
   ClCkptOpenFlagsT                          checkpointOpenFlags, 
   ClTimeT                                   timeout,            
   ClCkptHdlT                                *pCkptHdl)
{
    /* 
     * Following variable is needed to satisy func signature 
     */
    ClInvocationT  invocation  = 0;

    /*
     * Check whether the client library is initialized or not.
     */
    if( gClntInfo.ckptDbHdl == 0)
    {
        CKPT_DEBUG_E(("CKPT Client is not yet initialized"));
        return CL_CKPT_ERR_NOT_INITIALIZED;
    }
    
    return (ckptLocalCallForOpen(ckptSvcHdl,
                   (ClNameT *)pCkptName,
                   (ClCkptCheckpointCreationAttributesT *)
                             pCheckpointCreationAttributes,
                     checkpointOpenFlags,
                     timeout,pCkptHdl,
                     invocation,
                     CL_CKPT_OPEN_SYNC));
}



/* 
 * Opens/Create a checkpoint asynchronously
 */

ClRcT clCkptCheckpointOpenAsync(
        ClCkptSvcHdlT                             ckptSvcHdl,
        ClInvocationT                             invocation,
        const ClNameT                             *pCkptName,
        const ClCkptCheckpointCreationAttributesT *pCkptCreationAttributes,
        ClCkptOpenFlagsT                          checkpointOpenFlags)
{
    /* 
     * Following variables are needed to satisy func signature 
     */
    ClTimeT    timeout = 0;
    ClHandleT  ckptHdl = 0;
    
    /*
     * Check whether the client library is initialized or not.
     */
    if(gClntInfo.ckptDbHdl == 0)
    {
        CKPT_DEBUG_E(("CKPT Client is not yet initialized"));
        return CL_CKPT_ERR_NOT_INITIALIZED;
    }
    
    return(ckptLocalCallForOpen(ckptSvcHdl,
              (ClNameT *)pCkptName,
              (ClCkptCheckpointCreationAttributesT *)pCkptCreationAttributes,
              checkpointOpenFlags,
              timeout,&ckptHdl,
              invocation,
              CL_CKPT_OPEN_ASYNC));
}



/*
 *  Closes the checkpoint designated by the checkpointHandle
 *  If this checkpoint delete was requested and there are no other
 *  opens are pending on this checkpoint then it will be deleted.
 */

ClRcT clCkptCheckpointClose(ClCkptHdlT ckptHdl)
{
    ClRcT                 rc            = CL_OK;
    CkptInitInfoT         *pInitInfo    = NULL;
    ClCkptHdlT            ckptMastHdl   = CL_CKPT_INVALID_HDL;
    ClVersionT            version       = {0};
    ClCkptSvcHdlT         ckptSvcHdl    = CL_CKPT_INVALID_HDL;
    ClUint32T             numRetries    = 0;
    CkptHdlDbT            *pHdlInfo     = NULL;
    ClUint32T             cksum         = 0;
    ClCntKeyHandleT       tempsum       = 0;
    ClCntNodeHandleT      nodeHdl       = 0;
    ClCkptHdlT            *pStoredHdl    = NULL;

    if ((rc = ckptVerifyAndCheckout(ckptHdl, &ckptSvcHdl, &pInitInfo)) != CL_OK)
      {
        return rc;
      }

    /*
     * Lock the mutex.
     */
    clOsalMutexLock(&gClntInfo.ckptClntMutex);
    clOsalMutexLock(pInitInfo->ckptSvcMutex);

    /*
     * Update idl handle with master related info
     */
    rc = ckptMasterHandleGet(ckptHdl,&ckptMastHdl); 
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
              ("Ckpt: Passed handle %#llX is invalid rc[0x %x]\n",ckptHdl, rc), rc);
    rc =  clCkptMasterAddressGet(&pInitInfo->mastNodeAddr);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
          ("Ckpt: Failed in master Address Get rc[0x %x]\n",rc), rc);
    rc = ckptIdlHandleUpdate(pInitInfo->mastNodeAddr, pInitInfo->ckptIdlHdl,
                             CL_CKPT_NO_RETRY);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
              ("Ckpt: Idl Handle %#llX update error rc[0x %x]\n",ckptHdl, rc), rc);
          
    /* 
     * Fill the supported version information 
     */ 
    memcpy(&version,&clVersionSupported[0],sizeof(ClVersionT));
         
    /*
     * Send the call to the checkpoint master server.
     * Retry if the server is not fully up.
     */
    do
    {
        rc = VDECL_VER(clCkptMasterCkptCloseClientSync, 4, 0, 0)(pInitInfo->ckptIdlHdl, ckptMastHdl,
                                   clIocLocalAddressGet(),
                                   &version);
        if(numRetries > 0 && CL_GET_ERROR_CODE(rc) == CL_ERR_INVALID_HANDLE)
        {
            rc = CL_OK;
        }
    }while(CL_GET_ERROR_CODE(rc) == CL_ERR_TIMEOUT && numRetries++ < 2);  
    
    /* 
     * Check for the version mismatch 
     */
    if ((version.releaseCode != '\0') && (rc == CL_OK))
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL,
                   CL_CKPT_LOG_4_CLNT_VERSION_NACK, "Close call failed",
                   version.releaseCode,version.majorVersion, 
                   version.minorVersion);
          rc = CL_CKPT_ERR_VERSION_MISMATCH;             
    }
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
              ("Ckpt: Checkpoint close error rc[0x %x]\n",rc), rc);

    /* 
     * Checkout the data associated with the checkpoint handle 
     */
    rc = ckptHandleCheckout(ckptHdl, CL_CKPT_CHECKPOINT_HDL,
                            (void **)&pHdlInfo);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
              ("Ckpt: Checkpoint close error rc[0x %x]\n",rc), rc);

    /* 
     * Checkin the data associated with the checkpoint handle 
     */
    rc = clHandleCheckin(gClntInfo.ckptDbHdl,ckptHdl);
    
    /*
     * Delete the ckptHdl's entry from the Handle list.
     */
    cksum = pHdlInfo->cksum;          
    rc = clCntNodeFind(gClntInfo.ckptHdlList,(ClPtrT)(ClWordT)cksum,&nodeHdl);
    if( CL_OK != rc )
    {
        /* Just logging the error, continuing */
        clLogError(CL_CKPT_AREA_CLIENT, CL_LOG_CONTEXT_UNSPECIFIED, 
                "Unable to find handle for cksum [%d]", cksum);
    }
    tempsum = (ClPtrT)(ClWordT)cksum;
    while(nodeHdl != 0 && (ClWordT)tempsum == cksum)
    {
        /*
         * Associated with a given checkpoint, there can be multiple opens.
         * Find the appropriate ckptHdl and delete it.
         */
        rc = clCntNodeUserDataGet(gClntInfo.ckptHdlList, nodeHdl, 
                                  (ClCntDataHandleT *)&pStoredHdl);
        if( CL_OK != rc )
        {
            break;
        }
        if(*pStoredHdl == ckptHdl)
        {
            clCntNodeDelete(gClntInfo.ckptHdlList,nodeHdl);
            ckptClientCacheFree(pHdlInfo);
            break;
        }  
        clCntNextNodeGet(gClntInfo.ckptHdlList,nodeHdl,&nodeHdl);
        rc = clCntNodeUserKeyGet(gClntInfo.ckptHdlList,nodeHdl,
                            (ClCntKeyHandleT *)&tempsum);
        if( CL_OK != rc )
        {
            break;
        }
    }  

    /*
     * Destroy the checkpoint handle.
     */
    rc = ckptLocalHandleDelete(ckptHdl);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
              ("Ckpt: Checkpoint close error rc[0x %x]\n",rc), rc);
exitOnError:
    {

        /*
         * Unlock the mutex.
         */
        clOsalMutexUnlock(pInitInfo->ckptSvcMutex);
        clOsalMutexUnlock(&gClntInfo.ckptClntMutex);

        /* 
         * Checkin the data associated with the service handle 
         */
        clHandleCheckin(gClntInfo.ckptDbHdl,ckptSvcHdl);
        return rc;
    }
}



/*
 * Removes the checkpoint from the system if no other execution entity
 * has opened it.
 */
 
ClRcT clCkptCheckpointDelete(ClCkptSvcHdlT     ckptSvcHdl,
                             const ClNameT     *pCkptName)
{
    ClRcT              rc         = CL_OK;
    ClNameT            ckptName   = {0};
    CkptInitInfoT      *pInitInfo = NULL;
    ClVersionT         version    = {0};
    ClUint32T          numRetries = 0;
 
    /*
     * Check whether the client library is initialized or not.
     */
    if( gClntInfo.ckptDbHdl == 0)
    {
        CKPT_DEBUG_E(("CKPT Client is not yet initialized"));
        return CL_CKPT_ERR_NOT_INITIALIZED;
    }
    
    /* 
     * Validate the name.
     */
    CL_CKPT_NAME_VALIDATE(pCkptName);    
    clNameCopy(&ckptName, pCkptName);

    /* 
     * Checkout the data associated with the service handle.
     */
    rc = ckptHandleCheckout(ckptSvcHdl, CL_CKPT_SERVICE_HDL,
                            (void **)&pInitInfo);
    if(CL_OK != rc)
    { 
        CKPT_DEBUG_E(("\nPassed handle is invalid\n"));
        return rc;
    }    

    /*
     * Lock the mutex.
     */
    clOsalMutexLock(pInitInfo->ckptSvcMutex);
    
    /*
     * Update idl handle with master related info.
     */
    rc =  clCkptMasterAddressGet(&pInitInfo->mastNodeAddr);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
          ("Ckpt: Failed in master Address Get rc[0x %x]\n",rc), rc);
    rc = ckptIdlHandleUpdate(pInitInfo->mastNodeAddr, pInitInfo->ckptIdlHdl,
                             CL_CKPT_NO_RETRY);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
              ("Ckpt: Idl Handle update error rc[0x %x]\n",rc), rc);
              
    /* 
     * Fill the supported version information.
     */ 
    memcpy(&version,&clVersionSupported[0],sizeof(ClVersionT));
    
    /*
     * Send the call to the checkpoint master server.
     * Retry if the server is not fully up.
     */
    do
    {
        rc = VDECL_VER(clCkptMasterCkptUnlinkClientSync, 4, 0, 0)( pInitInfo->ckptIdlHdl,&ckptName, 
                                     clIocLocalAddressGet(),&version);
        if(numRetries > 0 && CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST)
        {
            rc = CL_OK;
        }
    }while( CL_GET_ERROR_CODE(rc) == CL_ERR_TIMEOUT && numRetries++ < 2); 

    /* 
     * Check for the version mismatch.
     */
    if(version.releaseCode != '\0' && rc == CL_OK)
    {
          clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL,
                    CL_CKPT_LOG_4_CLNT_VERSION_NACK, "Close call failed",
                    version.releaseCode,version.majorVersion, 
                    version.minorVersion);
          rc = CL_CKPT_ERR_VERSION_MISMATCH;             
    }
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
              ("Ckpt: Checkpoint delete error rc[0x %x]\n",rc), rc);
exitOnError:
    {
        /*
         * Unlock the mutex.
         */
        clOsalMutexUnlock(pInitInfo->ckptSvcMutex);
     
        /* 
         * Checkin the data associated with the service handle. 
         */
        clHandleCheckin(gClntInfo.ckptDbHdl,ckptSvcHdl);
        return rc;
    }
}



/*
 * Sets the retention duration of a checkpoint
 */
 
ClRcT clCkptCheckpointRetentionDurationSet(ClCkptHdlT ckptHdl,
                                           ClTimeT    retentionDuration)
{
    ClRcT              rc          = CL_OK;
    CkptInitInfoT      *pInitInfo  = NULL;
    ClCkptHdlT         mastHdl     = CL_CKPT_INVALID_HDL; 
    ClIdlHandleT       ckptIdlHdl  = CL_CKPT_INVALID_HDL;
    ClVersionT         version     = {0};
    ClCkptSvcHdlT      ckptSvcHdl  = CL_CKPT_INVALID_HDL;

    if ((rc = ckptVerifyAndCheckout(ckptHdl, &ckptSvcHdl, &pInitInfo)) != CL_OK)
      {
        return rc;
      }    

    /*
     * Lock the mutex.
     */
    clOsalMutexLock(pInitInfo->ckptSvcMutex);

    /*
     * Update idl handle with master related info
     */
    rc = ckptMasterHandleGet(ckptHdl,&mastHdl);    
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
              ("Ckpt: RetentionDurationSet failed, rc[0x %x]\n",rc), rc);
    rc =  clCkptMasterAddressGet(&pInitInfo->mastNodeAddr);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
          ("Ckpt: Failed in master Address Get rc[0x %x]\n",rc), rc);
    rc = ckptIdlHandleUpdate(pInitInfo->mastNodeAddr, pInitInfo->ckptIdlHdl,
                             CL_CKPT_MAX_RETRY);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
              ("Ckpt: RetentionDurationSet failed, rc[0x %x]\n",rc), rc);
              
    ckptIdlHdl = pInitInfo->ckptIdlHdl;    
    
    /* 
     * Fill the supported version information 
     */ 
    memcpy(&version,&clVersionSupported[0],sizeof(ClVersionT));
    
    /*
     * Send the call to the checkpoint master server.
     */
    rc = VDECL_VER(clCkptMasterCkptRetentionDurationSetClientSync, 4, 0, 0)(ckptIdlHdl,mastHdl,
                                              retentionDuration,
                                              &version);
                                              
    /* 
     * Check for the version mismatch 
     */
    if((version.releaseCode != '\0') && (CL_OK ==rc))
    {
          clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL,
                    CL_CKPT_LOG_4_CLNT_VERSION_NACK, "Close call failed",
                    version.releaseCode,version.majorVersion, 
                    version.minorVersion);
          rc = CL_CKPT_ERR_VERSION_MISMATCH;             
    }
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
              ("Ckpt: RetentionDurationSet failed, rc[0x %x]\n",rc), rc);
exitOnError:
    {
        /*
         * Unlock the mutex.
         */
        clOsalMutexUnlock(pInitInfo->ckptSvcMutex);

        /* 
         * Checkin the data associated with the service handle 
         */
        clHandleCheckin(gClntInfo.ckptDbHdl,ckptSvcHdl);
        return rc;
    }
}



/*
 * Sets the local replica to be active replica.
 */

ClRcT clCkptActiveReplicaSet(ClCkptHdlT ckptHdl)
{
    ClRcT               rc         = CL_OK;
    CkptInitInfoT       *pInitInfo = NULL;
    ClCkptHdlT          mastHdl    = CL_CKPT_INVALID_HDL; 
    ClIocNodeAddressT   nodeAddr   = 0;
    ClIdlHandleT        ckptIdlHdl = CL_CKPT_INVALID_HDL;
    ClVersionT          version    = {0};
    ClCkptSvcHdlT       ckptSvcHdl = CL_CKPT_INVALID_HDL;
    CkptHdlDbT          *pHdlInfo  = NULL;

    if ((rc = ckptVerifyAndCheckout(ckptHdl, &ckptSvcHdl, &pInitInfo)) != CL_OK)
      {
        return rc;
      }

    /*
     * Lock the mutex.
     */
    clOsalMutexLock(pInitInfo->ckptSvcMutex);
    
    /* 
     * Checkout the data associated with the checkpoint handle 
     */
    rc = ckptHandleCheckout(ckptHdl, CL_CKPT_CHECKPOINT_HDL, (void **)&pHdlInfo);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
              ("Ckpt: Handle create error rc[0x %x]\n",rc), rc);

    /*
     * Return ERROR if the checkpoint has not been opened with create or
     * write mode.
     */
    if(!((pHdlInfo->openFlag & CL_CKPT_CHECKPOINT_CREATE) ||
         (pHdlInfo->openFlag &  CL_CKPT_CHECKPOINT_WRITE)))
    {
        rc = CL_CKPT_ERR_OP_NOT_PERMITTED;
        clHandleCheckin(gClntInfo.ckptDbHdl,ckptHdl);
        CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR,
                ("Ckpt: Improper permissions as ckpt was not opened in write"
                 " or create mode, rc[0x %x]\n",rc), rc);
    }
    
    /* 
     * Checkin the data associated with the checkpoint handle 
     */
    rc = clHandleCheckin(gClntInfo.ckptDbHdl, ckptHdl);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
              ("Ckpt: Handle Check-in Error  rc[0x %x]\n",rc), rc);
    
    /*
     * Update idl handle with master related info.
     */
    rc = ckptMasterHandleGet(ckptHdl,&mastHdl);    
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, ("Ckpt:Master Handle Get error rc[0x %x]\n",rc), rc);
    rc =  clCkptMasterAddressGet(&pInitInfo->mastNodeAddr);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, ("Ckpt: Failed in master Address Get rc[0x %x]\n",rc), rc);
    rc = ckptIdlHandleUpdate(pInitInfo->mastNodeAddr, pInitInfo->ckptIdlHdl, CL_CKPT_MAX_RETRY);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, ("Ckpt: Idl Handle update error rc[0x %x]\n",rc), rc);
    ckptIdlHdl = pInitInfo->ckptIdlHdl;    
    
    nodeAddr  = clIocLocalAddressGet();
    
    /* 
     * Fill the supported version information 
     */ 
    memcpy(&version,&clVersionSupported[0],sizeof(ClVersionT));
    
    /*
     * Send the call to the checkpoint master server.
     */
    rc = VDECL_VER(clCkptMasterActiveReplicaSetClientSync, 4, 0, 0)(ckptIdlHdl,mastHdl,
                                      nodeAddr,
                                       &version);
    /* 
     * Check for the version mismatch 
     */
    if( version.releaseCode != '\0' && (rc == CL_OK) )
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL,
                   CL_CKPT_LOG_4_CLNT_VERSION_NACK, "Close call failed",
                   version.releaseCode, version.majorVersion, 
                   version.minorVersion);
          rc = CL_CKPT_ERR_VERSION_MISMATCH;             
    }
    
    if(rc == CL_OK)
    {
        /* 
         * Active replica set is successful. Update the info in
         * client ckpt database.
         */
         rc = ckptHandleCheckout(ckptHdl, CL_CKPT_CHECKPOINT_HDL,(void **)&pHdlInfo);
         if (rc == CL_OK)
           {
             pHdlInfo->activeAddr = clIocLocalAddressGet(); 
             rc = clHandleCheckin(gClntInfo.ckptDbHdl,ckptHdl);
             CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, ("Ckpt: Handle Check-in Error  rc[0x %x]\n",rc), rc);
           }
         else
           {
             clDbgCodeError(rc, ("Handle %#llX checkout failed", ckptHdl));
           }
    }
exitOnError:
    {
        /*
         * Unlock the mutex.
         */
        clOsalMutexUnlock(pInitInfo->ckptSvcMutex);

        /* 
         * Checkin the data associated with the service handle 
         */
        clHandleCheckin(gClntInfo.ckptDbHdl,ckptSvcHdl);
        return rc;
    }
}



/*
 * Sets the local replica to be active replica during switchover.
 * This function is meant to be used by cpm and ams.
 */
 
ClRcT clCkptActiveReplicaSetSwitchOver(ClCkptHdlT ckptHdl)
{
    ClRcT              rc         = CL_OK;
    CkptInitInfoT      *pInitInfo = NULL;
    ClCkptHdlT         mastHdl    = CL_CKPT_INVALID_HDL; 
    ClIocNodeAddressT  nodeAddr   = 0;
    ClIdlHandleT       ckptIdlHdl = CL_CKPT_INVALID_HDL;
    ClVersionT         version    = {0};
    CkptHdlDbT         *pHdlInfo  = NULL;
    ClCkptSvcHdlT      ckptSvcHdl = CL_CKPT_INVALID_HDL;

    if ((rc = ckptVerifyAndCheckout(ckptHdl, &ckptSvcHdl, &pInitInfo)) != CL_OK)
      {
        return rc;
      }

    /*
     * Lock the mutex.
     */
    clOsalMutexLock(pInitInfo->ckptSvcMutex);
              
    /*
     * Update idl handle with master related info.
     */
    rc = ckptMasterHandleGet(ckptHdl,&mastHdl);    
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
            ("Ckpt:Master Handle Get error rc[0x %x]\n",rc), rc);
    /*
     * Use local address since local node has become active from standby
     */
    nodeAddr = clIocLocalAddressGet();
    pInitInfo->mastNodeAddr = nodeAddr;
    rc = ckptIdlHandleUpdate(pInitInfo->mastNodeAddr, pInitInfo->ckptIdlHdl, 
                             CL_CKPT_MAX_RETRY);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
            ("Ckpt: Idl Handle update error rc[0x %x]\n",rc), rc);
    ckptIdlHdl = pInitInfo->ckptIdlHdl;    


    /* 
     * Fill the supported version information 
     */ 
    memcpy(&version,&clVersionSupported[0],sizeof(ClVersionT));

    /*
     * Send the call to the checkpoint master server.
     */
    rc = VDECL_VER(clCkptMasterActiveReplicaSetSwitchOverClientSync, 4, 0, 0)(ckptIdlHdl,mastHdl,
            nodeAddr,
            &version);
    /* 
     * Check for the version mismatch 
     */
    if((version.releaseCode != '\0')  && (rc == CL_OK))
    {
        rc = CL_CKPT_ERR_VERSION_MISMATCH;    
    }
    if(rc == CL_OK)
      {
        /* 
         * Active replica set is successful. Update the info in
         * client ckpt database.
         */
        rc = ckptHandleCheckout(ckptHdl, CL_CKPT_CHECKPOINT_HDL,(void **)&pHdlInfo);
        if (rc == CL_OK)
          {
            pHdlInfo->activeAddr = nodeAddr; 
            rc = clHandleCheckin(gClntInfo.ckptDbHdl,ckptHdl);
            CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, ("Ckpt: Handle Check-in Error  rc[0x %x]\n",rc), rc);
          }
        else
          {
            clDbgCodeError(rc,("Handle %#llX checkout failed", ckptHdl));
          }
      }
exitOnError:
    {
        /*
         * Unlock the mutex.
         */
        clOsalMutexUnlock(pInitInfo->ckptSvcMutex);

        /* 
         * Checkin the data associated with the service handle 
         */
        clHandleCheckin(gClntInfo.ckptDbHdl,ckptSvcHdl);
        return rc;
    }
}



/*
 * Function to get the active address of the checkpoint from the master.
 */
 
ClRcT _ckptMasterActiveAddressGet(ClCkptHdlT         ckptHdl,
                                  ClIocNodeAddressT* pNodeAddr)
{
    ClRcT              rc          = CL_OK;
    ClRcT              rc2         = CL_OK;
    ClCkptHdlT         mastHdl     = CL_CKPT_INVALID_HDL;
    CkptHdlDbT         *pHdlInfo   = NULL;
    ClIocNodeAddressT  mastAddr    = 0;
    ClCkptSvcHdlT      ckptSvcHdl  = CL_CKPT_INVALID_HDL;
    CkptInitInfoT      *pInitInfo  = NULL;
    ClVersionT         ckptVersion = {0};
    
    *pNodeAddr = CL_CKPT_UNINIT_VALUE;
    
    /*
     * Get the master address and the active handle associated with 
     * the ckptHdl.
     */
    rc =  clCkptMasterAddressGet(&mastAddr);
    if( CL_OK != rc )
    {
        clLogError(CL_CKPT_AREA_CLIENT, CL_LOG_CONTEXT_UNSPECIFIED, 
                "Failed to get the master address rc[0x %x]", rc);
        return rc;
    }
    rc = ckptActiveHandleGet(ckptHdl, &mastHdl);
    if( CL_OK != rc )
    {
        clLogError(CL_CKPT_AREA_CLIENT, CL_LOG_CONTEXT_UNSPECIFIED, 
                "Failed to get the master address rc[0x %x]", rc);
        return rc;
    }
    
    if ((rc = ckptVerifyAndCheckout(ckptHdl, &ckptSvcHdl, &pInitInfo)) != CL_OK)
      {
        return rc;
      }

    /* Andrew Stone: Think about this; there is no mutex lock here, unlike the other functions */

    if(pInitInfo != NULL)
    {
        /*
         * Send the request to the master.
         */
        rc = ckptIdlHandleUpdate(mastAddr, pInitInfo->ckptIdlHdl, 
                                 CL_CKPT_NO_RETRY);
        memcpy(&ckptVersion,clVersionSupported,sizeof(ClVersionT));
        rc = VDECL_VER(clCkptMasterActiveAddrGetClientSync, 4, 0, 0)(pInitInfo->ckptIdlHdl,
                                      &ckptVersion,
                                      mastHdl,
                                      pNodeAddr);
        rc2 = rc;
        /*
         * Check for version mismatch.
         */
        if(ckptVersion.releaseCode != '\0' && rc == CL_OK)                  
        {
            clHandleCheckin(gClntInfo.ckptDbHdl,ckptSvcHdl);
            return CL_CKPT_ERR_VERSION_MISMATCH;
        }
    }                                  
    
    
    /* 
     * Set the active replica address for the checkpoint to the obtained one.
     */
    rc = ckptHandleCheckout(ckptHdl, CL_CKPT_CHECKPOINT_HDL,(void **)&pHdlInfo);
    if (rc == CL_OK)
      {
          /*
           * Update the active handle ONLY if the master active address get was a success
           * otherwise retry with the same active address again as ckpt server could be busy
           */
          if(rc2 == CL_OK) 
              pHdlInfo->activeAddr = *pNodeAddr; 
        rc = clHandleCheckin(gClntInfo.ckptDbHdl,ckptHdl);
      }
    else
      {
        clDbgCodeError(rc,("Handle %#llX checkout failed", ckptHdl));
      }

    /* Andrew Stone: The service handle checkin was above that of the data, but this is not true for all other functions... shouldn't 
       you have to have the service to modify the data? */

    /* 
     * Checkin the data associated with the service handle 
     */
    clHandleCheckin(gClntInfo.ckptDbHdl,ckptSvcHdl);

    return rc;
}

static ClBoolT clCkptHandleTypicalErrors(ClRcT rc, ClCkptHdlT ckptHdl,ClIocNodeAddressT* nodeAddr)
{
    ClRcT retCode;
    if(  (CL_GET_ERROR_CODE(rc) == CL_IOC_ERR_COMP_UNREACHABLE) ||
         (CL_IOC_ERR_HOST_UNREACHABLE == CL_GET_ERROR_CODE(rc)) || 
         (rc == CL_CKPT_ERR_NOT_EXIST && nodeAddr && *nodeAddr == CL_CKPT_UNINIT_VALUE)
         ||
         rc == CL_ERR_NOT_EXIST)
    {
        /* 
         * Maybe the active address has changed and this client 
         * hasn't received the update yet. Get active address 
         * from the master.
         */
        retCode = _ckptMasterActiveAddressGet(ckptHdl, nodeAddr);
        if(retCode == CL_OK) return CL_TRUE;
        else return CL_FALSE;
    }
    return CL_FALSE;
}


/*
 * Gets the status(various attributes) of the checkpoint.
 * The list of attributes are defined by the structure 
 * ClCkptCheckpointDescriptorT.
 */

ClRcT clCkptCheckpointStatusGet(ClCkptHdlT                  ckptHdl,
                                ClCkptCheckpointDescriptorT *pCkptStatus)
{
    ClRcT             rc         = CL_OK;
    CkptInitInfoT     *pInitInfo = NULL;
    ClIdlHandleT      ckptIdlHdl = CL_CKPT_INVALID_HDL;
    ClIocNodeAddressT nodeAddr   = 0;
    ClCkptHdlT        ckptActHdl = CL_CKPT_INVALID_HDL; 
    ClVersionT        version    = {0};
    ClCkptSvcHdlT     ckptSvcHdl = CL_CKPT_INVALID_HDL;
    ClBoolT           tryAgain   = CL_FALSE;
    ClUint32T         maxRetry   = 0;

    /*
     * Validate the input parameter.
     */
    if (NULL == pCkptStatus) return CL_CKPT_ERR_NULL_POINTER;

    if ((rc = ckptVerifyAndCheckout(ckptHdl, &ckptSvcHdl, &pInitInfo)) != CL_OK)
      {
        return rc;
      }

    /*
     * Lock the mutex.
     */
    clOsalMutexLock(pInitInfo->ckptSvcMutex);
    
    /* 
     * Fill the supported version information 
     */ 
    memcpy(&version,&clVersionSupported[0],sizeof(ClVersionT));

    /*
     * Get the active handle and the active replica address associated
     * with that local ckptHdl.
     */
    rc = ckptActiveHandleGet(ckptHdl,&ckptActHdl); 
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
              ("Ckpt: Handle get error rc[0x %x]\n",rc), rc);
    rc = ckptActiveAddressGet(ckptHdl,&nodeAddr); 
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
              ("Ckpt: Handle get error rc[0x %x]\n",rc), rc);
              
    /*
     * Send the call to the checkpoint active server.
     * Retry if the server is not not reachable.
     */
    do
    {
        rc = ckptIdlHandleUpdate(nodeAddr, pInitInfo->ckptIdlHdl,
                                 CL_CKPT_MAX_RETRY);
        CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR,
              ("Ckpt: Idl Handle update error rc[0x %x]\n",rc), rc);
        ckptIdlHdl = pInitInfo->ckptIdlHdl;

        rc = VDECL_VER(_ckptCheckpointStatusGetClientSync, 4, 0, 0)(ckptIdlHdl, ckptActHdl,
                                      pCkptStatus, &version);
        tryAgain = clCkptHandleTypicalErrors(rc, ckptHdl,&nodeAddr);

    }while((tryAgain == CL_TRUE) && (maxRetry++ < 2));

    /* 
     * Check for the version mismatch. 
     */
    if((version.releaseCode != '\0') && (rc == CL_OK))
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL,
                CL_CKPT_LOG_4_CLNT_VERSION_NACK, "Close call failed",
                version.releaseCode,version.majorVersion, 
                version.minorVersion);
        rc = CL_CKPT_ERR_VERSION_MISMATCH;             
    }
exitOnError:
    {
        /*
         * Unlock the mutex.
         */
        clOsalMutexUnlock(pInitInfo->ckptSvcMutex);

        /* 
         * Checkin the data associated with the service handle 
         */
        clHandleCheckin(gClntInfo.ckptDbHdl,ckptSvcHdl);
        return rc;
    }
}



/*
 * Creates a section in the checkpoint identified by the checkpoint handle.
 */

ClRcT clCkptSectionCreate(
          ClCkptHdlT                         ckptHdl,
          ClCkptSectionCreationAttributesT   *pSecCreateAttr,
          const ClUint8T                     *pData,
          ClSizeT                            dataSize)
{
    ClRcT              rc         = CL_OK;
    CkptInitInfoT      *pInitInfo = NULL;
    ClCharT            *tempData  = "";    
    ClCkptHdlT         ckptActHdl = CL_CKPT_INVALID_HDL;
    ClIocNodeAddressT  nodeAddr   = 0; 
    ClIdlHandleT       ckptIdlHdl = CL_CKPT_INVALID_HDL;
    CkptHdlDbT         *pHdlInfo  = NULL;
    ClVersionT         version    = {0};
    ClCkptSvcHdlT      ckptSvcHdl = CL_CKPT_INVALID_HDL;
    ClUint32T          numRetries = 0;
    ClBoolT            tryAgain   = CL_FALSE;
    ClUint32T          maxRetry   = 0;
    ClUint32T          index      = 0;

    CKPT_DEBUG_T(("ckptHdl: %#llX\n", ckptHdl));

    /*
     * Validate the input parameters.
     */
    if(NULL == pSecCreateAttr) return CL_CKPT_ERR_NULL_POINTER;
    if((NULL == pSecCreateAttr->sectionId) ||
       ((NULL == pSecCreateAttr->sectionId->id) && 
       (pSecCreateAttr->sectionId->idLen != 0)))
    {
        return CL_CKPT_ERR_NULL_POINTER;
    }
    if(NULL != pData && dataSize == 0) return CL_CKPT_ERR_INVALID_PARAMETER;
    if(NULL == pData && dataSize > 0) return CL_CKPT_ERR_NULL_POINTER;
    if(NULL == pData && dataSize == 0)
    {
        pData = (ClUint8T *)tempData;
    }
    
    if ((rc = ckptVerifyAndCheckout(ckptHdl, &ckptSvcHdl, &pInitInfo)) != CL_OK)
      {
        return rc;
      }

    /*
     * Lock the mutex.
     */
    clOsalMutexLock(pInitInfo->ckptSvcMutex);

    /*
     * Checkout the data associated with the checkpoint handle.
     */
    rc = ckptHandleCheckout(ckptHdl, CL_CKPT_CHECKPOINT_HDL, 
                            (void **)&pHdlInfo);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
              ("Ckpt: Handle get error rc[0x %x]\n",rc), rc);
   
    /*
     * Return ERROR if the checkpoint has been opened in read mode.
     */
    if(!((pHdlInfo->openFlag & CL_CKPT_CHECKPOINT_CREATE)
     ||(pHdlInfo->openFlag & CL_CKPT_CHECKPOINT_WRITE)))
    {
        rc = CL_CKPT_ERR_OP_NOT_PERMITTED;
        clHandleCheckin(gClntInfo.ckptDbHdl,ckptHdl);
    }
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR,
            ("Ckpt: Improper permissions as ckpt was not opened in write"
             " or create mode, rc[0x %x]\n",rc), rc);
              
    /* 
     * Checkin the data associated with the checkpoint handle. 
     */
    rc = clHandleCheckin(gClntInfo.ckptDbHdl,ckptHdl);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
              ("Ckpt: Section Creation failed rc[0x %x]\n",rc), rc);
              
    /*
     * Get the active handle and the active replica address associated
     * with that local ckptHdl.
     */
    rc = ckptActiveHandleGet(ckptHdl,&ckptActHdl); 
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
              ("Ckpt: Handle get error rc[0x %x]\n",rc), rc);
    rc = ckptActiveAddressGet(ckptHdl,&nodeAddr); 
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
              ("Ckpt: Handle get error rc[0x %x]\n",rc), rc);
              
    /* 
     * Fill the supported version information 
     */ 
    memcpy(&version,&clVersionSupported[0],sizeof(ClVersionT));

    /*
     * Send the call to the checkpoint active server.
     * Retry if the server is not not reachable.
     */
    do
    {
        numRetries = 0;
        rc = ckptIdlHandleUpdate(nodeAddr, pInitInfo->ckptIdlHdl,
                                 CL_CKPT_NO_RETRY);
        CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
              ("Ckpt: Idl Handle update error rc[0x %x]\n",rc), rc);
        ckptIdlHdl = pInitInfo->ckptIdlHdl;
        do
        {
            rc = VDECL_VER(_ckptSectionCreateClientSync, 4, 0, 0)( ckptIdlHdl, ckptActHdl,
                    CL_TRUE, pSecCreateAttr, (ClUint8T *)pData,
                    dataSize, &version,&index);
            if((numRetries > 0) && 
               (CL_GET_ERROR_CODE(rc) == CL_ERR_ALREADY_EXIST))
            {
                rc = CL_OK;
            }
        }while(CL_GET_ERROR_CODE(rc) == CL_ERR_TIMEOUT && (numRetries++ < 2));                         
        tryAgain = clCkptHandleTypicalErrors(rc, ckptHdl,&nodeAddr);

    }while((tryAgain == CL_TRUE) && (maxRetry++ < 2));

    /* 
     * Check for the version mismatch. 
     */
    if((version.releaseCode != '\0') && (rc == CL_OK))
    {
          clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL,
                    CL_CKPT_LOG_4_CLNT_VERSION_NACK, "Close call failed",
                    version.releaseCode,version.majorVersion, 
                    version.minorVersion);
          rc = CL_CKPT_ERR_VERSION_MISMATCH;             
    }
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
              ("Ckpt: Section creation error rc[0x %x]\n",rc), rc);

    if ((pSecCreateAttr->sectionId->idLen == 0) &&
            (pSecCreateAttr->sectionId->id == NULL))
    {
        /*
         * This is the case for generated sectionID. So we need to use the
         * value of index variable to re-generate the id and return to client
         */
        pSecCreateAttr->sectionId->id =
            clHeapCalloc(CL_CKPT_GEN_SEC_LENGTH, sizeof(ClCharT));
        if( NULL == pSecCreateAttr->sectionId->id )
        {
            rc = CL_CKPT_ERR_NO_MEMORY;
            CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR,
                    ("Failed to allocate memory for generated section"),rc);
        }
        snprintf((ClCharT *) pSecCreateAttr->sectionId->id, CL_CKPT_GEN_SEC_LENGTH,"generatedSection%d",
                index);
        pSecCreateAttr->sectionId->idLen =
            strlen((ClCharT *) pSecCreateAttr->sectionId->id) + 1;

    }

exitOnError:
    {
        /*
         * Unlock the mutex.
         */
        clOsalMutexUnlock(pInitInfo->ckptSvcMutex);

        /* 
         * Checkin the data associated with the service handle 
         */
        clHandleCheckin(gClntInfo.ckptDbHdl,ckptSvcHdl);
        return rc;
    }
}
 

 
/*
 * Deletes a section in the given checkpoint
 */
 
ClRcT clCkptSectionDelete(ClCkptHdlT               ckptHdl,
                          const  ClCkptSectionIdT *pSectionId)
{
    ClRcT              rc         = CL_OK;
    ClRcT              retCode    = CL_OK;
    CkptInitInfoT      *pInitInfo = NULL;
    ClCkptSectionIdT   tempSecId  = {0};
    ClIocNodeAddressT  nodeAddr   = 0;
    ClCkptHdlT         actHdl     = CL_CKPT_INVALID_HDL;
    ClIdlHandleT       ckptIdlHdl = CL_CKPT_INVALID_HDL;
    ClVersionT         version    = {0};
    ClCkptSvcHdlT      ckptSvcHdl = CL_CKPT_INVALID_HDL;
    ClUint32T          numRetries = 0;
    CkptHdlDbT         *pHdlInfo  = NULL;
    ClUint32T          hostResolutionRetries = 0;
    ClBoolT            tryAgain   = CL_FALSE;
   
    /*
     * Validate the input parameters.
     */
    if (NULL == pSectionId) return CL_CKPT_ERR_NULL_POINTER;
    
    if ((pSectionId->idLen == 0) || (pSectionId->id == NULL))
    {
        CKPT_DEBUG_E(("Ckpt: Invalid Parameter rc[0x %x]\n",rc));
        return CL_CKPT_ERR_INVALID_PARAMETER; 
    } 
    
    tempSecId.idLen = pSectionId->idLen;
    tempSecId.id    = (ClUint8T *)clHeapAllocate(tempSecId.idLen+1);
    if (!tempSecId.id)
      {
        clDbgCodeError(CL_CKPT_ERR_NO_MEMORY, ("Out of memory allocating %d bytes", tempSecId.idLen+1));
        return CL_CKPT_ERR_NO_MEMORY;
      }

    ((char* ) tempSecId.id)[tempSecId.idLen] = 0;
    memcpy(tempSecId.id,pSectionId->id,tempSecId.idLen);

    if ((rc = ckptVerifyAndCheckout(ckptHdl, &ckptSvcHdl, &pInitInfo)) != CL_OK)
      {
        clHeapFree(tempSecId.id);
        return rc;
      }    

    /*
     * Lock the mutex.
     */
    clOsalMutexLock(pInitInfo->ckptSvcMutex);
    
    /*
     * Checkout the data associated with the checkpoint handle.
     */
    rc = ckptHandleCheckout(ckptHdl, CL_CKPT_CHECKPOINT_HDL, 
                            (void **)&pHdlInfo);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
              ("Ckpt: Handle get error rc[0x %x]\n",rc), rc);

    /*
     * Return ERROR in case the checkpoint was neither opened in write nor
     * create mode.
     */
    if(!((pHdlInfo->openFlag & CL_CKPT_CHECKPOINT_CREATE) ||
         (pHdlInfo->openFlag & CL_CKPT_CHECKPOINT_WRITE)))
    {
        rc = CL_CKPT_ERR_OP_NOT_PERMITTED;
        clHandleCheckin(gClntInfo.ckptDbHdl,ckptHdl);
        CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR,
                ("Ckpt: Improper permissions as ckpt was not opened in write"
                 " or create mode, rc[0x %x]\n",rc), rc);
    }
    
    /*
     * Checkin the data associated with the checkpoint handle.
     */
    rc = clHandleCheckin(gClntInfo.ckptDbHdl,ckptHdl);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
              ("Ckpt: Handle Check-in Error rc[0x %x]\n",rc), rc);

    /*
     * Get the active handle and the active replica address associated
     * with that local ckptHdl.
     */
    rc = ckptActiveHandleGet(ckptHdl,&actHdl);   
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, ("Ckpt: Handle get error rc[0x %x]\n",rc), rc);
    rc = ckptActiveAddressGet(ckptHdl,&nodeAddr); 
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, ("Ckpt: Active server not exist rc[0x %x]\n",rc), rc);
 
    /* 
     * Fill the supported version information 
     */ 
    memcpy(&version,&clVersionSupported[0],sizeof(ClVersionT));

    /*
     * Send the call to the checkpoint active server.
     * Retry if the server is not not reachable.
     */
    do
    {
        tryAgain   = CL_FALSE;
        numRetries = 0;
        rc = ckptIdlHandleUpdate(nodeAddr, pInitInfo->ckptIdlHdl, 
                                CL_CKPT_NO_RETRY);
        CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
              ("Ckpt: Idl Handle update error rc[0x %x]\n",rc), rc);
        ckptIdlHdl = pInitInfo->ckptIdlHdl;

        do
        {
            rc = VDECL_VER(_ckptSectionDeleteClientSync, 4, 0, 0)( ckptIdlHdl, actHdl,
                                     CL_TRUE, 
                                     &tempSecId,  &version);
            if(numRetries > 0 && 
               CL_GET_ERROR_CODE(rc) == CL_ERR_ALREADY_EXIST)
            {
                rc = CL_OK;
            }
        }while(CL_GET_ERROR_CODE(rc) == CL_ERR_TIMEOUT && (numRetries++ < 2));                         

        if(CL_GET_ERROR_CODE(rc) == CL_IOC_ERR_COMP_UNREACHABLE || CL_IOC_ERR_HOST_UNREACHABLE == CL_GET_ERROR_CODE(rc))
        {
            /* 
             * May be the active address has changed and this client 
             * hasn't received the update yet. Get active address 
             * from the master.
             */
            tryAgain = CL_TRUE;
            retCode = _ckptMasterActiveAddressGet(ckptHdl, &nodeAddr);
            if(retCode == CL_OK)
              hostResolutionRetries++;
            else
              hostResolutionRetries = 1000;  /* Don't retry */
        }
    } while((CL_TRUE == tryAgain) && (hostResolutionRetries < 2));

    /* 
     * Check for the version mismatch. 
     */
    if((version.releaseCode != '\0')  && (rc == CL_OK))
    {
          clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL,
                    CL_CKPT_LOG_4_CLNT_VERSION_NACK, "Close call failed",
                    version.releaseCode,version.majorVersion, 
                    version.minorVersion);
          rc = CL_CKPT_ERR_VERSION_MISMATCH;             
    }
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, ("Ckpt: Section delete error rc[0x %x]\n",rc), rc);
    
exitOnError:
    {
        CL_ASSERT(tempSecId.id);
        clHeapFree(tempSecId.id);

        /*
         * Unlock the mutex.
         */
        clOsalMutexUnlock(pInitInfo->ckptSvcMutex);

        /* 
         * Checkin the data associated with the service handle 
         */
        clHandleCheckin(gClntInfo.ckptDbHdl,ckptSvcHdl);
        return rc;
    }
}



/*
 * Sets the expiration time of a section
 */
 
ClRcT clCkptSectionExpirationTimeSet(ClCkptHdlT              ckptHdl,
                                     const ClCkptSectionIdT  *pSectionId,
                                     ClTimeT                 expryTime)
{
    ClRcT              rc         = CL_OK;
    CkptHdlDbT         *pHdlInfo  = NULL;
    CkptInitInfoT      *pInitInfo = NULL;
    ClCkptSectionIdT   tempSecId  = {0};
    ClIocNodeAddressT  nodeAddr   = 0;
    ClCkptHdlT         actHdl     = CL_CKPT_INVALID_HDL;
    ClIdlHandleT       ckptIdlHdl = CL_CKPT_INVALID_HDL;
    ClVersionT         version    = {0};
    ClCkptSvcHdlT      ckptSvcHdl = CL_CKPT_INVALID_HDL;
    ClBoolT            tryAgain   = CL_FALSE;
    ClUint32T          maxRetry   = 0;
      
    /*
     * Validate the input parameters.
     */
    if (NULL == pSectionId) return CL_CKPT_ERR_NULL_POINTER;
    
     /*
     * Return ERROR if the section is default section.
     */
    if((pSectionId->idLen == 0)  || (pSectionId->id == NULL ))
    {
        rc = CL_CKPT_ERR_INVALID_PARAMETER;
        CKPT_DEBUG_E(("Default section expirationtime can not be changed\n"));
        return rc;
    }

    /*
     * Copy the information to be sent across to the server side.
     */
    tempSecId.idLen = pSectionId->idLen;
    tempSecId.id    = (ClUint8T *)clHeapCalloc(1,(tempSecId.idLen+1));
    if (!tempSecId.id)
      {
        clDbgCodeError(CL_CKPT_ERR_NO_MEMORY, ("Out of memory allocating %d bytes", tempSecId.idLen+1));
        return CL_CKPT_ERR_NO_MEMORY;
      }

    ((char* ) tempSecId.id)[tempSecId.idLen] = 0;
    memcpy(tempSecId.id,pSectionId->id,tempSecId.idLen);

    if ((rc = ckptVerifyAndCheckout(ckptHdl, &ckptSvcHdl, &pInitInfo)) != CL_OK)
      {
        clHeapFree(tempSecId.id);
        return rc;
      }

    /*
     * Lock the mutex.
     */
    clOsalMutexLock(pInitInfo->ckptSvcMutex); 
    
    /* 
     * Checkout the data associated with the checkpoint handle. 
     */
    rc = ckptHandleCheckout(ckptHdl, CL_CKPT_CHECKPOINT_HDL, 
                            (void **)&pHdlInfo);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
              ("Ckpt: Handle create error rc[0x %x]\n",rc), rc);

    /*
     * Return ERROR if the checkpoint was opened with read mode.
     */
    if(! ((CL_CKPT_CHECKPOINT_CREATE == (pHdlInfo->openFlag &
                  CL_CKPT_CHECKPOINT_CREATE)) ||
          (CL_CKPT_CHECKPOINT_WRITE == (pHdlInfo->openFlag &
                  CL_CKPT_CHECKPOINT_WRITE))))
    {
        rc = CL_CKPT_ERR_OP_NOT_PERMITTED;
    }
        
    /* 
     * Checkin the data associated with the checkpoint handle. 
     */
    clHandleCheckin(gClntInfo.ckptDbHdl,ckptHdl);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR,
            ("Ckpt: Improper permissions as ckpt was not opened in write"
             " or create mode, rc[0x %x]\n",rc), rc);

    /*
     * Get the active handle and the active replica address associated
     * with that local ckptHdl.
     */
    rc = ckptActiveHandleGet(ckptHdl,&actHdl);   
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
              ("Ckpt: Handle get error rc[0x %x]\n",rc), rc);
    rc = ckptActiveAddressGet(ckptHdl,&nodeAddr); 
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
              ("Ckpt: Handle get error rc[0x %x]\n",rc), rc);
 
    /* 
     * Fill the supported version information 
     */ 
    memcpy(&version,&clVersionSupported[0],sizeof(ClVersionT));

    /*
     * Send the call to the checkpoint active server.
     * Retry if the server is not not reachable.
     */
    do
    {
        rc = ckptIdlHandleUpdate(nodeAddr, pInitInfo->ckptIdlHdl,
                                CL_CKPT_MAX_RETRY);
        CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
              ("Ckpt: Idl Handle update error rc[0x %x]\n",rc), rc);
        ckptIdlHdl = pInitInfo->ckptIdlHdl;

        rc = VDECL_VER(_ckptSectionExpirationTimeSetClientSync, 4, 0, 0)(ckptIdlHdl, actHdl,
                                           &tempSecId, expryTime,
                                           &version);
        tryAgain = clCkptHandleTypicalErrors(rc, ckptHdl,&nodeAddr);

    }while((tryAgain == CL_TRUE) && (maxRetry++ < 2));

    /* 
     * Check for the version mismatch. 
     */
    if((version.releaseCode != '\0')  && (rc == CL_OK))
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL,
                   CL_CKPT_LOG_4_CLNT_VERSION_NACK, "Close call failed",
                   version.releaseCode, version.majorVersion, 
                   version.minorVersion);
        rc = CL_CKPT_ERR_VERSION_MISMATCH;             
    }
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
              ("Ckpt: Expiration Time set failed rc[0x %x]\n",rc), rc);
    
exitOnError:
    {
        CL_ASSERT(tempSecId.id != NULL);
        clHeapFree(tempSecId.id);
        /*
         * Unlock the mutex.
         */
        clOsalMutexUnlock(pInitInfo->ckptSvcMutex);

        /* 
         * Checkin the data associated with the service handle 
         */
        clHandleCheckin(gClntInfo.ckptDbHdl,ckptSvcHdl);
        return rc;
    }
}



/*
 * Enable the application to iterate thru sections in a checkpoint
 */
 
ClRcT clCkptSectionIterationInitialize(ClCkptHdlT             ckptHdl,
                                       ClCkptSectionsChosenT  sectionsChosen,
                                       ClTimeT                exprTime,
                                       ClHandleT              *pSecItrHdl)
{
    ClRcT                  rc                 = CL_OK;
    CkptIterationInfoT     *pSecIterationInfo = NULL;
    ClCkptSectionIdT       *pSec              = NULL;
    CkptInitInfoT          *pInitInfo         = NULL;
    ClCkptHdlT             actHdl             = CL_CKPT_INVALID_HDL;
    ClIocNodeAddressT      nodeAddr           = 0;
    ClUint32T              count              = 0;
    ClUint32T              secCount           = 0;
    ClCkptSvcHdlT          ckptSvcHdl         = CL_CKPT_INVALID_HDL;
    ClBoolT                tryAgain           = CL_FALSE;
    ClCkptSectionIdT       *pSecId            = NULL;
    ClCkptSectionIdT       *pTempSec          = NULL;
    ClVersionT             ckptVersion        = {0};
    ClUint32T              maxRetry           = 0;
    ClUint8T               status = CL_IOC_NODE_DOWN;
    ClTimerTimeOutT        delay = {.tsSec = 0, .tsMilliSec = 500};
        
    /*
     * Verify the input parameters.
     */
    if(pSecItrHdl == NULL) return CL_CKPT_ERR_NULL_POINTER;

    if(CL_CKPT_SECTIONS_ANY < sectionsChosen)
    {
        return CL_CKPT_ERR_INVALID_PARAMETER;
    }
    
    retry:
    if(maxRetry++ >= 5)
    {
        return rc;
    }
    
    if( gClntInfo.ckptDbHdl == 0 || gClntInfo.ckptSvcHdlCount == 0)
    {
        CKPT_DEBUG_E(("CKPT Client is not yet initialized"));
        return CL_CKPT_ERR_NOT_INITIALIZED;
    }

    if(maxRetry > 1)
        clOsalTaskDelay(delay);

    /*
     * Safe to grab incase svc handle count is non zero or ckpt is initialized.
     */
    clOsalMutexLock(&gClntInfo.ckptClntMutex);

    if(!gClntInfo.ckptSvcHdlCount)
    {
        clOsalMutexUnlock(&gClntInfo.ckptClntMutex);
        return CL_CKPT_ERR_NOT_INITIALIZED;
    }

    if ((rc = ckptVerifyAndCheckout(ckptHdl, &ckptSvcHdl, &pInitInfo)) != CL_OK)
    {
        clOsalMutexUnlock(&gClntInfo.ckptClntMutex);
        return rc;
    }

    /*
     * Lock the mutex.
     */
    clOsalMutexLock(pInitInfo->ckptSvcMutex);
    
    /*
     * Get the active handle and the active replica address associated
     * with that local ckptHdl.
     */
    rc = ckptActiveHandleGet(ckptHdl,&actHdl);   
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
                   ("Ckpt: Handle get error rc[0x %x]\n",rc), rc);
    rc = ckptActiveAddressGet(ckptHdl,&nodeAddr); 
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
                   ("Ckpt: Handle get error rc[0x %x]\n",rc), rc);

    rc = clIocRemoteNodeStatusGet(nodeAddr, &status);
    if(status == CL_IOC_NODE_DOWN)
    {
        clOsalMutexUnlock(pInitInfo->ckptSvcMutex);
        clHandleCheckin(gClntInfo.ckptDbHdl, ckptSvcHdl);
        clOsalMutexUnlock(&gClntInfo.ckptClntMutex);
        rc = CL_CKPT_ERR_NOT_EXIST;
        goto retry;
    }

    /* 
     * Fill the supported version information 
     */ 
    memcpy(&ckptVersion,&clVersionSupported[0],sizeof(ClVersionT));

    /*
     * Send the call to the checkpoint active server.
     * Retry if the server is not not reachable.
     */
    rc = ckptIdlHandleUpdate(nodeAddr, pInitInfo->ckptIdlHdl,
                             CL_CKPT_MAX_RETRY);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
                   ("Ckpt: Idl Handle update error rc[0x %x]\n",rc), rc);
    rc = VDECL_VER(clCkptSvrIterationInitializeClientSync, 4, 0, 0)( pInitInfo->ckptIdlHdl,
                                                                     &ckptVersion,
                                                                     actHdl,
                                                                     sectionsChosen,
                                                                     exprTime,
                                                                     &secCount,
                                                                     &pSecId);
    tryAgain = clCkptHandleTypicalErrors(rc, ckptHdl,&nodeAddr);

    if(tryAgain)
    {
        clOsalMutexUnlock(pInitInfo->ckptSvcMutex);
        clHandleCheckin(gClntInfo.ckptDbHdl, ckptSvcHdl);
        clOsalMutexUnlock(&gClntInfo.ckptClntMutex);
        if(pSecId)
        {
            for(count = 0; count < secCount; ++count)
            {
                if(pSecId[count].id)
                {
                    clHeapFree(pSecId[count].id);
                }
            }
            clHeapFree(pSecId);
            pSecId = NULL;
            secCount = 0;
        }
        goto retry;
    }

    pTempSec = pSecId;   /* On error, pTempSec will be cleaned up */

    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR,
                   ("Ckpt:Error during Svc init rc[0x %x]\n",rc), rc);

    /*
     * Create a iteration handle and store the section iteration related
     * info in it.
     */
    rc = clHandleCreate(gClntInfo.ckptDbHdl,sizeof(CkptIterationInfoT),
                        pSecItrHdl);
    rc = clHandleCheckout(gClntInfo.ckptDbHdl,*pSecItrHdl,
                          (void **)&pSecIterationInfo);

    /*
     * Add the handle to the iteration handle list.
     */
    rc = clCntNodeAdd(pInitInfo->hdlList,(ClCntKeyHandleT)(ClWordT)*pSecItrHdl,
                      0, NULL);
    /*
     * Store the info associated with the given iteration request.
     */
    pSecIterationInfo->hdlType   = CL_CKPT_ITER_HDL;
    pSecIterationInfo->ckptHdl   = actHdl;
    pSecIterationInfo->localHdl  = ckptHdl;
    pSecIterationInfo->idlHdl    = pInitInfo->ckptIdlHdl;
    pSecIterationInfo->secCount  = secCount;
    pSecIterationInfo->pSecId    = (ClCkptSectionIdT *)clHeapCalloc(
                                                                    1, secCount * sizeof(ClCkptSectionIdT));
    if(pSecIterationInfo->pSecId == NULL)
    {
        CL_DEBUG_PRINT (CL_DEBUG_CRITICAL,
                        ("Ckpt:memory allocation is failed rc[0x %x]\n",rc));
        clLogWrite(CL_LOG_HANDLE_APP,CL_LOG_CRITICAL,CL_LOG_CKPT_LIB_NAME,
                   CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
        /* Don't need to free pSecId, because it will happen in the exitOnError call. clHeapFree(pSecId); */
        clHandleCheckin(gClntInfo.ckptDbHdl,*pSecItrHdl);
        rc = CL_CKPT_ERR_NO_MEMORY;
        goto exitOnError;
    }
    
    pSec = pSecIterationInfo->pSecId;

    for(count = 0;count < secCount ;count++)
    {
        pSecIterationInfo->pSecId->idLen = pSecId->idLen;

        if(pSecIterationInfo->pSecId->idLen > 0)
        {
            pSecIterationInfo->pSecId->id    = (ClUint8T *)clHeapAllocate(
                                                                          (pSecIterationInfo->pSecId->idLen+1));
            if(pSecIterationInfo->pSecId->id == NULL)  
            {
                CL_DEBUG_PRINT (CL_DEBUG_CRITICAL,
                                ("Ckpt:memory allocation is failed rc[0x %x]\n",rc));
                clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_CRITICAL,
                           CL_LOG_CKPT_LIB_NAME,
                           CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
                /* Andrew Stone: But what about all the other iteratrions? */ 
                clHeapFree(pSecIterationInfo->pSecId);
                /* Don't need to free pSecId, because it will happen in the exitOnError call. clHeapFree(pTempSec); */
                clHandleCheckin(gClntInfo.ckptDbHdl,*pSecItrHdl);
                rc = CL_CKPT_ERR_NO_MEMORY;
                goto exitOnError;
            }
            /* Andrew Stone: inefficient: memset(pSecIterationInfo->pSecId->id, '\0', (pSecIterationInfo->pSecId->idLen+1)); */
            ((char*) pSecIterationInfo->pSecId->id)[pSecIterationInfo->pSecId->idLen]=0;
            memcpy(pSecIterationInfo->pSecId->id,pSecId->id,pSecId->idLen);
        }
        pSecIterationInfo->pSecId++;
        pSecId++;
    }
    pSecIterationInfo->pSecId  = pSec;
    pSecIterationInfo->nextSec = 0;
    
    /* 
     * Checkin the data associated with the iteration handle. 
     */
    rc = clHandleCheckin(gClntInfo.ckptDbHdl,*pSecItrHdl);
    exitOnError:
    {
        /*
         * Unlock the mutex.
         */
        clOsalMutexUnlock(pInitInfo->ckptSvcMutex);

        /* 
         * Checkin the data associated with the service handle 
         */
        clHandleCheckin(gClntInfo.ckptDbHdl,ckptSvcHdl);
        clOsalMutexUnlock(&gClntInfo.ckptClntMutex);
        if(pTempSec != NULL) 
        {
            for(count = 0; count < secCount; ++count)
            {
                if(pTempSec[count].id)
                    clHeapFree(pTempSec[count].id);
            }
            clHeapFree(pTempSec);
        }
        return rc;
    }
}

/*
 * Gets the next section in the list of sections matching the 
 * ckptSectionIterationInitialize call.
 */

ClRcT clCkptSectionIterationNext(ClHandleT                secIterHdl,
                                 ClCkptSectionDescriptorT *pSecDescriptor)
{
    ClRcT                 rc            = CL_OK;
    CkptIterationInfoT    *pSecIterInfo = NULL;
    ClCkptSectionIdT      secId         = {0};
    ClCkptSectionIdT      *pSecId       = NULL;
    ClCkptHdlT            actHdl        = CL_CKPT_INVALID_HDL;
    ClIdlHandleT          ckptIdlHdl    = CL_CKPT_INVALID_HDL;
    ClVersionT            version       = {0};
    CkptHdlDbT            *pHdlInfo     = NULL;
 
    /*
     * Check whether the client library is initialized or not.
     */
    if( gClntInfo.ckptDbHdl == 0)
    {
        CKPT_DEBUG_E(("CKPT Client is not yet initialized"));
        return CL_CKPT_ERR_NOT_INITIALIZED;
    }
    
    /*
     * Verify the input parameters.
     */
    if(pSecDescriptor == NULL) return CL_CKPT_ERR_NULL_POINTER;
    
    do
    {
       /*
        * Checkout the info associated with the iteration handle.
        */
        rc = ckptHandleCheckout(secIterHdl, CL_CKPT_ITER_HDL,
                (void **)&pSecIterInfo);
        if(rc != CL_OK)
        {
            CKPT_DEBUG_E(("Passed handle %#llX is invalid", secIterHdl));
            return CL_CKPT_ERR_INVALID_HANDLE;
        }
    
       /*
        * Checkout the info associated with the checkpoint handle.
        * This is just to check that the checkpoint exists and has not been 
        * deleted.
        */
        rc = ckptHandleCheckout(pSecIterInfo->localHdl, 
                                CL_CKPT_CHECKPOINT_HDL, 
                                (void **)&pHdlInfo);
        if(rc != CL_OK)
        {
            /*
             * Free the section iteration related info.
             */
            clHandleCheckin(gClntInfo.ckptDbHdl,secIterHdl);
            return CL_CKPT_ERR_INVALID_HANDLE;
        }
        else
        {
            /*
             * Checkin the info associated with the checkpoint handle.
             */
            clHandleCheckin(gClntInfo.ckptDbHdl, pSecIterInfo->localHdl);
        }
        
        actHdl     = pSecIterInfo->ckptHdl;
        ckptIdlHdl = pSecIterInfo->idlHdl;
        
        if( pSecIterInfo->nextSec >= pSecIterInfo->secCount)
        {
            rc = CKPT_RC(CL_CKPT_ERR_NO_SECTIONS);
            clHandleCheckin(gClntInfo.ckptDbHdl,secIterHdl);
            break;
        }
        pSecId  = pSecIterInfo->pSecId + pSecIterInfo->nextSec;
        if(pSecId != NULL)
        {
            secId.idLen = pSecId->idLen;
            if(secId.idLen == 0 )
            {
                secId.idLen = 0;
                secId.id    = NULL;
            }
            else
            {
                secId.id    = (ClUint8T *)clHeapAllocate(pSecId->idLen+1); 
                if(secId.id == NULL)
                {
                    CL_DEBUG_PRINT (CL_DEBUG_CRITICAL,
                       ("Ckpt:memory allocation is failed rc[0x %x]\n",rc));
                    clHandleCheckin(gClntInfo.ckptDbHdl,secIterHdl);
                    return CL_CKPT_ERR_NO_MEMORY;
                }
                memcpy(secId.id,pSecId->id,secId.idLen);
                secId.id[secId.idLen] = 0;
            }  
        }
        pSecIterInfo->nextSec++;
        
       /*
        * Checkin the info associated with the iteration handle.
        */
        rc = clHandleCheckin(gClntInfo.ckptDbHdl,secIterHdl);
        CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
                ("Ckpt: Handle Checkin error rc[0x %x]\n",rc), rc);

        /* 
         * Fill the supported version information 
         */ 
        memcpy(&version,&clVersionSupported[0],sizeof(ClVersionT));

        /*
         * Send the call to the checkpoint active server.
         */
        rc = VDECL_VER(_ckptIterationNextGetClientSync, 4, 0, 0)(ckptIdlHdl,actHdl,&secId,
                pSecDescriptor,&version);
                
        /* 
         * Check for the version mismatch. 
         */
        if((version.releaseCode != '\0')  && (rc == CL_OK))
        {
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL,
                       CL_CKPT_LOG_4_CLNT_VERSION_NACK, "Close call failed",
                       version.releaseCode, version.majorVersion, 
                       version.minorVersion);
            rc = CL_CKPT_ERR_VERSION_MISMATCH;             
        }
        if(secId.id != NULL)
        {
            clHeapFree(secId.id);
            secId.id  = NULL;
        }
    }while(rc == CL_CKPT_ERR_NOT_EXIST);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_TRACE, 
            ("Ckpt: Section is not exist error rc[0x %x]\n",rc), rc);

    if(pSecDescriptor->sectionId.idLen == 0)
    {
        if(pSecDescriptor->sectionId.id != NULL) 
            clHeapFree(pSecDescriptor->sectionId.id);
        pSecDescriptor->sectionId.id = NULL;
    }
    if(secId.id != NULL) clHeapFree(secId.id);
    return rc;
exitOnError:
    {
        if(secId.id != NULL) clHeapFree(secId.id);
        return rc;
    }
}



/*
 * Frees resources associated with the iteration identified by
 * sectionIterationHandle
 */
 
ClRcT clCkptSectionIterationFinalize(ClHandleT secIterHdl)
{
    ClRcT                 rc            = CL_OK;
    CkptIterationInfoT    *pSecIterInfo = NULL;
    ClCkptSectionIdT      *pSec         = NULL;
    ClUint32T             count         = 0;
    CkptHdlDbT            *pHdlInfo     = NULL;
    ClCkptSvcHdlT         ckptSvcHdl    = CL_CKPT_INVALID_HDL;
    CkptInitInfoT         *pInitInfo    = NULL;
    
    /*
     * Check whether the client library is initialized or not.
     */
    if(gClntInfo.ckptDbHdl == 0)
    {
        CKPT_DEBUG_E(("CKPT Client is not yet initialized"));
        return CL_CKPT_ERR_NOT_INITIALIZED;
    }
    
    /* 
     * Checkout the data associated with the iteration handle.
     */
    rc = ckptHandleCheckout(secIterHdl, CL_CKPT_ITER_HDL,
                            (void **)&pSecIterInfo);
    if(rc != CL_OK)
    {
        CKPT_DEBUG_E(("\nPassed handle is invalid\n"));
        return CL_CKPT_ERR_INVALID_HANDLE;
    }
    
    /* 
     * Checkout the data associated with the checkpoint handle.
     * This is just to check that the checkpoint exists and has not been 
     * deleted.
     */
    rc = ckptHandleCheckout(pSecIterInfo->localHdl, CL_CKPT_CHECKPOINT_HDL,
            (void **)&pHdlInfo);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
            ("Ckpt: Invalid Handle  rc[0x %x]\n",rc), rc);
            
    for(count = 0; count < pSecIterInfo->secCount ; count++)
    {
        pSec = pSecIterInfo->pSecId + count;
        if(pSec != NULL) clHeapFree(pSec->id); 
    }	
    
    clHeapFree(pSecIterInfo->pSecId);
    
    /* 
     * Checkin the data associated with the checkpoint handle.
     */
    rc = clHandleCheckin(gClntInfo.ckptDbHdl,pSecIterInfo->localHdl);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
            ("Ckpt: Invalid Handle  rc[0x %x]\n",rc), rc);
            
    /* 
     * Checkin the data associated with the iteration handle.
     */
    rc = clHandleCheckin(gClntInfo.ckptDbHdl,secIterHdl);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
            ("Ckpt: Invalid Handle  rc[0x %x]\n",rc), rc);

    /*
     * Get the associated service handle.
     */
    rc = ckptSvcHandleGet(pSecIterInfo->localHdl, &ckptSvcHdl);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
            ("Ckpt: Invalid Handle  rc[0x %x]\n",rc), rc);
            
    /* 
     * Checkout the data associated with the service handle.
     */
    rc = ckptHandleCheckout(ckptSvcHdl, CL_CKPT_SERVICE_HDL,
            (void **)&pInitInfo);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
            ("Ckpt: Invalid Handle  rc[0x %x]\n",rc), rc);
    /*
     * Delete the iteration handle from the handle list.
     */
    rc = clCntAllNodesForKeyDelete(pInitInfo->hdlList,
                                   (ClCntKeyHandleT)(ClWordT)secIterHdl);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
            ("Ckpt: Invalid Handle  rc[0x %x]\n",rc), rc);

    /* 
     * Checkin the data associated with the service handle 
     */
    rc = clHandleCheckin(gClntInfo.ckptDbHdl,ckptSvcHdl);
exitOnError:
    {
        return rc;
    }
}

ClRcT clCkptSectionCheck(ClCkptHdlT             ckptHdl,
                         ClCkptSectionIdT       *pSectionId)
{
    ClRcT                  rc                 = CL_OK;
    CkptInitInfoT          *pInitInfo         = NULL;
    ClCkptHdlT             actHdl             = CL_CKPT_INVALID_HDL;
    ClIocNodeAddressT      nodeAddr           = 0;
    ClCkptSvcHdlT          ckptSvcHdl         = CL_CKPT_INVALID_HDL;
    ClBoolT                tryAgain           = CL_FALSE;
    ClUint32T              maxRetry           = 0;
    ClUint8T               status = CL_IOC_NODE_DOWN;
    ClTimerTimeOutT        delay = {.tsSec = 0, .tsMilliSec = 50};
        
    /*
     * Verify the input parameters.
     */
    if(!pSectionId) return CL_CKPT_ERR_INVALID_PARAMETER;

    retry:
    if(maxRetry++ >= 3)
    {
        return rc;
    }
    
    if( gClntInfo.ckptDbHdl == 0 || gClntInfo.ckptSvcHdlCount == 0)
    {
        CKPT_DEBUG_E(("CKPT Client is not yet initialized"));
        return CL_CKPT_ERR_NOT_INITIALIZED;
    }

    if(maxRetry > 1)
        clOsalTaskDelay(delay);

    /*
     * Safe to grab incase svc handle count is non zero or ckpt is initialized.
     */
    clOsalMutexLock(&gClntInfo.ckptClntMutex);

    if(!gClntInfo.ckptSvcHdlCount)
    {
        clOsalMutexUnlock(&gClntInfo.ckptClntMutex);
        return CL_CKPT_ERR_NOT_INITIALIZED;
    }

    if ((rc = ckptVerifyAndCheckout(ckptHdl, &ckptSvcHdl, &pInitInfo)) != CL_OK)
    {
        clOsalMutexUnlock(&gClntInfo.ckptClntMutex);
        return rc;
    }

    /*
     * Lock the mutex.
     */
    clOsalMutexLock(pInitInfo->ckptSvcMutex);
    
    /*
     * Get the active handle and the active replica address associated
     * with that local ckptHdl.
     */
    rc = ckptActiveHandleGet(ckptHdl,&actHdl);   
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
                   ("Ckpt: Handle get error rc[0x %x]\n",rc), rc);
    rc = ckptActiveAddressGet(ckptHdl,&nodeAddr); 
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
                   ("Ckpt: Handle get error rc[0x %x]\n",rc), rc);

    clOsalMutexUnlock(pInitInfo->ckptSvcMutex);
    clHandleCheckin(gClntInfo.ckptDbHdl, ckptSvcHdl);
    clOsalMutexUnlock(&gClntInfo.ckptClntMutex);

    rc = clIocRemoteNodeStatusGet(nodeAddr, &status);
    if(status == CL_IOC_NODE_DOWN)
    {
        rc = CL_CKPT_ERR_NOT_EXIST;
        goto retry;
    }
    
    /*
     * Send the call to the checkpoint active server.
     * Retry if the server is not not reachable.
     */
    rc = ckptIdlHandleUpdate(nodeAddr, pInitInfo->ckptIdlHdl, 1);
    CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
                                  ("Ckpt: Idl Handle update error rc[0x %x]\n",rc), rc);
    rc = VDECL_VER(_ckptSectionCheckClientSync, 5, 0, 0)( pInitInfo->ckptIdlHdl,
                                                          actHdl,
                                                          pSectionId);
    tryAgain = clCkptHandleTypicalErrors(rc, ckptHdl, &nodeAddr);

    if(tryAgain)
    {
        goto retry;
    }
    
    return rc;
    
    exitOnError:
    clOsalMutexUnlock(pInitInfo->ckptSvcMutex);
    clHandleCheckin(gClntInfo.ckptDbHdl, ckptSvcHdl);
    clOsalMutexUnlock(&gClntInfo.ckptClntMutex);

    exitOnErrorBeforeHdlCheckout:
    return rc;
}

static ClRcT ckptClientInfoGet(CkptHdlDbT *pHdlInfo, 
                               ClIdlHandleT idlHdl, ClCkptHdlT actHdl, 
                               ClCkptClientInfoListT *pClientList)
{
    ClRcT rc = CL_OK;
    /*
     * Check the cache first
     */
    if(pHdlInfo->clientList.pClientInfo)
    {
        if(!gClntInfo.ckptClientCacheDisable 
           &&
           !(pHdlInfo->creationFlag & CL_CKPT_PEER_TO_PEER_CACHE_DISABLE))
        {
            memcpy(pClientList, &pHdlInfo->clientList, sizeof(*pClientList));
            clLogTrace("CACHE", "GET", "Found [%d] client entries in cache for ckpt [%.*s]",
                       pClientList->numEntries, pHdlInfo->ckptName.length, pHdlInfo->ckptName.value);
            goto out;
        }
        clHeapFree(pHdlInfo->clientList.pClientInfo);
        pHdlInfo->clientList.pClientInfo = NULL;
        pHdlInfo->clientList.numEntries = 0;
    }

    rc = VDECL_VER(_ckptClientInfoGetClientSync, 6, 0, 0)(idlHdl, 
                                                          actHdl,
                                                          pClientList);

    if(rc == CL_OK)
    {
        /*
         * Skip self entries
         */
        ClUint32T numEntries = pClientList->numEntries;
        for(ClUint32T i = 0; i < numEntries; ++i)
        {
            if(pClientList->pClientInfo[i].nodeAddress == gClntInfo.ckptOwnAddr.nodeAddress
               &&
               pClientList->pClientInfo[i].portId == gClntInfo.ckptOwnAddr.portId)
            {
                --pClientList->numEntries;
                if(pClientList->numEntries > 0 && i < pClientList->numEntries)
                {
                    memmove(&pClientList->pClientInfo[i], &pClientList->pClientInfo[i+1],
                            sizeof(*pClientList->pClientInfo) * (pClientList->numEntries - i));
                }
                break;
            }
        }
        if(pClientList->numEntries == 0)
        {
            if(pClientList->pClientInfo)
            {
                clHeapFree(pClientList->pClientInfo);
                pClientList->pClientInfo = NULL;
            }
        }
        else
        {
            memcpy(&pHdlInfo->clientList, pClientList, sizeof(pHdlInfo->clientList));
        }
    }

    out:
    return rc;
}

/*
 * Writes multiple sections on to a given checkpoint.
 */

ClRcT clCkptCheckpointWriteVector(ClCkptHdlT                     ckptHdl,
                                  const ClCkptDifferenceIOVectorElementT   *pDifferenceIoVector,
                                  ClUint32T                      numberOfElements,
                                  ClUint32T                      *pError)
{
    ClRcT               rc         = CL_OK;
    CkptInitInfoT       *pInitInfo = NULL;
    ClIocNodeAddressT   nodeAddr   = 0;
    ClCkptHdlT          actHdl     = CL_CKPT_INVALID_HDL;
    ClVersionT          version    = {0};
    CkptHdlDbT          *pHdlInfo  = 0;
    ClIdlHandleT        ckptIdlHdl = CL_CKPT_INVALID_HDL; 
    ClCkptSvcHdlT       ckptSvcHdl = CL_CKPT_INVALID_HDL;
    ClUint32T           tempError  = 0;
    ClBoolT             tryAgain   = CL_FALSE;
    ClUint32T           maxRetry   = 0;
    ClIocPortT          iocPort    = 0;
    ClUint8T            status = CL_IOC_NODE_DOWN;
    ClTimerTimeOutT     delay =  {.tsSec = 0, .tsMilliSec = 500 };

    /*
     * Input parameter verification.
     */
    if (NULL == pDifferenceIoVector) return CL_CKPT_ERR_NULL_POINTER;
    if (0 == numberOfElements) return CL_CKPT_ERR_INVALID_PARAMETER;

    retry:

    if(maxRetry++ >= 5)
        return rc;

    if( gClntInfo.ckptDbHdl == 0 || gClntInfo.ckptSvcHdlCount == 0)
    {
        CKPT_DEBUG_E(("CKPT Client is not yet initialized"));
        return CL_CKPT_ERR_NOT_INITIALIZED;
    }

    if(maxRetry > 1)
        clOsalTaskDelay(delay);

    /*
     * Safe to grab incase svc handle count is non zero or ckpt is initialized.
     */
    clOsalMutexLock(&gClntInfo.ckptClntMutex);

    if(!gClntInfo.ckptSvcHdlCount)
    {
        clOsalMutexUnlock(&gClntInfo.ckptClntMutex);
        return CL_CKPT_ERR_NOT_INITIALIZED;
    }
    
    if ((rc = ckptVerifyAndCheckout(ckptHdl, &ckptSvcHdl, &pInitInfo)) != CL_OK)
    {
        clOsalMutexUnlock(&gClntInfo.ckptClntMutex);
        return rc;
    }

    /*
     * Lock the mutex.
     */
    clOsalMutexLock(pInitInfo->ckptSvcMutex);
    
    /* 
     * Checkout the data associated with the checkpoint handle. 
     */
    rc = ckptHandleCheckout(ckptHdl, CL_CKPT_CHECKPOINT_HDL, 
                            (void **)&pHdlInfo);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
                   ("Ckpt: Handle create error rc[0x %x]\n",rc), rc);
              
    /*
     * Return ERROR in case the checkpoint was neither opened in write nor
     * create mode.
     */
    if(!((pHdlInfo->openFlag & CL_CKPT_CHECKPOINT_CREATE) ||
         (pHdlInfo->openFlag & CL_CKPT_CHECKPOINT_WRITE)))
    {
        rc = CL_CKPT_ERR_OP_NOT_PERMITTED;
        clHandleCheckin(gClntInfo.ckptDbHdl,ckptHdl);
    }
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR,
                   ("Ckpt: Improper permissions as ckpt was not opened in write"
                    " or create mode, rc[0x %x]\n",rc), rc);
              
    /* 
     * Checkin the data associated with the checkpoint handle. 
     */
    clHandleCheckin(gClntInfo.ckptDbHdl,ckptHdl);
    
    /*
     * Get the active handle and the active replica address associated
     * with that local ckptHdl.
     */
    rc = ckptActiveHandleGet(ckptHdl,&actHdl);   
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
                   ("Ckpt: Handle get error rc[0x %x]\n",rc), rc);
    rc = ckptActiveAddressGet(ckptHdl,&nodeAddr); 
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
                   ("Ckpt: Handle get error rc[0x %x]\n",rc), rc);
 
    rc = clIocRemoteNodeStatusGet(nodeAddr, &status);
    if(status == CL_IOC_NODE_DOWN)
    {
        clOsalMutexUnlock(pInitInfo->ckptSvcMutex);
        clHandleCheckin(gClntInfo.ckptDbHdl, ckptSvcHdl);
        clOsalMutexUnlock(&gClntInfo.ckptClntMutex);
        rc = CL_CKPT_ERR_NOT_EXIST;
        goto retry;
    }

    /* 
     * Fill the supported version information 
     */ 
    memcpy(&version,&clVersionSupported[0],sizeof(ClVersionT));

    /*
     * Send the call to the checkpoint active server.
     * Retry if the server is not not reachable.
     */
    rc = ckptIdlHandleUpdate(nodeAddr,pInitInfo->ckptIdlHdl, 
                             CL_CKPT_MAX_RETRY);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
                   ("Ckpt: Idl Handle update error rc[0x %x]\n",rc), rc);
    ckptIdlHdl = pInitInfo->ckptIdlHdl;
    clEoMyEoIocPortGet(&iocPort);

    rc = VDECL_VER(_ckptCheckpointWriteVectorClientSync, 4, 0, 0)( ckptIdlHdl,  actHdl,
                                                                   clIocLocalAddressGet(), iocPort,
                                                                   numberOfElements, 
                                                                   (ClCkptDifferenceIOVectorElementT*)pDifferenceIoVector,
                                                                   &tempError, &version);

    if(rc != CL_OK)
    {
        if(!CL_RMD_VERSION_ERROR(rc))
        {        
            tryAgain = clCkptHandleTypicalErrors(rc, ckptHdl,&nodeAddr);
        }
    }

    if(tryAgain)
    {
        clOsalMutexUnlock(pInitInfo->ckptSvcMutex);
        clHandleCheckin(gClntInfo.ckptDbHdl, ckptSvcHdl);
        clOsalMutexUnlock(&gClntInfo.ckptClntMutex);
        goto retry;
    }

    /*
     * Copy the value of error returned by the server.
     */
    if( pError != NULL )
    {
        *pError = tempError;
    }
    
    /* 
     * Check for the version mismatch. 
     */
    if((version.releaseCode != '\0') && (rc == CL_OK))
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL,
                   CL_CKPT_LOG_4_CLNT_VERSION_NACK, "Close call failed",
                   version.releaseCode,version.majorVersion,
                   version.minorVersion);
        rc = CL_CKPT_ERR_VERSION_MISMATCH;             
    }
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
                   ("Ckpt: Checkpoint write failed, rc[0x %x]\n",rc), rc);
    exitOnError:
    {
        /*
         * Unlock the mutex.
         */
        clOsalMutexUnlock(pInitInfo->ckptSvcMutex);

        /* 
         * Checkin the data associated with the service handle 
         */
        clHandleCheckin(gClntInfo.ckptDbHdl,ckptSvcHdl);
        clOsalMutexUnlock(&gClntInfo.ckptClntMutex);
        return rc;
    }
}

ClRcT clCkptCheckpointWriteLinear(ClCkptHdlT                     ckptHdl,
                                  const ClCkptIOVectorElementT   *pIoVector,
                                  ClUint32T                      numberOfElements,
                                  ClUint32T                      *pError)
{
    ClRcT               rc         = CL_OK;
    CkptInitInfoT       *pInitInfo = NULL;
    ClIocNodeAddressT   nodeAddr   = 0;
    ClCkptHdlT          actHdl     = CL_CKPT_INVALID_HDL;
    ClVersionT          version    = {0};
    CkptHdlDbT          *pHdlInfo  = 0;
    ClIdlHandleT        ckptIdlHdl = CL_CKPT_INVALID_HDL; 
    ClCkptSvcHdlT       ckptSvcHdl = CL_CKPT_INVALID_HDL;
    ClUint32T           tempError  = 0;
    ClBoolT             tryAgain   = CL_FALSE;
    ClUint32T           maxRetry   = 0;
    ClIocPortT          iocPort    = 0;
    ClUint8T            status = CL_IOC_NODE_DOWN;
    ClBoolT             clientUpdate = CL_FALSE;
    ClTimerTimeOutT     delay =  {.tsSec = 0, .tsMilliSec = 500 };

    /*
     * Input parameter verification.
     */
    if (NULL == pIoVector) return CL_CKPT_ERR_NULL_POINTER;
    if (0 == numberOfElements) return CL_CKPT_ERR_INVALID_PARAMETER;

    retry:

    if(maxRetry++ >= 5)
        return rc;

    if( gClntInfo.ckptDbHdl == 0 || gClntInfo.ckptSvcHdlCount == 0)
    {
        CKPT_DEBUG_E(("CKPT Client is not yet initialized"));
        return CL_CKPT_ERR_NOT_INITIALIZED;
    }

    if(maxRetry > 1)
        clOsalTaskDelay(delay);

    /*
     * Safe to grab incase svc handle count is non zero or ckpt is initialized.
     */
    clOsalMutexLock(&gClntInfo.ckptClntMutex);

    if(!gClntInfo.ckptSvcHdlCount)
    {
        clOsalMutexUnlock(&gClntInfo.ckptClntMutex);
        return CL_CKPT_ERR_NOT_INITIALIZED;
    }
    
    if ((rc = ckptVerifyAndCheckout(ckptHdl, &ckptSvcHdl, &pInitInfo)) != CL_OK)
    {
        clOsalMutexUnlock(&gClntInfo.ckptClntMutex);
        return rc;
    }

    /*
     * Lock the mutex.
     */
    clOsalMutexLock(pInitInfo->ckptSvcMutex);
    
    /* 
     * Checkout the data associated with the checkpoint handle. 
     */
    rc = ckptHandleCheckout(ckptHdl, CL_CKPT_CHECKPOINT_HDL, 
                            (void **)&pHdlInfo);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
                   ("Ckpt: Handle create error rc[0x %x]\n",rc), rc);
              
    /*
     * Return ERROR in case the checkpoint was neither opened in write nor
     * create mode.
     */
    if(!((pHdlInfo->openFlag & CL_CKPT_CHECKPOINT_CREATE) ||
         (pHdlInfo->openFlag & CL_CKPT_CHECKPOINT_WRITE)))
    {
        rc = CL_CKPT_ERR_OP_NOT_PERMITTED;
        clHandleCheckin(gClntInfo.ckptDbHdl,ckptHdl);
    }
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR,
                   ("Ckpt: Improper permissions as ckpt was not opened in write"
                    " or create mode, rc[0x %x]\n",rc), rc);
              
    /* 
     * Checkin the data associated with the checkpoint handle. 
     */
    clHandleCheckin(gClntInfo.ckptDbHdl,ckptHdl);
    
    /*
     * Get the active handle and the active replica address associated
     * with that local ckptHdl.
     */
    rc = ckptActiveHandleGet(ckptHdl,&actHdl);   
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
                   ("Ckpt: Handle get error rc[0x %x]\n",rc), rc);
    rc = ckptActiveAddressGet(ckptHdl,&nodeAddr); 
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
                   ("Ckpt: Handle get error rc[0x %x]\n",rc), rc);
 
    rc = clIocRemoteNodeStatusGet(nodeAddr, &status);
    if(status == CL_IOC_NODE_DOWN)
    {
        clOsalMutexUnlock(pInitInfo->ckptSvcMutex);
        clHandleCheckin(gClntInfo.ckptDbHdl, ckptSvcHdl);
        clOsalMutexUnlock(&gClntInfo.ckptClntMutex);
        rc = CL_CKPT_ERR_NOT_EXIST;
        goto retry;
    }

    /* 
     * Fill the supported version information 
     */ 
    memcpy(&version,&clVersionSupported[0],sizeof(ClVersionT));

    /*
     * Send the call to the checkpoint active server.
     * Retry if the server is not not reachable.
     */
    rc = ckptIdlHandleUpdate(nodeAddr,pInitInfo->ckptIdlHdl, 
                             CL_CKPT_MAX_RETRY);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
                   ("Ckpt: Idl Handle update error rc[0x %x]\n",rc), rc);
    ckptIdlHdl = pInitInfo->ckptIdlHdl;
    clEoMyEoIocPortGet(&iocPort);

    if( (pHdlInfo->creationFlag & CL_CKPT_PEER_TO_PEER_REPLICA) 
        && 
        !clientUpdate)
    {
        ClCkptClientInfoListT clientInfo = {0};
        rc = ckptClientInfoGet(pHdlInfo, ckptIdlHdl, actHdl, &clientInfo);
        if(rc != CL_OK)
        {
            clLogError("INFO", "GET", "Ckpt client info get returned with [%#x]", rc);
        }
        else if(clientInfo.numEntries > 0)
        {
            ClIdlHandleT clientIdlHdl = pInitInfo->ckptClientIdlHdl;
            ClCkptIOVectorElementT *pTmpVec = (ClCkptIOVectorElementT*)pIoVector;
            ClUint32T i;
            for(i = 0; i < numberOfElements; ++i)
            {
                if(pIoVector[i].sectionId.idLen == 0)
                    break;
            }
            /*
             * If we have a null or default section, then copy in defaultSection id
            */
            if(i != numberOfElements)
            {
                pTmpVec = clHeapCalloc(numberOfElements, sizeof(*pTmpVec));
                CL_ASSERT(pTmpVec != NULL);
                memcpy(pTmpVec, pIoVector, sizeof(*pTmpVec) * numberOfElements);
                while(i < numberOfElements)
                {
                    if(pTmpVec[i].sectionId.idLen == 0)
                    {
                        memcpy(&pTmpVec[i].sectionId, &gClDefaultSection, sizeof(gClDefaultSection));
                    }
                    ++i;
                }
            }
            for(i = 0; i < clientInfo.numEntries; ++i)
            {
                rc = clCkptClientIdlHandleUpdate(clientIdlHdl,
                                                 clientInfo.pClientInfo[i].nodeAddress,
                                                 clientInfo.pClientInfo[i].portId, 0);
                if(rc == CL_OK)
                {
                    clLogDebug("PEER", "WRITE", "Ckpt [%.*s] peer [%d:%d] write for section [%.*s],"
                               "vectors [%d]", 
                               pHdlInfo->ckptName.length, pHdlInfo->ckptName.value,
                               clientInfo.pClientInfo[i].nodeAddress, 
                               clientInfo.pClientInfo[i].portId,
                               pTmpVec->sectionId.idLen, pTmpVec->sectionId.id, numberOfElements);
                    rc = VDECL_VER(clCkptWriteUpdationNotificationClientAsync, 4, 0, 0)
                        (clientIdlHdl, &pHdlInfo->ckptName, numberOfElements, pTmpVec, NULL, NULL);

                    if(rc == CL_OK && !clientUpdate)
                    {
                        clientUpdate = CL_TRUE;
                    }
                }
            }
            if(pTmpVec != pIoVector)
            {
                clHeapFree(pTmpVec);
            }
        }
    }

    rc = VDECL_VER(_ckptCheckpointWriteClientSync, 4, 0, 0)( ckptIdlHdl,  actHdl,
                                                             clIocLocalAddressGet(), iocPort,
                                                             numberOfElements, 
                                                             (ClCkptIOVectorElementT *)pIoVector,
                                                             &tempError, &version);
    tryAgain = clCkptHandleTypicalErrors(rc, ckptHdl,&nodeAddr);

    if(tryAgain)
    {
        clOsalMutexUnlock(pInitInfo->ckptSvcMutex);
        clHandleCheckin(gClntInfo.ckptDbHdl, ckptSvcHdl);
        clOsalMutexUnlock(&gClntInfo.ckptClntMutex);
        goto retry;
    }

    /*
     * Copy the value of error returned by the server.
     */
    if( pError != NULL )
    {
        *pError = tempError;
    }
    
    /* 
     * Check for the version mismatch. 
     */
    if((version.releaseCode != '\0') && (rc == CL_OK))
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL,
                   CL_CKPT_LOG_4_CLNT_VERSION_NACK, "Close call failed",
                   version.releaseCode,version.majorVersion,
                   version.minorVersion);
        rc = CL_CKPT_ERR_VERSION_MISMATCH;             
    }
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
                   ("Ckpt: Checkpoint write failed, rc[0x %x]\n",rc), rc);
    exitOnError:
    {
        /*
         * Unlock the mutex.
         */
        clOsalMutexUnlock(pInitInfo->ckptSvcMutex);

        /* 
         * Checkin the data associated with the service handle 
         */
        clHandleCheckin(gClntInfo.ckptDbHdl,ckptSvcHdl);
        clOsalMutexUnlock(&gClntInfo.ckptClntMutex);
        return rc;
    }
}

ClRcT clCkptCheckpointWrite(ClCkptHdlT                     ckptHdl,
                            const ClCkptIOVectorElementT   *pIoVector,
                            ClUint32T                      numberOfElements,
                            ClUint32T                      *pError)
{
    ClRcT rc = CL_OK;
    ClUint32T versionCode;
    
    if(!numberOfElements || !pIoVector || !ckptHdl)
        return CL_CKPT_ERR_INVALID_PARAMETER;

    versionCode = CL_VERSION_CODE(CL_RELEASE_VERSION_BASE, CL_MAJOR_VERSION_BASE, CL_MINOR_VERSION_BASE);
    clNodeCacheMinVersionGet(NULL, &versionCode);

    if(versionCode >= CL_VERSION_CODE(5, 1, 0))
    {
        if(gClDifferentialCkpt)
        {
            ClCkptDifferenceIOVectorElementT *pDifferenceIoVector = NULL;
            ClUint32T i;
            pDifferenceIoVector = clHeapCalloc(numberOfElements, sizeof(*pDifferenceIoVector));
            CL_ASSERT(pDifferenceIoVector != NULL);
            for(i = 0; i < numberOfElements; ++i)
            {
                pDifferenceIoVector[i].differenceVector = clHeapCalloc(1, sizeof(*pDifferenceIoVector[i].differenceVector));
                CL_ASSERT(pDifferenceIoVector[i].differenceVector != NULL);
                pDifferenceIoVector[i].dataOffset = pIoVector[i].dataOffset;
                pDifferenceIoVector[i].dataSize = pIoVector[i].dataSize;
                memcpy(&pDifferenceIoVector[i].sectionId, &pIoVector[i].sectionId, sizeof(pDifferenceIoVector[i].sectionId));
                pDifferenceIoVector[i].differenceVector->dataVectors = clHeapCalloc(1, sizeof(*pDifferenceIoVector[i].differenceVector->dataVectors));
                pDifferenceIoVector[i].differenceVector->numDataVectors = 1;
                pDifferenceIoVector[i].differenceVector->dataVectors[0].dataBase = pIoVector[i].dataBuffer;
                pDifferenceIoVector[i].differenceVector->dataVectors[0].dataBlock = 
                    pIoVector[i].dataOffset >> CL_DIFFERENCE_VECTOR_BLOCK_SHIFT;
                pDifferenceIoVector[i].differenceVector->dataVectors[0].dataSize = pIoVector[i].dataSize;
                pDifferenceIoVector[i].differenceVector->md5Blocks = 0;
                pDifferenceIoVector[i].differenceVector->md5List = NULL;
            }
            rc = clCkptCheckpointWriteVector(ckptHdl, pDifferenceIoVector, numberOfElements, pError);
            for(i = 0; i < numberOfElements; ++i)
            {
                clHeapFree(pDifferenceIoVector[i].differenceVector->dataVectors);
                clHeapFree(pDifferenceIoVector[i].differenceVector);
            }
            clHeapFree(pDifferenceIoVector);
            goto out;
        }
    }       

    rc = clCkptCheckpointWriteLinear(ckptHdl, pIoVector, numberOfElements, pError);
    
    out:
    return rc;
}

/* 
 * Writes a single section in a given checkpoint
 */

ClRcT clCkptSectionOverwriteVector(ClCkptHdlT               ckptHdl,
                                   const ClCkptSectionIdT   *pSectionId,
                                   ClSizeT                  dataSize,
                                   ClDifferenceVectorT         *differenceVector)
{
    ClCkptSectionIdT   tempSecId  = {0};
    ClRcT              rc         = CL_OK;
    CkptInitInfoT      *pInitInfo = NULL;
    ClIocNodeAddressT  nodeAddr   = 0;
    ClCkptHdlT         actHdl     = 0;
    ClVersionT         version    = {0};
    ClIdlHandleT       ckptIdlHdl = 0;
    CkptHdlDbT         *pHdlInfo  = NULL;
    ClCkptSvcHdlT      ckptSvcHdl = CL_CKPT_INVALID_HDL;
    ClBoolT            tryAgain   = CL_FALSE;
    ClUint32T          maxRetry   = 0;
    ClIocPortT         iocPort    = 0;
    ClUint8T           status = CL_IOC_NODE_DOWN;
    ClTimerTimeOutT    delay = {.tsSec = 0, .tsMilliSec = 500};
    /*
     * Input parameter verification.
     */
    if (NULL == pSectionId) return CL_CKPT_ERR_NULL_POINTER;
    if (dataSize == 0) return CL_CKPT_ERR_INVALID_PARAMETER;
    if(!differenceVector || !differenceVector->numDataVectors) return CL_CKPT_ERR_INVALID_PARAMETER;

    /*
     * Check whether the client library is initialized or not.
     */
    retry:
    if(maxRetry++ >= 5)
    {
        if(tempSecId.id) clHeapFree(tempSecId.id);
        return rc;
    }

    if( gClntInfo.ckptDbHdl == 0 || gClntInfo.ckptSvcHdlCount == 0)
    {
        if(tempSecId.id) clHeapFree(tempSecId.id);
        CKPT_DEBUG_E(("CKPT Client is not yet initialized"));
        return CL_CKPT_ERR_NOT_INITIALIZED;
    }

    if(maxRetry > 1)
        clOsalTaskDelay(delay);

    /*
     * Safe to grab incase svc handle count is non zero or ckpt is initialized.
     */
    clOsalMutexLock(&gClntInfo.ckptClntMutex);

    if(!gClntInfo.ckptSvcHdlCount)
    {
        clOsalMutexUnlock(&gClntInfo.ckptClntMutex);
        if(tempSecId.id) clHeapFree(tempSecId.id);
        return CL_CKPT_ERR_NOT_INITIALIZED;
    }

    if ((rc = ckptVerifyAndCheckout(ckptHdl, &ckptSvcHdl, &pInitInfo)) != CL_OK)
    {
        clOsalMutexUnlock(&gClntInfo.ckptClntMutex);
        if(tempSecId.id) clHeapFree(tempSecId.id);
        return rc;
    }

    /*
     * Lock the mutex.
     */
    clOsalMutexLock(pInitInfo->ckptSvcMutex);
    rc = ckptHandleCheckout(ckptHdl, CL_CKPT_CHECKPOINT_HDL, 
                            (void **)&pHdlInfo);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
                   ("Ckpt: Handle create error rc[0x %x]\n",rc), rc);
              
    clEoMyEoIocPortGet(&iocPort);
    /*
     * Return ERROR in case the checkpoint was neither opened in write nor
     * create mode.
     */
    if(!((pHdlInfo->openFlag & CL_CKPT_CHECKPOINT_CREATE) ||
         (pHdlInfo->openFlag &  CL_CKPT_CHECKPOINT_WRITE)))
    {
        rc = CL_CKPT_ERR_OP_NOT_PERMITTED;
        clHandleCheckin(gClntInfo.ckptDbHdl,ckptHdl);
    }
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR,
                   ("Ckpt: Improper permissions as ckpt was not opened in write"
                    " or create mode, rc[0x %x]\n",rc), rc);

    /* 
     * Checkin the data associated with the checkpoint handle. 
     */
    rc = clHandleCheckin(gClntInfo.ckptDbHdl,ckptHdl);
    
    /*
     * Get the active handle and the active replica address associated
     * with that local ckptHdl.
     */
    rc = ckptActiveHandleGet(ckptHdl,&actHdl);   
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
                   ("Ckpt: Handle get error rc[0x %x]\n",rc), rc);
    rc = ckptActiveAddressGet(ckptHdl,&nodeAddr); 
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
                   ("Ckpt: Active server not exist rc[0x %x]\n",rc), rc);

    rc = clIocRemoteNodeStatusGet(nodeAddr, &status);
    if(status == CL_IOC_NODE_DOWN)
    {
        clOsalMutexUnlock(pInitInfo->ckptSvcMutex);
        clHandleCheckin(gClntInfo.ckptDbHdl, ckptSvcHdl);
        clOsalMutexUnlock(&gClntInfo.ckptClntMutex);
        rc = CL_CKPT_ERR_NOT_EXIST;
        goto retry;
    }

    if(!tempSecId.id)
    {
        tempSecId.idLen = pSectionId->idLen;
        tempSecId.id    = (ClUint8T *)clHeapCalloc(1,tempSecId.idLen);
        if(tempSecId.id == NULL)
        {
            CL_DEBUG_PRINT (CL_DEBUG_CRITICAL,
                            ("Ckpt:memory allocation is failed rc[0x %x]\n",rc));
            clLogWrite(CL_LOG_HANDLE_APP,CL_LOG_CRITICAL,CL_LOG_CKPT_LIB_NAME,
                       CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
            rc = CL_CKPT_ERR_NO_MEMORY;
        
            /*
             * Unlock the mutex.
             */
            clOsalMutexUnlock(pInitInfo->ckptSvcMutex);
            /* 
             * Checkin the data associated with the service handle 
             */
            clHandleCheckin(gClntInfo.ckptDbHdl,ckptSvcHdl);
            clOsalMutexUnlock(&gClntInfo.ckptClntMutex);
            return rc;
        }
        if(pSectionId->id)
            memcpy(tempSecId.id,pSectionId->id,tempSecId.idLen);
        /* 
         * Fill the supported version information 
         */ 
        memcpy(&version,&clVersionSupported[0],sizeof(ClVersionT));
    }
    /*
     * Send the call to the checkpoint active server.
     * Retry if the server is not not reachable.
     */
    rc = ckptIdlHandleUpdate(nodeAddr, pInitInfo->ckptIdlHdl,
                             CL_CKPT_MAX_RETRY);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
                   ("Ckpt: Idl Handle update error rc[0x %x]\n",rc), rc);
    ckptIdlHdl = pInitInfo->ckptIdlHdl;

    rc = VDECL_VER(_ckptSectionOverwriteVectorClientSync, 4, 0, 0)( ckptIdlHdl, actHdl,
                                                                    clIocLocalAddressGet(), iocPort,  
                                                                    CL_TRUE, &tempSecId, 0, dataSize, 
                                                                    differenceVector,
                                                                    &version);
    
    if(rc != CL_OK)
    {
        if(!CL_RMD_VERSION_ERROR(rc))
        {        
            tryAgain = clCkptHandleTypicalErrors(rc, ckptHdl,&nodeAddr);
        }
    }

    if(tryAgain)
    {
        clOsalMutexUnlock(pInitInfo->ckptSvcMutex);
        clHandleCheckin(gClntInfo.ckptDbHdl, ckptSvcHdl);
        clOsalMutexUnlock(&gClntInfo.ckptClntMutex);
        goto retry;
    }
    

    /* 
     * Check for the version mismatch. 
     */
    if((version.releaseCode != '\0') && (rc == CL_OK))
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL,
                   CL_CKPT_LOG_4_CLNT_VERSION_NACK, "Overwrite call failed",
                   version.releaseCode,version.majorVersion, 
                   version.minorVersion);
        rc = CL_CKPT_ERR_VERSION_MISMATCH;             
    }
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
                   ("Ckpt: Section Overwrite failed rc[0x %x]\n",rc), rc);
    exitOnError:
    {
        if(tempSecId.id != NULL)  clHeapFree(tempSecId.id);
        /*
         * Unlock the mutex.
         */
        clOsalMutexUnlock(pInitInfo->ckptSvcMutex);

        /* 
         * Checkin the data associated with the service handle 
         */
        clHandleCheckin(gClntInfo.ckptDbHdl,ckptSvcHdl);
        clOsalMutexUnlock(&gClntInfo.ckptClntMutex);
        return rc;
    }
}

ClRcT clCkptSectionOverwriteLinear(ClCkptHdlT               ckptHdl,
                                   const ClCkptSectionIdT   *pSectionId,
                                   const void               *pData,
                                   ClSizeT                  dataSize)
{
    ClCkptSectionIdT   tempSecId  = {0};
    ClRcT              rc         = CL_OK;
    CkptInitInfoT      *pInitInfo = NULL;
    ClIocNodeAddressT  nodeAddr   = 0;
    ClCkptHdlT         actHdl     = 0;
    ClVersionT         version    = {0};
    ClIdlHandleT       ckptIdlHdl = 0;
    CkptHdlDbT         *pHdlInfo  = NULL;
    ClCkptSvcHdlT      ckptSvcHdl = CL_CKPT_INVALID_HDL;
    ClBoolT            tryAgain   = CL_FALSE;
    ClUint32T          maxRetry   = 0;
    ClIocPortT         iocPort    = 0;
    ClUint8T           status = CL_IOC_NODE_DOWN;
    ClBoolT            clientUpdate = CL_FALSE;
    ClTimerTimeOutT    delay = {.tsSec = 0, .tsMilliSec = 500};
    /*
     * Input parameter verification.
     */
    if (NULL == pSectionId) return CL_CKPT_ERR_NULL_POINTER;
    if (NULL == pData) return CL_CKPT_ERR_NULL_POINTER;
    if (dataSize == 0) return CL_CKPT_ERR_INVALID_PARAMETER;

    /*
     * Check whether the client library is initialized or not.
     */
    retry:
    if(maxRetry++ >= 5)
    {
        if(tempSecId.id) clHeapFree(tempSecId.id);
        return rc;
    }

    if( gClntInfo.ckptDbHdl == 0 || gClntInfo.ckptSvcHdlCount == 0)
    {
        if(tempSecId.id) clHeapFree(tempSecId.id);
        CKPT_DEBUG_E(("CKPT Client is not yet initialized"));
        return CL_CKPT_ERR_NOT_INITIALIZED;
    }

    if(maxRetry > 1)
        clOsalTaskDelay(delay);

    /*
     * Safe to grab incase svc handle count is non zero or ckpt is initialized.
     */
    clOsalMutexLock(&gClntInfo.ckptClntMutex);

    if(!gClntInfo.ckptSvcHdlCount)
    {
        clOsalMutexUnlock(&gClntInfo.ckptClntMutex);
        if(tempSecId.id) clHeapFree(tempSecId.id);
        return CL_CKPT_ERR_NOT_INITIALIZED;
    }

    if ((rc = ckptVerifyAndCheckout(ckptHdl, &ckptSvcHdl, &pInitInfo)) != CL_OK)
    {
        clOsalMutexUnlock(&gClntInfo.ckptClntMutex);
        if(tempSecId.id) clHeapFree(tempSecId.id);
        return rc;
    }

    /*
     * Lock the mutex.
     */
    clOsalMutexLock(pInitInfo->ckptSvcMutex);

    rc = ckptHandleCheckout(ckptHdl, CL_CKPT_CHECKPOINT_HDL, (void **)&pHdlInfo);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, ("Ckpt: Handle checkout error rc[0x %x]\n",rc), rc);
              
    clEoMyEoIocPortGet(&iocPort);
    /*
     * Return ERROR in case the checkpoint was neither opened in write nor
     * create mode.
     */
    if(!((pHdlInfo->openFlag & CL_CKPT_CHECKPOINT_CREATE) ||
         (pHdlInfo->openFlag &  CL_CKPT_CHECKPOINT_WRITE)))
    {
        rc = CL_CKPT_ERR_OP_NOT_PERMITTED;
        clHandleCheckin(gClntInfo.ckptDbHdl,ckptHdl);
    }
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR,
                   ("Ckpt [%s]: Improper permissions as ckpt was not opened in write or create mode, rc[0x %x]\n",pHdlInfo->ckptName.value,rc), rc);

    /* 
     * Checkin the data associated with the checkpoint handle. 
     */
    rc = clHandleCheckin(gClntInfo.ckptDbHdl,ckptHdl);
    
    /*
     * Get the active handle and the active replica address associated
     * with that local ckptHdl.
     */
    rc = ckptActiveHandleGet(ckptHdl,&actHdl);   
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
                   ("Ckpt: Handle get error rc[0x %x]\n",rc), rc);
    rc = ckptActiveAddressGet(ckptHdl,&nodeAddr); 
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
                   ("Ckpt: Active server not exist rc[0x %x]\n",rc), rc);

    rc = clIocRemoteNodeStatusGet(nodeAddr, &status);
    if(status == CL_IOC_NODE_DOWN)
    {
        clOsalMutexUnlock(pInitInfo->ckptSvcMutex);
        clHandleCheckin(gClntInfo.ckptDbHdl, ckptSvcHdl);
        clOsalMutexUnlock(&gClntInfo.ckptClntMutex);
        rc = CL_CKPT_ERR_NOT_EXIST;
        goto retry;
    }
    /*
     * Copy the data to be sent across to the server side.
     * this is required for DEFAULT section.
     */
    if(!tempSecId.id)
    {
        tempSecId.idLen = pSectionId->idLen;
        tempSecId.id    = (ClUint8T *)clHeapCalloc(1,tempSecId.idLen);
        if(tempSecId.id == NULL)
        {
            CL_DEBUG_PRINT (CL_DEBUG_CRITICAL,
                            ("Ckpt:memory allocation is failed rc[0x %x]\n",rc));
            clLogWrite(CL_LOG_HANDLE_APP,CL_LOG_CRITICAL,CL_LOG_CKPT_LIB_NAME,
                       CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
            rc = CL_CKPT_ERR_NO_MEMORY;
        
            /*
             * Unlock the mutex.
             */
            clOsalMutexUnlock(pInitInfo->ckptSvcMutex);
            /* 
             * Checkin the data associated with the service handle 
             */
            clHandleCheckin(gClntInfo.ckptDbHdl,ckptSvcHdl);
            clOsalMutexUnlock(&gClntInfo.ckptClntMutex);
            return rc;
        }
        if(pSectionId->id)
            memcpy(tempSecId.id,pSectionId->id,tempSecId.idLen);
        /* 
         * Fill the supported version information 
         */ 
        memcpy(&version,&clVersionSupported[0],sizeof(ClVersionT));
    }

    /*
     * Send the call to the checkpoint active server.
     * Retry if the server is not not reachable.
     */
    rc = ckptIdlHandleUpdate(nodeAddr, pInitInfo->ckptIdlHdl,
                             CL_CKPT_MAX_RETRY);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
                   ("Ckpt: Idl Handle update error rc[0x %x]\n",rc), rc);
    ckptIdlHdl = pInitInfo->ckptIdlHdl;

    if((pHdlInfo->creationFlag & CL_CKPT_PEER_TO_PEER_REPLICA) && !clientUpdate)
    {
        ClCkptClientInfoListT clientInfo = {0};
        rc = ckptClientInfoGet(pHdlInfo, ckptIdlHdl, actHdl, &clientInfo);
        if(rc != CL_OK)
        {
            clLogError("INFO", "GET", "Ckpt client info get returned with [%#x]", rc);
        }
        else if(clientInfo.numEntries > 0)
        {
            ClCkptSectionIdT *pClientSection = &tempSecId;
            ClIdlHandleT clientIdlHdl = pInitInfo->ckptClientIdlHdl;
            if(!tempSecId.id || !tempSecId.idLen)
            {
                pClientSection = &gClDefaultSection;
            }
            for(ClUint32T i = 0; i < clientInfo.numEntries; ++i)
            {
                rc = clCkptClientIdlHandleUpdate(clientIdlHdl, 
                                                 clientInfo.pClientInfo[i].nodeAddress,
                                                 clientInfo.pClientInfo[i].portId, 0);
                if(rc == CL_OK)
                {
                    clLogDebug("PEER", "WRITE", "Ckpt [%.*s] peer [%d:%d] write for section [%.*s], "
                               "size [%lld]", pHdlInfo->ckptName.length, pHdlInfo->ckptName.value,
                               clientInfo.pClientInfo[i].nodeAddress, clientInfo.pClientInfo[i].portId,
                               pClientSection->idLen, pClientSection->id, dataSize);
                    rc = VDECL_VER(clCkptSectionUpdationNotificationClientAsync, 4, 0, 0)
                        (clientIdlHdl, &pHdlInfo->ckptName, pClientSection, 
                         dataSize, (ClUint8T*)pData, NULL, NULL);
                    if(rc == CL_OK && !clientUpdate)
                        clientUpdate = CL_TRUE;
                }
            }
        }
    }
            
    rc = VDECL_VER(_ckptSectionOverwriteClientSync, 4, 0, 0)( ckptIdlHdl, actHdl,
                                                              clIocLocalAddressGet(), iocPort,  
                                                              CL_TRUE, &tempSecId, 0, dataSize, 
                                                              (ClUint8T *) pData,
                                                              &version);
    tryAgain = clCkptHandleTypicalErrors(rc, ckptHdl,&nodeAddr);

    if(tryAgain)
    {
        clOsalMutexUnlock(pInitInfo->ckptSvcMutex);
        clHandleCheckin(gClntInfo.ckptDbHdl, ckptSvcHdl);
        clOsalMutexUnlock(&gClntInfo.ckptClntMutex);
        goto retry;
    }
    

    /* 
     * Check for the version mismatch. 
     */
    if((version.releaseCode != '\0') && (rc == CL_OK))
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL,
                   CL_CKPT_LOG_4_CLNT_VERSION_NACK, "Overwrite call failed",
                   version.releaseCode,version.majorVersion, 
                   version.minorVersion);
        rc = CL_CKPT_ERR_VERSION_MISMATCH;             
    }
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
                   ("Ckpt: Section Overwrite failed rc[0x %x]\n",rc), rc);
    exitOnError:
    {
        if(tempSecId.id != NULL)  clHeapFree(tempSecId.id);
       
        /*
         * Unlock the mutex.
         */
        clOsalMutexUnlock(pInitInfo->ckptSvcMutex);

        /* 
         * Checkin the data associated with the service handle 
         */
        clHandleCheckin(gClntInfo.ckptDbHdl,ckptSvcHdl);
        clOsalMutexUnlock(&gClntInfo.ckptClntMutex);
        return rc;
    }
}

ClRcT clCkptSectionOverwrite(ClCkptHdlT               ckptHdl,
                             const ClCkptSectionIdT   *pSectionId,
                             const void               *pData,
                             ClSizeT                  dataSize)
{
    ClUint32T version = CL_VERSION_CODE(CL_RELEASE_VERSION_BASE, CL_MAJOR_VERSION_BASE, CL_MINOR_VERSION_BASE);
    ClRcT rc  = CL_OK;

    clNodeCacheMinVersionGet(NULL, &version);
    if(version >= CL_VERSION_CODE(5, 1, 0))
    {
        if(gClDifferentialCkpt)
        {
            ClDifferenceVectorT differenceVector = {0};
            differenceVector.numDataVectors = 1;
            differenceVector.dataVectors = clHeapCalloc(1, sizeof(*differenceVector.dataVectors));
            CL_ASSERT(differenceVector.dataVectors != NULL);
            differenceVector.dataVectors[0].dataBlock = 0;
            differenceVector.dataVectors[0].dataSize = dataSize;
            differenceVector.dataVectors[0].dataBase = (ClUint8T*)pData;
            rc = clCkptSectionOverwriteVector(ckptHdl, pSectionId, dataSize, &differenceVector);
            clHeapFree(differenceVector.dataVectors);
            goto out;
        }
    }

    rc = clCkptSectionOverwriteLinear(ckptHdl, pSectionId, pData, dataSize);
    
    out:
    return rc;
}

/*
 * Reads multiple sections at a time. Can be used to read a single section
 */
 
ClRcT  clCkptCheckpointRead(ClCkptHdlT              ckptHdl,
                            ClCkptIOVectorElementT  *pIoVector,
                            ClUint32T               numberOfElements,
                            ClUint32T               *pError)
{
    ClRcT                   rc             = CL_OK;
    ClCkptIOVectorElementT  *pLocalVec     = NULL;
    ClCkptIOVectorElementT  *pTempLocalVec = NULL;
    ClCkptIOVectorElementT  *pTempOutVec   = NULL;
    ClUint32T               count          = 0;
    CkptInitInfoT           *pInitInfo     = NULL;
    ClIocNodeAddressT       nodeAddr       = 0;
    ClVersionT              version        = {0};
    ClCkptHdlT              actHdl         = CL_CKPT_INVALID_HDL;
    ClIdlHandleT            ckptIdlHdl     = CL_CKPT_INVALID_HDL;
    CkptHdlDbT              *pHdlInfo      = NULL;
    ClCkptSvcHdlT           ckptSvcHdl     = CL_CKPT_INVALID_HDL;
    ClUint32T               tempError      = 0;
    ClBoolT                 tryAgain       = CL_FALSE;
    ClUint32T               maxRetry       = 0;
    ClTimerTimeOutT delay = {.tsSec = 0, .tsMilliSec = 500 };

    /* Clear out the extra error field to not cause confusion */
    if( pError != NULL ) *pError = 0;
    
    /*
     * Input parameter verification.
     */
    if (NULL == pIoVector) return CL_CKPT_ERR_NULL_POINTER;
    if (0 == numberOfElements) return CL_CKPT_ERR_INVALID_PARAMETER; 
 
    if ((rc = ckptVerifyAndCheckout(ckptHdl, &ckptSvcHdl, &pInitInfo)) != CL_OK)
      {
        return rc;
      }

    /*
     * Lock the mutex.
     */
    clOsalMutexLock(pInitInfo->ckptSvcMutex);
    
    /* 
     * Checkout the data associated with the checkpoint handle. 
     */
    rc = ckptHandleCheckout(ckptHdl, CL_CKPT_CHECKPOINT_HDL, 
                            (void **)&pHdlInfo);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
              ("Ckpt: Handle create error rc[0x %x]\n",rc), rc);
              
    /*
     * Return ERROR in case the checkpoint was not opened in read mode.
     */
    if( CL_CKPT_CHECKPOINT_READ !=
             (pHdlInfo->openFlag & CL_CKPT_CHECKPOINT_READ))
    {         
        rc = CL_CKPT_ERR_OP_NOT_PERMITTED;
    }    
    
    /* 
     * Checkin the data associated with the service handle. 
     */
    clHandleCheckin(gClntInfo.ckptDbHdl,ckptHdl);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR,
            ("Ckpt: Improper permissions as ckpt was not opened in read"
             " mode, rc[0x %x]\n",rc), rc);
              
    /*
     * Get the active handle and the active replica address associated
     * with that local ckptHdl.
     */
    rc = ckptActiveHandleGet(ckptHdl,&actHdl);   
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
              ("Ckpt: Handle get error rc[0x %x]\n",rc), rc);
    rc = ckptActiveAddressGet(ckptHdl,&nodeAddr); 
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
              ("Ckpt: Active address get error rc[0x %x]\n",rc), rc);
 
    /* 
     * Fill the supported version information. 
     */ 
    memcpy(&version,&clVersionSupported[0],sizeof(ClVersionT));

    /*
     * Create local io vector to carry back the read information. This
     * would be used as out parameter in the read API.
     */
    pLocalVec = (ClCkptIOVectorElementT *)clHeapCalloc(1,
                          sizeof(ClCkptIOVectorElementT) * numberOfElements);
    if(pLocalVec == NULL)
    {
         CL_DEBUG_PRINT (CL_DEBUG_CRITICAL,
                  ("Ckpt:memory allocation is failed rc[0x %x]\n",rc));
         rc = CL_CKPT_ERR_NO_MEMORY;
         goto exitOnError;
    }

    /*
     * Incase buffer into which data has to be carried back hasnt been 
     * allocated, make the datasize to be read to 0. Server side will 
     * return entire data and NOT the requested size,
     */
    if(pIoVector->dataBuffer == NULL)
    {
        pIoVector->dataSize = 0;
    }
    
    /*
     * Send the call to the checkpoint active server.
     * Retry if the server is not reachable.
     */
    do
    {
        rc = ckptIdlHandleUpdate(nodeAddr, pInitInfo->ckptIdlHdl,
                                CL_CKPT_MAX_RETRY);
        CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
              ("Ckpt: Idl Handle update error rc[0x %x]\n",rc), rc);
        ckptIdlHdl = pInitInfo->ckptIdlHdl;

        if(nodeAddr != CL_CKPT_UNINIT_VALUE)
        {
            rc = VDECL_VER(_ckptCheckpointReadClientSync, 4, 0, 0)(ckptIdlHdl, actHdl,
                                                                   pIoVector, numberOfElements, pLocalVec,  
                                                                   &tempError, &version);
        }
        else rc = CL_IOC_RC(CL_IOC_ERR_COMP_UNREACHABLE);

        tryAgain = clCkptHandleTypicalErrors(rc, ckptHdl,&nodeAddr);
        
    } while(tryAgain == CL_TRUE && maxRetry++ < 10 && clOsalTaskDelay(delay) == CL_OK );

    /*
     * Copy the value of error returned by the server.
     */
    if( pError != NULL )
    {
        *pError = tempError;
    }
    
    /* 
     * Check for the version mismatch. 
     */
    if(version.releaseCode != '\0' && (rc == CL_OK))
    {
          clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL,
                    CL_CKPT_LOG_4_CLNT_VERSION_NACK, "Close call failed",
                    version.releaseCode,version.majorVersion, 
                    version.minorVersion);
          rc = CL_CKPT_ERR_VERSION_MISMATCH;             
    }
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
              ("Ckpt section does not exist rc[0x %x]\n",rc), rc);
    
    /*
     * Copy the read information to the user buffer.
     */
    pTempOutVec   = pIoVector; 
    pTempLocalVec = pLocalVec;
    for(count = 0; count < numberOfElements ;count++)
    {
        /*
         * Copy section by section.
         */
    	pIoVector->readSize = pLocalVec->readSize;
        
        /*
         * Incase user hasnt allocated buffer to carry back the read 
         * information, allocate the buffer. Copy the read information
         */
        if(pIoVector->dataBuffer == NULL)
        {
            pIoVector->dataBuffer = (ClUint8T *)clHeapAllocate(
                               pLocalVec->readSize + 1);
            if(pIoVector->dataBuffer == NULL)
            {
                CL_DEBUG_PRINT (CL_DEBUG_CRITICAL,
                        ("Ckpt:memory allocation is failed rc[0x %x]\n",rc));
                rc = CL_CKPT_ERR_NO_MEMORY;
                goto exitOnError;
            }
            memcpy(pIoVector->dataBuffer, pLocalVec->dataBuffer, 
                   pIoVector->readSize);
        }       
        else
        {
           memcpy(pIoVector->dataBuffer, pLocalVec->dataBuffer,
                  pIoVector->readSize);
        }
        
    	pIoVector->dataSize     = pLocalVec->dataSize;
    	pIoVector->dataOffset   = pLocalVec->dataOffset;
        pIoVector++;
        pLocalVec++;
    }
                          
    pLocalVec = pTempLocalVec;
    
    /*
     * Free the local io vector.
     */
    for(count = 0; count <numberOfElements; count++)
    {
       clHeapFree(pLocalVec->sectionId.id);
       clHeapFree(pLocalVec->dataBuffer);
       pLocalVec++;
    }
    pIoVector = pTempOutVec;
exitOnError:
   {
       /*
        * Unlock the mutex.
        */
       clOsalMutexUnlock(pInitInfo->ckptSvcMutex);

       /* 
        * Checkin the data associated with the service handle 
        */
       clHandleCheckin(gClntInfo.ckptDbHdl,ckptSvcHdl);
       clHeapFree(pTempLocalVec);
       return rc;
   }
}

ClRcT  clCkptCheckpointReadSections(ClCkptHdlT              ckptHdl,
                                    ClCkptIOVectorElementT  **ppIOVecs,
                                    ClUint32T               *pNumVecs)
{
    ClRcT                   rc             = CL_OK;
    CkptInitInfoT           *pInitInfo     = NULL;
    ClIocNodeAddressT       nodeAddr       = 0;
    ClCkptHdlT              actHdl         = CL_CKPT_INVALID_HDL;
    ClIdlHandleT            ckptIdlHdl     = CL_CKPT_INVALID_HDL;
    CkptHdlDbT              *pHdlInfo      = NULL;
    ClCkptSvcHdlT           ckptSvcHdl     = CL_CKPT_INVALID_HDL;
    ClBoolT                 tryAgain       = CL_FALSE;
    ClUint32T               maxRetry       = 0;
    ClTimerTimeOutT delay = {.tsSec = 0, .tsMilliSec = 500 };

    /*
     * Input parameter verification.
     */
    if (!ppIOVecs || !pNumVecs) return CL_CKPT_ERR_NULL_POINTER;
 
    if ((rc = ckptVerifyAndCheckout(ckptHdl, &ckptSvcHdl, &pInitInfo)) != CL_OK)
    {
        return rc;
    }
    
    *ppIOVecs = NULL;
    *pNumVecs = 0;

    /*
     * Lock the mutex.
     */
    clOsalMutexLock(pInitInfo->ckptSvcMutex);
    
    /* 
     * Checkout the data associated with the checkpoint handle. 
     */
    rc = ckptHandleCheckout(ckptHdl, CL_CKPT_CHECKPOINT_HDL, 
                            (void **)&pHdlInfo);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
                   ("Ckpt: Handle create error rc[0x %x]\n",rc), rc);
              
    /*
     * Return ERROR in case the checkpoint was not opened in read mode.
     */
    if( CL_CKPT_CHECKPOINT_READ !=
        (pHdlInfo->openFlag & CL_CKPT_CHECKPOINT_READ))
    {         
        rc = CL_CKPT_ERR_OP_NOT_PERMITTED;
    }    
    
    /* 
     * Checkin the data associated with the service handle. 
     */
    clHandleCheckin(gClntInfo.ckptDbHdl,ckptHdl);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR,
                   ("Ckpt: Improper permissions as ckpt was not opened in read"
                    " mode, rc[0x %x]\n",rc), rc);
              
    /*
     * Get the active handle and the active replica address associated
     * with that local ckptHdl.
     */
    rc = ckptActiveHandleGet(ckptHdl,&actHdl);   
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
                   ("Ckpt: Handle get error rc[0x %x]\n",rc), rc);
    rc = ckptActiveAddressGet(ckptHdl,&nodeAddr); 
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
                   ("Ckpt: Active address get error rc[0x %x]\n",rc), rc);
 
    /*
     * Send the call to the checkpoint active server.
     * Retry if the server is not reachable.
     */
    do
    {
        rc = ckptIdlHandleUpdate(nodeAddr, pInitInfo->ckptIdlHdl,
                                 CL_CKPT_MAX_RETRY);
        CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
                       ("Ckpt: Idl Handle update error rc[0x %x]\n",rc), rc);
        ckptIdlHdl = pInitInfo->ckptIdlHdl;

        if(nodeAddr != CL_CKPT_UNINIT_VALUE)
        {
            rc = VDECL_VER(_ckptCheckpointReadSectionsClientSync, 6, 0, 0)(ckptIdlHdl,
                                                                           actHdl,
                                                                           ppIOVecs, 
                                                                           pNumVecs);
            if(rc == CL_OK) break;
        }
        else rc = CL_IOC_RC(CL_IOC_ERR_COMP_UNREACHABLE);

        tryAgain = clCkptHandleTypicalErrors(rc, ckptHdl, &nodeAddr);
        
    } while(tryAgain == CL_TRUE && maxRetry++ < 10 && clOsalTaskDelay(delay) == CL_OK );

    exitOnError:
    /*
     * Unlock the mutex.
     */
    clOsalMutexUnlock(pInitInfo->ckptSvcMutex);

    /* 
     * Checkin the data associated with the service handle 
     */
    clHandleCheckin(gClntInfo.ckptDbHdl, ckptSvcHdl);

    if(rc != CL_OK && *ppIOVecs && *pNumVecs > 0)
    {
        clCkptIOVectorFree(*ppIOVecs, *pNumVecs);
        clHeapFree(*ppIOVecs);
        *ppIOVecs = NULL;
        *pNumVecs = 0;
    }

    return rc;
}


/*
 * Synchronizes the replicas of a checkpoint 
 */
 
ClRcT clCkptCheckpointSynchronize(ClCkptHdlT ckptHdl,
                                  ClTimeT    timeout)
{
    ClRcT              rc         = CL_OK;
    CkptInitInfoT      *pInitInfo = NULL;
    ClCkptHdlT         ckptActHdl = 0;
    ClIocNodeAddressT  nodeAddr   = 0; 
    ClIdlHandleT       ckptIdlHdl = 0;
    ClVersionT         version    = {0};
    ClCkptSvcHdlT      ckptSvcHdl = CL_CKPT_INVALID_HDL;
    CkptHdlDbT         *pHdlInfo  = NULL;
    ClBoolT            tryAgain   = CL_FALSE;
    ClUint32T          maxRetry   = 0;
     
    if ((rc = ckptVerifyAndCheckout(ckptHdl, &ckptSvcHdl, &pInitInfo)) != CL_OK)
      {
        return rc;
      }

    /*
     * Lock the mutex.
     */
    clOsalMutexLock(pInitInfo->ckptSvcMutex);
    
    /* 
     * Checkout the data associated with the checkpoint handle.
     */
    rc = ckptHandleCheckout(ckptHdl, CL_CKPT_CHECKPOINT_HDL, 
                            (void **)&pHdlInfo);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
              ("Ckpt: Handle create error rc[0x %x]\n",rc), rc);
              
    /*
     * Return ERROR in case the checkpoint was neither opened in write nor
     * create mode.
     */
    if(!((pHdlInfo->openFlag & CL_CKPT_CHECKPOINT_CREATE) ||
         (pHdlInfo->openFlag & CL_CKPT_CHECKPOINT_WRITE)))
    {
        rc = CL_CKPT_ERR_OP_NOT_PERMITTED;
        clHandleCheckin(gClntInfo.ckptDbHdl,ckptHdl);
        CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR,
                ("Ckpt: Improper permissions as ckpt was not opened in write"
                 " or create mode, rc[0x %x]\n",rc), rc);
    }
    
    /*
     * Return ERROR in case the checkpoint was of SYNC nature.
     */
    if(pHdlInfo->creationFlag & CL_CKPT_WR_ALL_REPLICAS)
    {
        rc = CL_CKPT_ERR_BAD_OPERATION;
        clHandleCheckin(gClntInfo.ckptDbHdl,ckptHdl);
        CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
         ("Ckpt: API not allowed for SYNC checkpoint, rc[0x %x]\n",rc), rc);
    }
    
    /* 
     * Checkin the data associated with the checkpoint handle.
     */
    rc = clHandleCheckin(gClntInfo.ckptDbHdl,ckptHdl);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
              ("Ckpt: Handle Check-in Error  rc[0x %x]\n",rc), rc);

    /*
     * Obtain the active address and active handle associated with the
     * local handle.
     */
    rc = ckptActiveHandleGet(ckptHdl,&ckptActHdl); 
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
              ("Ckpt: Handle get error rc[0x %x]\n",rc), rc);
    rc = ckptActiveAddressGet(ckptHdl,&nodeAddr); 
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
              ("Ckpt: Handle get error rc[0x %x]\n",rc), rc);
              
    /* 
     * Fill the supported version information. 
     */ 
    memcpy(&version,&clVersionSupported[0],sizeof(ClVersionT));

    /*
     * Send the call to the checkpoint active server.
     * Retry if the server is not not reachable.
     */
    do
    {
        rc = ckptIdlHandleUpdate(nodeAddr, pInitInfo->ckptIdlHdl,
                                CL_CKPT_MAX_RETRY);
        CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
              ("Ckpt: Idl Handle update error rc[0x %x]\n",rc), rc);
        ckptIdlHdl = pInitInfo->ckptIdlHdl;

        rc = VDECL_VER(_ckptCheckpointSynchronizeClientSync, 4, 0, 0)(ckptIdlHdl,
                                    ckptActHdl, timeout, 0,
                                    ckptHdl, &version);
        tryAgain = clCkptHandleTypicalErrors(rc, ckptHdl,&nodeAddr);

    }while((tryAgain == CL_TRUE) && (maxRetry++ < 2));

    /* 
     * Check for the version mismatch. 
     */
    if((version.releaseCode != '\0') && (rc == CL_OK))
    {
          clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL,
                    CL_CKPT_LOG_4_CLNT_VERSION_NACK, "Close call failed",
                    version.releaseCode,version.majorVersion, 
                    version.minorVersion);
          rc = CL_CKPT_ERR_VERSION_MISMATCH;             
    }
exitOnError:
    {
    /*
     * Unlock the mutex.
     */
    clOsalMutexUnlock(pInitInfo->ckptSvcMutex);
    
    /* 
     * Checkin the data associated with the service handle 
     */
    clHandleCheckin(gClntInfo.ckptDbHdl,ckptSvcHdl);
    return rc;
    }
}



/*
 * Synchronize async related callback function.
 */
 
void _ckptSynchronizeCallback(ClIdlHandleT  handle,
                              ClHandleT     ckptActHdl, 
                              ClInt64T      timeout,
                              ClUint16T     flag,
                              ClCkptHdlT    ckptHdl,
                              ClVersionT    *suppVersion,
                              ClRcT         retCode,
                              void*         pInitData)
{
    CkptInitInfoT    *pInitInfo   = NULL;
    CkptQueueInfoT   *pQueueData  = NULL;
    CkptSyncCbInfoT  *pSyncInfo   = NULL;
    ClInvocationT     invocation  = *(ClInvocationT *)pInitData;
    ClCharT           chr         = 'c';
    ClCkptSvcHdlT     ckptSvcHdl  = CL_CKPT_INVALID_HDL;
    ClRcT             rc          = CL_OK;

    /*
     * Check for version mismatch.
     */
    if(suppVersion->releaseCode != '\0')
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL,
                CL_CKPT_LOG_4_CLNT_VERSION_NACK, "Synchrnoize call failed",
                suppVersion->releaseCode,suppVersion->majorVersion, 
                suppVersion->minorVersion);
        rc = CL_CKPT_ERR_VERSION_MISMATCH;             
    }
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
            ("Ckpt: Checkpoint synchronize failed rc[0x %x]\n",rc), rc);

    /* 
     * Checkout the data associated with the service handle. 
     */

#if 0  
    rc = ckptSvcHandleGet(ckptHdl,&ckptSvcHdl);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
            ("Ckpt: Checkpoint Synchronize failed rc[0x %x]\n",rc), rc);
    ckptHandleCheckout(ckptSvcHdl, CL_CKPT_SERVICE_HDL, (void **)&pInitInfo);
#endif
    /* Andrew Stone: Note that this code does not check the result of HandleCheckout,
       and that the statements below rely on a good pInitInfo.
       Replaced with a std subroutine that does check */
    if ((rc = ckptVerifyAndCheckout(ckptHdl, &ckptSvcHdl, &pInitInfo)) != CL_OK)
      {
        goto exitOnError;
      }

    /*
     * Lock the selection obj related mutex.
     */
    clOsalMutexLock(pInitInfo->ckptSelObjMutex);
    
    if(pInitInfo->pCallback != NULL)
    {
        if(pInitInfo->cbFlag == CL_TRUE)
        {
            /*
             * User has called selection object get.
             */

            /*
             * Store the relevent info into the queue
             */
            pQueueData  = (CkptQueueInfoT *)clHeapAllocate(
                    sizeof(CkptQueueInfoT));
            memset(pQueueData , 0 ,sizeof(CkptQueueInfoT));

            pQueueData->callbackId = CL_CKPT_SYNC_CALLBACK;
            pSyncInfo = (CkptSyncCbInfoT *)clHeapAllocate(
                    sizeof(CkptSyncCbInfoT)); 
            if(pSyncInfo == NULL)
            {
                CL_DEBUG_PRINT (CL_DEBUG_CRITICAL,
                        ("Ckpt:memory allocation is failed rc[0x %x]\n",rc));
                clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_CRITICAL, 
                           CL_LOG_CKPT_LIB_NAME,
                           CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
                /*
                 * Unock the selection obj related mutex.
                 */
                clOsalMutexUnlock(pInitInfo->ckptSelObjMutex);

                /* 
                 * Checkin the data associated with the service handle 
                 */
                clHandleCheckin(gClntInfo.ckptDbHdl,ckptSvcHdl);
                return ;
            }
            memset(pSyncInfo , 0 ,sizeof(CkptSyncCbInfoT));
            pSyncInfo->invocation   = invocation;
            pSyncInfo->error        = retCode;
            pQueueData->pData       = (CkptSyncCbInfoT *)pSyncInfo;    

            rc = clQueueNodeInsert(pInitInfo->cbQueue, 
                                   (ClQueueDataT)pQueueData);
            if(write(pInitInfo->writeFd,(void *)&chr, 1) != 1)
                clLogError("CKP", "SYNC", "Write to dispatch pipe returned [%s]",
                           strerror(errno));
        }	
        else
        {
            /* 
             * Seletion object get not called. Call the registered callback 
             * function immediately.
             */
            pInitInfo->pCallback->
                 checkpointSynchronizeCallback(invocation,rc);
        }
    }

    /*
     * Unock the selection obj related mutex.
     */
    clOsalMutexUnlock(pInitInfo->ckptSelObjMutex);
    
    /* 
     * Checkin the data associated with the service handle 
     */
    clHandleCheckin(gClntInfo.ckptDbHdl,ckptSvcHdl);
    return ;
exitOnError:
    return ;
}



/*
 * Synchronizes the replicas of a checkpoint asynchronously
 */
 
ClRcT clCkptCheckpointSynchronizeAsync(ClCkptHdlT    ckptHdl,
                                       ClInvocationT invocation)
{
    ClRcT               rc           = CL_OK;
    CkptInitInfoT       *pInitInfo   = NULL;
    ClCkptHdlT          ckptActHdl   = CL_CKPT_INVALID_HDL;
    ClIocNodeAddressT   nodeAddr     = 0; 
    ClIdlHandleT        ckptIdlHdl   = CL_CKPT_INVALID_HDL;
    ClVersionT          version      = {0};
    ClCkptSvcHdlT       ckptSvcHdl   = CL_CKPT_INVALID_HDL;
    ClInvocationT       *pInvocation = NULL;
    CkptHdlDbT          *pHdlInfo    = NULL;
    ClBoolT             tryAgain     = CL_FALSE;
    ClUint32T           maxRetry     = 0;

    if ((rc = ckptVerifyAndCheckout(ckptHdl, &ckptSvcHdl, &pInitInfo)) != CL_OK)
      {
        return rc;
      }

    /*
     * Lock the mutex.
     */
    clOsalMutexLock(pInitInfo->ckptSvcMutex);

    /*
     * Return ERROR if synchronize related callback is not present.
     */
    if(pInitInfo->pCallback->checkpointSynchronizeCallback == NULL)
    {
        rc = CL_CKPT_ERR_INITIALIZED;
        CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
                  ("Ckpt: Ckpt open async failed, rc=[0x %x]\n",rc), rc);
    }
    
    /* 
     * Checkout the data associated with the checkpoint handle. 
     */
    rc = ckptHandleCheckout(ckptHdl, CL_CKPT_CHECKPOINT_HDL, 
                            (void **)&pHdlInfo);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
              ("Ckpt: Handle create error rc[0x %x]\n",rc), rc);
              
    /*
     * Return ERROR in case the checkpoint was neither opened in write nor
     * create mode.
     */
    if(!((pHdlInfo->openFlag & CL_CKPT_CHECKPOINT_CREATE) ||
         (pHdlInfo->openFlag & CL_CKPT_CHECKPOINT_WRITE)))
    {
        rc = CL_CKPT_ERR_OP_NOT_PERMITTED;
        clHandleCheckin(gClntInfo.ckptDbHdl,ckptHdl);
        CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR,
                ("Ckpt: Improper permissions as ckpt was not opened in write"
                 " or create mode, rc[0x %x]\n",rc), rc);
    }
    
    /*
     * Return ERROR in case the checkpoint was of SYNC nature.
     */
    if(pHdlInfo->creationFlag & CL_CKPT_WR_ALL_REPLICAS)
    {
        rc = CL_CKPT_ERR_BAD_OPERATION;
        clHandleCheckin(gClntInfo.ckptDbHdl,ckptHdl);
        CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
                  ("Ckpt: Creation Flag are not proper rc[0x %x]\n",rc), rc);
    }
    
    /* 
     * Checkin the data associated with the checkpoint handle. 
     */
    rc = clHandleCheckin(gClntInfo.ckptDbHdl,ckptHdl);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
              ("Ckpt: Handle Check-in Error  rc[0x %x]\n",rc), rc);
              
    /*
     * Obtain the active address and active handle associated with the
     * local handle.
     */
    rc = ckptActiveHandleGet(ckptHdl,&ckptActHdl); 
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
              ("Ckpt: Handle get error rc[0x %x]\n",rc), rc);
    rc = ckptActiveAddressGet(ckptHdl,&nodeAddr); 
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
              ("Ckpt: Handle get error rc[0x %x]\n",rc), rc);

    /* 
     * Fill the supported version information. 
     */ 
    memcpy(&version,&clVersionSupported[0],sizeof(ClVersionT));
    pInvocation  = (ClInvocationT *)clHeapAllocate(sizeof(ClInvocationT));
    if(pInvocation == NULL)
    {
       rc =  CL_CKPT_ERR_NO_MEMORY;
       clLogWrite(CL_LOG_HANDLE_APP,CL_LOG_CRITICAL,CL_LOG_CKPT_LIB_NAME,
                  CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
       CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
              ("Ckpt: CheckpoinAsync open failed rc[0x %x]\n",rc), rc);
    }
    memset(pInvocation , 0 , sizeof(ClInvocationT));
    *pInvocation          = invocation;

    /*
     * Send the call to the checkpoint active server.
     * Retry if the server is not not reachable.
     */
    do
    {
        rc = ckptIdlHandleUpdate(nodeAddr, pInitInfo->ckptIdlHdl, 
                                 CL_CKPT_MAX_RETRY);
        CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
              ("Ckpt: Idl Handle update error rc[0x %x]\n",rc), rc);
        ckptIdlHdl = pInitInfo->ckptIdlHdl;

        rc = VDECL_VER(_ckptCheckpointSynchronizeClientAsync, 4, 0, 0)(ckptIdlHdl,
                    ckptActHdl,0,1,ckptHdl,
                    &version,
                    _ckptSynchronizeCallback,pInvocation);
        tryAgain = clCkptHandleTypicalErrors(rc, ckptHdl,&nodeAddr);
   }while((tryAgain == CL_TRUE) && (maxRetry++ < 2));
exitOnError:
    {
        /*
         * Unlock the mutex.
         */
        clOsalMutexUnlock(pInitInfo->ckptSvcMutex);

        /* 
         * Checkin the data associated with the service handle 
         */
        clHandleCheckin(gClntInfo.ckptDbHdl, ckptSvcHdl);
        return rc;
    }
}



/*
 * Function for initializing the checkpoint library.
 */
 
ClRcT clCkptInitialize(ClCkptSvcHdlT            *pCkptSvcHandle,    
                       const ClCkptCallbacksT   *pCallbacks,       
                       ClVersionT               *pVersion)        
{
    ClIdlAddressT      address       = {0};
    ClIdlHandleObjT    idlObj        = CL_IDL_HANDLE_INVALID_VALUE;
    ClEoExecutionObjT  *pThis        = NULL;
    CkptInitInfoT      *pInitInfo    = NULL;
    ClIocNodeAddressT  mastNodeAddr  = 0;
    ClRcT              rc            = CL_OK;
    ClVersionT         evtVersion    = CL_EVENT_VERSION;

    CKPT_DEBUG_T(("pCkptHdl : %p pCallbacks: %p pVersion: %p\n",
                   (void *)pCkptSvcHandle, (void *)pCallbacks, 
                   (void *) pVersion));

    if(gClDifferentialCkpt == -1)
    {
        gClDifferentialCkpt = clParseEnvBoolean("CL_ASP_DIFFERENTIAL_CKPT") ? 1 : 0;
    }
        
    /*
     * Validate the input parameters.
     */
    if ((pCkptSvcHandle == NULL) || (pVersion == NULL)) 
    {
        rc = CL_CKPT_ERR_NULL_POINTER;
        CKPT_DEBUG_E(("Ckpt:Initialize failed rc[0x%x]\n",rc));
        return rc;
    }
    
    *pCkptSvcHandle = CL_CKPT_INVALID_HDL;

    /* 
     * Version verify. 
     */
    rc = clVersionVerify(&versionDatabase,pVersion);
    if( CL_OK != rc)
    {
        CKPT_DEBUG_E(("Ckpt:Initialize failed rc[0x %x]\n",rc));
        return rc;
    }    
    if( CL_OK != clCkptMasterAddressGet(&mastNodeAddr) ) 
    {
        return CL_CKPT_ERR_TRY_AGAIN;
    }
    /*
     * Create a mutex to safeguard the top level client information that 
     * contains the handle database, service initialize count, ckpt handle
     * list and event related information. This information is unique per
     * checkpoint library.
     */
    if (!gClntInfo.ckptSvcHdlCount)
    {
        /*
         * First time initialize. Hence create the mutex.
         */
        rc = clOsalMutexInit(&gClntInfo.ckptClntMutex);
        if(rc != CL_OK)
        {
            CKPT_DEBUG_E(("Ckpt:Initialize failed rc[0x %x]\n",rc));
            return rc;
        }
    }
                
    /*
     * Lock the top level client information.
     */
    clOsalMutexLock(&gClntInfo.ckptClntMutex); 
    
    /*
     * Allocate memeory for the required data structures.
     */
    if (!gClntInfo.ckptSvcHdlCount)
    {
       /*
        * First instance of initialization.
        */
        
       /* 
        * Create a handle database that will contain all 
        * service, checkpoint and iteration related handles.
        */
        rc = clHandleDatabaseCreate(NULL, &gClntInfo.ckptDbHdl);
        CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
                ("Ckpt:Initialize failed rc[0x %x]\n",rc), rc);

        /* 
         * Create a list that will contain all the handles associated with 
         * all the checkpoints opened with that client (on per checkpoint 
         * basis). This is required in case active replica of a checkpoint
         * has changed and we need to update the active address in all the
         * handles associated with that checkpoint.
         */
        rc = clCntLlistCreate(ckptHdlKeyCompare,
                ckptHdlDeleteCallback,
                ckptHdlDeleteCallback,
                CL_CNT_NON_UNIQUE_KEY,
                &gClntInfo.ckptHdlList);
        CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR,
                ("Ckpt:nitialize failed rc[0x %x]\n",rc), rc);
    }

    /*
     * Create the service handle to be returned to the user.
     */
    rc = clHandleCreate(gClntInfo.ckptDbHdl, sizeof(CkptInitInfoT), 
                        pCkptSvcHandle);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
            ("Ckpt:Initialize failed rc[0x %x]\n",rc), rc);

    /*
     * Associate user defined callbacks, selection object related info,
     * idl handle for making calls to the server and a mutex to safeguard 
     * these information with the service handle.
     */
    rc = clHandleCheckout(gClntInfo.ckptDbHdl, *pCkptSvcHandle,
                          (void **)&pInitInfo);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
            ("Ckpt:Initialize failed rc[0x %x]\n",rc), rc);

    /*
     * Create a list for storing iteration handles.
     */
    rc = clCntLlistCreate(ckptHdlKeyCompare,
            ckptSecHdlDeleteCallback,
            ckptSecHdlDeleteCallback,
            CL_CNT_UNIQUE_KEY,
            &pInitInfo->hdlList);
    if( rc != CL_OK)                        
    {
        clHandleCheckin(gClntInfo.ckptDbHdl, *pCkptSvcHandle);
    }
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
            ("Ckpt:Initialize failed rc[0x %x]\n",rc), rc);

    /*
     * User defined callbacks.
     */
    if (pCallbacks)
    {
        pInitInfo->pCallback = (ClCkptCallbacksT *)clHeapCalloc(1, 
                sizeof(ClCkptCallbacksT));
        if (NULL == pInitInfo->pCallback )
        {	
            rc = CL_CKPT_ERR_NO_MEMORY;
            clHandleCheckin(gClntInfo.ckptDbHdl, *pCkptSvcHandle);
            CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
                    ("Ckpt:Initialize failed rc[0x %x]\n",rc), rc);
        }
        pInitInfo->pCallback->checkpointOpenCallback =
            pCallbacks->checkpointOpenCallback;
        pInitInfo->pCallback->checkpointSynchronizeCallback =
            pCallbacks->checkpointSynchronizeCallback;
    }

    /*
     * Selection object related information.
     */
    pInitInfo->readFd    = 0;
    pInitInfo->writeFd   = 0;
    pInitInfo->cbFlag    = CL_FALSE;
    pInitInfo->cbQueue   = NULL;

    /*
     * idl related information.
     */
    pInitInfo->mastNodeAddr = mastNodeAddr;
    pInitInfo->hdlType      = CL_CKPT_SERVICE_HDL;
    memset(&address, 0,sizeof(ClIdlAddressT));
    address.addressType = CL_IDL_ADDRESSTYPE_IOC ;
    address.address.iocAddress.iocPhyAddress.nodeAddress  = mastNodeAddr;
    address.address.iocAddress.iocPhyAddress.portId       =
        CL_IOC_CKPT_PORT;
    idlObj.address = address;
    idlObj.flags   = CL_RMD_CALL_DO_NOT_OPTIMIZE;
    idlObj.options.timeout = 5000;
    idlObj.options.priority = CL_RMD_DEFAULT_PRIORITY;
    idlObj.options.retries  = 0;
    rc = clIdlHandleInitialize(&idlObj,&pInitInfo->ckptIdlHdl);
    if(rc != CL_OK)
    {
        clHandleCheckin(gClntInfo.ckptDbHdl, *pCkptSvcHandle);
    }
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR,
            ("Ckpt:Initialize failed rc[0x %x]\n",rc), rc);

    rc = clCkptClientIdlHandleInit(&pInitInfo->ckptClientIdlHdl);
    if(rc != CL_OK)
    {
        clHandleCheckin(gClntInfo.ckptDbHdl, *pCkptSvcHandle);
    }
    CKPT_ERR_CHECK(CL_CKPT_LIB, CL_DEBUG_ERROR,
                   ("Ckpt: Initialize failed with [%#x]\n", rc), rc);

    /*
     * Mutex for safeguarding the associated selection object.
     */
    rc = clOsalMutexCreate(&pInitInfo->ckptSelObjMutex);
    if(rc != CL_OK)
    {
        clHandleCheckin(gClntInfo.ckptDbHdl, *pCkptSvcHandle);
    }
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
            ("Ckpt:Initialize failed rc[0x %x]\n",rc), rc);
            
    /*
     * Mutex for safeguarding service handle related information.
     */
    rc = clOsalMutexCreate(&pInitInfo->ckptSvcMutex);
    if(rc != CL_OK)
    {
        clOsalMutexDelete(pInitInfo->ckptSelObjMutex);
        clHandleCheckin(gClntInfo.ckptDbHdl, *pCkptSvcHandle);
    }
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR, 
            ("Ckpt:Initialize failed rc[0x %x]\n",rc), rc);

    clHandleCheckin(gClntInfo.ckptDbHdl, *pCkptSvcHandle);        

    /*
     * Initialize the event client library. This is needed in case active
     * adddress of non-collocated checkpoint gets changed.
     */
    if (!gClntInfo.ckptSvcHdlCount)
    {
        /*
         * First instance of initialization.
         */

         /*
          * In case some failure occurs during event library initialization,
          * normal checkpoint operations will still work. Problems will 
          * occur only if the active address of non-collocated checkpoint
          * gets updated.
          */
        rc = clEventInitialize(&gClntInfo.ckptEvtHdl, &ckptEvtCallbacks, &evtVersion);
        if(CL_OK != rc)
        {
            CKPT_DEBUG_E(("Ckpt:Initialize failed in rc[0x %x]\n",rc));
            goto exitOnError;
        }
        else
        {
            rc = clEventChannelOpen(gClntInfo.ckptEvtHdl, &ckptSubChannelName,
                    CL_EVENT_GLOBAL_CHANNEL | CL_EVENT_CHANNEL_SUBSCRIBER,
                    (ClTimeT)-1, &gClntInfo.ckptChannelSubHdl);
            if(CL_OK != rc)
            {
                CKPT_DEBUG_E(("Ckpt:Ckpt Initialize failed in rc[0x %x]\n",
                            rc));
                CKPT_DEBUG_E(("Sync Checkpoint will work fine. Async "
                            "Checkpoint Operation will fail if activeaddress"
                            "is changed.\n"));
                clEventFinalize(gClntInfo.ckptEvtHdl);
            }
            else
            {
                rc = clEventSubscribe(gClntInfo.ckptChannelSubHdl, NULL , 2, 
                                      NULL);
                if(CL_OK != rc)
                {
                    CKPT_DEBUG_E(("Ckpt:Ckpt Initialize failed in rc"
                                "[0x %x]\n",rc));
                    CKPT_DEBUG_E(("Sync Checkpoint will work fine. Async "
                                "Checkpoint Operation will fail if active"
                                "address is changed.\n"));
                    clEventChannelClose(gClntInfo.ckptChannelSubHdl);
                    clEventFinalize(gClntInfo.ckptEvtHdl);
                }
            }
        }
    }   
    /*
     * Increment the service initialization count.
     */
    gClntInfo.ckptSvcHdlCount++;
    /*
     * Install the client function to receive notification for 
     * any checkpoint update, even it failed , we can continue to
     * provide servie other checkpoints of DISTRIBUTED.
     */
    clCkptClntEoClientInstall();

    rc = clEoMyEoObjectGet(&pThis);
    CL_ASSERT(rc == CL_OK);
    
    gClntInfo.ckptClientCacheDisable = clParseEnvBoolean("CL_CKPT_CLIENT_CACHE_DISABLE");
    gClntInfo.ckptOwnAddr.nodeAddress = clIocLocalAddressGet();
    gClntInfo.ckptOwnAddr.portId = pThis->eoPort;

    rc = clCkptEoClientTableRegister(CL_IOC_CKPT_PORT);
    CL_ASSERT(rc == CL_OK);

    rc = clCkptClntEoClientTableRegister(pThis->eoPort);
    CL_ASSERT(rc == CL_OK);

    /*
     * Unlock the top level client info.
     */
    clOsalMutexUnlock(&gClntInfo.ckptClntMutex);
    
    return rc;
exitOnError:
    {
        /*
         * Do the necessary cleanup.
         */
        if (pInitInfo != NULL)
        {
            if (!pInitInfo->hdlList) 
                clCntDelete(pInitInfo->hdlList);
            if(pInitInfo->pCallback != NULL) 
                clHeapFree(pInitInfo->pCallback);
        }
        if (*pCkptSvcHandle != CL_CKPT_INVALID_HDL)
        {
            clHandleDestroy(gClntInfo.ckptDbHdl, *pCkptSvcHandle);
        }
        if ((!gClntInfo.ckptSvcHdlCount) && gClntInfo.ckptHdlList)
        {
            clCntDelete(gClntInfo.ckptHdlList);
        }
        if ((!gClntInfo.ckptSvcHdlCount) && gClntInfo.ckptDbHdl)
        {
            clHandleDatabaseDestroy(gClntInfo.ckptDbHdl);
        }
        clOsalMutexUnlock(&gClntInfo.ckptClntMutex);
        return rc; 
    }
}



/*
 * Function for finalizing the checkpointing client library.
 */
 
ClRcT clCkptFinalize(ClCkptSvcHdlT ckptSvcHdl)
{
    CkptInitInfoT      *pInitInfo     = NULL;
    ClRcT              rc             = CL_OK;
    ClIocNodeAddressT  nodeAddr       = 0;
    ClIocPortT         iocPort        = 0;
    ClVersionT         ckptVersion    = {0};
    ClCntNodeHandleT   nodeHdl        = 0;
    ClCntNodeHandleT   tempNode       = 0;
    ClCkptSvcHdlT      ckptTempSvcHdl = CL_CKPT_INVALID_HDL;
    ClCkptHdlT         *pCkptHdl      = NULL;
    ClCkptHdlT         ckptTempHdl    = CL_CKPT_INVALID_HDL;
    CkptHdlDbT         *pHdlInfo      = NULL;

    CKPT_DEBUG_T(("CkptSvcHdl : %#llX\n", ckptSvcHdl));  

    /*
     * Check whether any checkpoint library initialization was
     * done or not.
     */
    if (!gClntInfo.ckptSvcHdlCount)
    {
        CKPT_DEBUG_E(("CKPT Client is not yet initialized"));
        return CL_CKPT_ERR_NOT_INITIALIZED;
    }

    /*
     * Lock the top level client info.
     */
    clOsalMutexLock(&gClntInfo.ckptClntMutex); 
    
    /*
     * Checkout the information associated with the service handle.
     */
    rc = ckptHandleCheckout(ckptSvcHdl, CL_CKPT_SERVICE_HDL,
                            (void **)&pInitInfo);
    if(rc != CL_OK)
    {
        CKPT_DEBUG_E(("Ckpt:Invalid Service Handle rc[0x %x]\n",rc));
        clOsalMutexUnlock(&gClntInfo.ckptClntMutex); 
        return CL_CKPT_ERR_INVALID_HANDLE;
    }

    /*
     * Lock selection obj related info.
     */
    clOsalMutexLock(pInitInfo->ckptSelObjMutex); 

    /*
     * Delete selection object related info.
     */
    if (pInitInfo->cbFlag)
    {
        close(pInitInfo->writeFd);
        close(pInitInfo->readFd);
    }

    /*
     * Unlock selection obj related info and delete the mutex.
     */
    clOsalMutexUnlock(pInitInfo->ckptSelObjMutex); 
    clOsalMutexDelete(pInitInfo->ckptSelObjMutex);
    
    /*
     * Lock service handle related info.
     */
    clOsalMutexLock(pInitInfo->ckptSvcMutex); 
    
    /*
     * Delete the associated checkpoint handle list and the callbacks.
     */
    
    if(pInitInfo->pCallback != NULL)
        clHeapFree(pInitInfo->pCallback);
        
    /*
     * Delete the list for storing iteration handles.
     */
    clCntDelete(pInitInfo->hdlList);
    
    
    /*
     * Send the call to the master to close all the checkpoints that had
     * been opened by this client.
     */
    memcpy(&ckptVersion,&clVersionSupported[0],sizeof(ClVersionT));
    rc = clCkptMasterAddressGet(&nodeAddr);
    if (rc != CL_OK)
    {
         CKPT_DEBUG_E(("clCkptMasterAddressGet failed with rc 0x%x",rc));  
    }
    rc = ckptIdlHandleUpdate(nodeAddr, pInitInfo->ckptIdlHdl,
                             CL_CKPT_NO_RETRY);
    nodeAddr  = clIocLocalAddressGet();
    clEoMyEoIocPortGet(&iocPort);

    rc = VDECL_VER(clCkptServerFinalizeClientSync, 4, 0, 0)(pInitInfo->ckptIdlHdl,
            &ckptVersion,
            nodeAddr,iocPort);
    
    /*
     * Walk through the list,delete all ckptHdls corresponding
     * to service Handle.
     */
    clCntFirstNodeGet(gClntInfo.ckptHdlList, &nodeHdl);
    while (nodeHdl)
    {
        pHdlInfo = NULL;
        ckptTempSvcHdl = 0;
        clCntNodeUserDataGet(gClntInfo.ckptHdlList,nodeHdl,(ClCntDataHandleT *)&pCkptHdl);
        tempNode = nodeHdl;
        clCntNextNodeGet(gClntInfo.ckptHdlList, nodeHdl, &nodeHdl);
        rc = ckptHandleCheckout(*pCkptHdl, CL_CKPT_CHECKPOINT_HDL, (void **)&pHdlInfo);
        if(rc == CL_OK)
        {
            if(pHdlInfo)
            {
                ckptTempSvcHdl = pHdlInfo->ckptSvcHdl;
                ckptClientCacheFree(pHdlInfo);
            }
            clHandleCheckin(gClntInfo.ckptDbHdl, *pCkptHdl);
        }
        if (ckptTempSvcHdl == ckptSvcHdl)
        {
            ckptTempHdl = *pCkptHdl;
            clCntNodeDelete(gClntInfo.ckptHdlList, tempNode); 
            clHandleDestroy(gClntInfo.ckptDbHdl, ckptTempHdl);
        }
    }

    rc = CL_OK;

    /*
     * Unlock selection obj related info and delete the mutex.
     */
    clOsalMutexUnlock(pInitInfo->ckptSvcMutex); 
    clOsalMutexDelete(pInitInfo->ckptSvcMutex);
    
    /*
     * Destroy the service handle.
     */
    clHandleCheckin(gClntInfo.ckptDbHdl, ckptSvcHdl);
    clHandleDestroy(gClntInfo.ckptDbHdl, ckptSvcHdl);    


    /*
     * Decrement the service initialization count.
     */
    gClntInfo.ckptSvcHdlCount--;

    /*
     * Finalize the event client library.
     */
    if( !gClntInfo.ckptSvcHdlCount)
    {
        clEventUnsubscribe(gClntInfo.ckptChannelSubHdl, 2);
        /*
         * Drop the ckpt clnt mutex as if the ckpteventcallback
         * is blocked on this mutex, the event finalize in flight
         * event receive status check is going to be blocked forever
         * or deadlocked.   
         */
        clOsalMutexUnlock(&gClntInfo.ckptClntMutex);
        clEventFinalize(gClntInfo.ckptEvtHdl);
        clOsalMutexLock(&gClntInfo.ckptClntMutex);
        clCntDelete(gClntInfo.ckptHdlList);
        /*coverity:suppress*/
        gClntInfo.ckptHdlList = 0;
        clHandleDatabaseDestroy(gClntInfo.ckptDbHdl);    
        gClntInfo.ckptDbHdl = 0;
    }
    
    /*
     * Unlock the top level client info and delete the associated
     * mutex if service count becomes 0.
     */
    clOsalMutexUnlock(&gClntInfo.ckptClntMutex);
    
    return rc;
}



/*
 * Callback function that would be invoked whenever there is a change in
 * requested checkpoint data.
 */
 
ClRcT clCkptImmediateConsumptionRegister(
                       ClCkptHdlT                    ckptLocalHdl,
                       ClCkptNotificationCallbackT   callback,
                       ClPtrT                        pCookie)
{
    ClRcT          rc         = CL_OK;
    CkptHdlDbT     *pHdlInfo  = NULL;
    ClCkptSvcHdlT  svcHdl     = CL_CKPT_INVALID_HDL;
    CkptInitInfoT  *pInitInfo = NULL;
    ClIocPortT     iocPort    = 0;
    ClHandleT      actHdl     = 0;

    /*
     * Checkout the data associated with the checkpoint handle.
     */
    rc = ckptHandleCheckout((ClHandleT)ckptLocalHdl, CL_CKPT_CHECKPOINT_HDL, 
            (void **)&pHdlInfo);
    if(rc != CL_OK)
    {
        return rc;
    }

    CL_ASSERT(pHdlInfo != NULL);

    /*
     * If the checkpoint is not distributed, allow this, otherwise say error
     * CL_ERR_OP_NOT_PERMITTED
     */
    if(! (pHdlInfo->creationFlag & CL_CKPT_DISTRIBUTED) )
    {
        clLogError(CL_CKPT_AREA_CLIENT, CL_CKPT_CTX_REG,
                "Checkpoint doesn't have proper creation flags to register"
                "NotificationCallback");
        clHandleCheckin(gClntInfo.ckptDbHdl,(ClHandleT)ckptLocalHdl);
        return CL_CKPT_ERR_OP_NOT_PERMITTED;
    }

    /*
     * Get the svc handle associated with the ckpt handle and lock the
     * mutex associated with that service handle.
     */
    svcHdl = pHdlInfo->ckptSvcHdl;

    /* 
     * Checkout the data associated with the service handle. 
     */
    rc = ckptHandleCheckout(svcHdl, CL_CKPT_SERVICE_HDL, 
            (void **)&pInitInfo);
    if(rc != CL_OK)
    {
        clLogError(CL_CKPT_AREA_CLIENT, CL_CKPT_CTX_REG,
                "Checkpoint service handle [%#llX] associated with"
                "checkpoint handle[%#llX] is not proper", 
                svcHdl, ckptLocalHdl);
        clHandleCheckin(gClntInfo.ckptDbHdl,(ClHandleT)ckptLocalHdl);
        return CL_CKPT_ERR_INVALID_HANDLE;
    }

    /*
     * Lock the mutex.
     */
    rc = clOsalMutexLock(pInitInfo->ckptSvcMutex);
    if( CL_OK != rc )
    {
        clLogError(CL_CKPT_AREA_CLIENT, CL_CKPT_CTX_REG,
                "Locing init mutex failed for handle [%#llX] with rc[0x %x]",
                ckptLocalHdl, rc);
        goto handleChkin;
    }


    rc = ckptActiveHandleGet(ckptLocalHdl, &actHdl); 
    if( CL_OK != rc )
    {
        clLogError(CL_CKPT_AREA_CLIENT, CL_CKPT_CTX_REG, 
                "Failed to find the corresponding active handle for hdl [%#llX]",
                ckptLocalHdl);
        goto unlockNexit;
    }

    rc = ckptIdlHandleUpdate(clIocLocalAddressGet(), pInitInfo->ckptIdlHdl,
            CL_CKPT_MAX_RETRY);
    if( CL_OK != rc )
    {
        clLogError(CL_CKPT_AREA_CLIENT, CL_CKPT_CTX_REG, 
                "Failed to update idl handle for checkpoint handle [%#llX]",
                ckptLocalHdl);
        goto unlockNexit;
    }

    clEoMyEoIocPortGet(&iocPort);

    rc = VDECL_VER(clCkptActiveCallbackNotifyClientSync, 4, 0, 0)(pInitInfo->ckptIdlHdl, actHdl,
            clIocLocalAddressGet(),
            iocPort);
    if( CL_OK != rc )
    {
        clLogError(CL_CKPT_AREA_CLIENT, CL_CKPT_CTX_REG, 
                "Failed to update the active server for handle [%#llX]",
                ckptLocalHdl);
        goto unlockNexit;
    }
    /*
     * Associate the passed callback with the checkpoint info.
     */
    if(pHdlInfo != NULL)
    {
        pHdlInfo->notificationCallback    = callback;
        pHdlInfo->pCookie                 = pCookie;
    }

unlockNexit:
    /*
     * Unlock the mutex.
     */
    clOsalMutexUnlock(pInitInfo->ckptSvcMutex);
handleChkin:    
    /*
     * Checkin the data associated with the checkpoint handle.
     */
    clHandleCheckin(gClntInfo.ckptDbHdl,(ClHandleT)ckptLocalHdl);
    /* 
     * Checkin the data associated with the service handle 
     */
    clHandleCheckin(gClntInfo.ckptDbHdl, svcHdl);

    return rc;
}

/* 
 * This routine is a call back routine for the subscribed events.
 * Client subscribes for two types of events:- 
 * 1. Immediate consumption.
 * 2. Active replica change.
 */
 
void ckptEventCallback(ClEventSubscriptionIdT    subscriptionId,
                       ClEventHandleT            eventHandle,
                       ClSizeT                   eventDataSize )
{
    ClRcT                  rc          = CL_OK;
    ClCkptClientUpdInfoT   payLoad     = {0};
    ClSizeT                payLoadLen  = sizeof(ClCkptClientUpdInfoT);
    ClCntNodeHandleT       nodeHdl     = 0;
    ClUint32T              cksum       = 0;
    ClUint32T              tempsum     = 0;
    CkptHdlDbT             *pHdlInfo   = NULL;
    ClCkptHdlT             *pCkptHdl   = NULL;
    ClCkptSvcHdlT          svcHdl      = CL_CKPT_INVALID_HDL;
    CkptInitInfoT          *pInitInfo  = NULL;

    /*
     * Get the payload associated with the received event.
     */
    rc = clEventDataGet (eventHandle, &payLoad,  &payLoadLen);
    if(rc != CL_OK)
    {
        clLogError(CL_CKPT_AREA_CLIENT, CL_LOG_CONTEXT_UNSPECIFIED,
                   "Ckpt: Event Data Get failed  [Rc: 0x%x]" , rc);
        return;
    }

    payLoad.name.length = ntohs(payLoad.name.length);

    clLogNotice(CL_CKPT_AREA_CLIENT, CL_LOG_CONTEXT_UNSPECIFIED, 
            "Received address change event for checkpoint [%.*s] with address [%d]", payLoad.name.length, payLoad.name.value,
            ntohl(payLoad.actAddr));
    /*
     * Get all the handles associated with the checkpoint for which the
     * event is received.
     */
    if (payLoad.name.length > payLoadLen)
      {
        clLogError(CL_CKPT_AREA_CLIENT, CL_LOG_CONTEXT_UNSPECIFIED,"Event data is corrupted.  Name length [%d] > message length [%lu]",payLoad.name.length, (long unsigned int) payLoadLen);
    	return;
      }
    
    clCksm32bitCompute ((ClUint8T *)payLoad.name.value,
                        payLoad.name.length, &cksum);

    clOsalMutexLock(&gClntInfo.ckptClntMutex);

    rc = clCntNodeFind(gClntInfo.ckptHdlList, (ClPtrT)(ClWordT)cksum, &nodeHdl);
    if( CL_OK != rc )
    {
        clOsalMutexUnlock(&gClntInfo.ckptClntMutex);
        clEventFree(eventHandle);
        return ;
    }
    
    if(gClCkptReplicaChangeCallback
       && 
       ntohl(payLoad.eventType) == CL_CKPT_ACTIVE_REP_CHG_EVENT
       && 
       ntohl(payLoad.actAddr) != CL_CKPT_UNINIT_VALUE)
    {
        if(CL_OK != gClCkptReplicaChangeCallback((const ClNameT*)&payLoad.name, 
                                                 ntohl(payLoad.actAddr)))
        {
            clOsalMutexUnlock(&gClntInfo.ckptClntMutex);
            clEventFree(eventHandle);
            clLogWarning("REP", "CHG", 
                         "Ignoring replica change callback for ckpt [%.*s] since it was disallowed for node [%#x]",
                         payLoad.name.length, payLoad.name.value, ntohl(payLoad.actAddr));
            return;
        }
    }

    tempsum = cksum;

    /*
     * For a given checkpoint, there can be multiple opens.
     * So iterate through all possible handles.
     */
    while(nodeHdl != 0 && tempsum == cksum)
    {
        ClCntKeyHandleT key = 0;
        (void)clCntNodeUserDataGet(gClntInfo.ckptHdlList, nodeHdl,
                             (ClCntDataHandleT *)&pCkptHdl);
        (void)clCntNextNodeGet(gClntInfo.ckptHdlList,nodeHdl,&nodeHdl);
        if( 0 != nodeHdl )
        {
            (void)clCntNodeUserKeyGet(gClntInfo.ckptHdlList,nodeHdl,&key);
        }
        tempsum = (ClUint32T)(ClWordT) key;
        if(!pCkptHdl)
            continue;
        /* 
         * Checkout the data associated with the checkpoint handle. 
         */
        rc = ckptHandleCheckout(*pCkptHdl, CL_CKPT_CHECKPOINT_HDL, 
                                (void **)&pHdlInfo);
        CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                       ("Ckpt: Event Data Get failed  [Rc: 0x%x]" , rc), rc);
        
        /*
         * Get the svc handle associated with the ckpt handle and lock the
         * mutex associated with that service handle.
         */
        svcHdl = pHdlInfo->ckptSvcHdl;

        /* 
         * Checkout the data associated with the service handle. 
         */
        rc = ckptHandleCheckout(svcHdl, CL_CKPT_SERVICE_HDL, 
                                (void **)&pInitInfo);
        if(rc != CL_OK)
        {
            clOsalMutexUnlock(&gClntInfo.ckptClntMutex);
            CKPT_DEBUG_E(("\nPassed handle is invalid\n"));
            return;
        }

        /*
         * Lock the mutex.
         */
        clOsalMutexLock(pInitInfo->ckptSvcMutex);
        
        /*
         * Check the event type and take appropriate action. 
         * Also make sure that the checkpoint names match since there could be multiple ones 
         * with the same checksum.
         */
        if(ntohl(payLoad.eventType) == CL_CKPT_ACTIVE_REP_CHG_EVENT
           && 
           pHdlInfo->ckptName.length == payLoad.name.length
           &&
           !strncmp((const ClCharT*)pHdlInfo->ckptName.value, 
                    (const ClCharT*)payLoad.name.value, payLoad.name.length))
        {
            /*
             * Active replica change event. Update the active replica address
             * only if its not a local address
             */
            if(ntohl(payLoad.actAddr) == CL_CKPT_UNINIT_VALUE && pHdlInfo->activeAddr == clIocLocalAddressGet())
            {
                clLogInfo(CL_CKPT_AREA_CLIENT, CL_CKPT_CTX_NOTIFICATION,
                          "Skipping updation of Active address to uninitialized address as local address [%d] is currently active replica",
                          pHdlInfo->activeAddr);
            }
            else
            {

                clLogInfo(CL_CKPT_AREA_CLIENT, CL_CKPT_CTX_NOTIFICATION,
                          "Active address is getting changed from [%d] to [%d] for [%.*s]",
                          pHdlInfo->activeAddr, ntohl(payLoad.actAddr), payLoad.name.length,
                          payLoad.name.value);
                pHdlInfo->activeAddr = ntohl(payLoad.actAddr);

            }
        }
        /* 
         * Checkin the data associated with the checkpoint handle. 
         */
        clHandleCheckin(gClntInfo.ckptDbHdl, *pCkptHdl);
        
        /*
         * Unlock the mutex.
         */
        clOsalMutexUnlock(pInitInfo->ckptSvcMutex);

        /* 
         * Checkin the data associated with the service handle 
         */
        clHandleCheckin(gClntInfo.ckptDbHdl, svcHdl);
    }
    
    /*
     * Free the event handle.
     */
    clEventFree(eventHandle);
    exitOnError:
    {
        clOsalMutexUnlock(&gClntInfo.ckptClntMutex);
        return;
    }
}

ClRcT
VDECL_VER(clCkptSectionUpdationNotification, 4, 0, 0)(ClNameT          *pName,
                                  ClCkptSectionIdT *pSecId,      
                                  ClUint32T        dataSize,   
                                  ClUint8T         *pData) 
{
    ClRcT                        rc             = CL_OK;
    ClUint32T                    cksum          = 0;
    ClCkptNotificationCallbackT  notifyCallback = 0;
    ClCntNodeHandleT             nodeHdl        = NULL;
    ClUint32T                    tempsum        = 0;
    ClCkptHdlT                   *pCkptHdl      = NULL;
    CkptHdlDbT                   *pHdlInfo      = NULL;
    ClCkptSvcHdlT                svcHdl         = CL_CKPT_INVALID_HDL;
    CkptInitInfoT                *pInitInfo     = NULL;
    ClCkptIOVectorElementT       iov            = {{0}};
    ClPtrT                       pCookie        = NULL;

    iov.sectionId.id    = pSecId->id;
    iov.sectionId.idLen = pSecId->idLen;
    iov.dataSize        = dataSize;
    iov.readSize        = dataSize;
    iov.dataOffset      = 0;
    iov.dataBuffer      = clHeapCalloc(dataSize, sizeof(ClCharT)); 
    if( NULL == iov.dataBuffer )
    {
        clLogError(CL_CKPT_AREA_CLIENT, CL_CKPT_CTX_NOTIFICATION,
                   "Memory allocation failed during notification");
        return CL_CKPT_ERR_NO_MEMORY;
    }
    memcpy(iov.dataBuffer, pData, dataSize);

    /*
     * Get all the handles associated with the checkpoint for which the
     * notification is received. 
     */
    clCksm32bitCompute ((ClUint8T *)pName->value, pName->length, &cksum);
    rc = clCntNodeFind(gClntInfo.ckptHdlList, (ClPtrT)(ClWordT)cksum, &nodeHdl);
    if( CL_OK != rc )
    {
        clLogError(CL_CKPT_AREA_CLIENT, CL_CKPT_CTX_NOTIFICATION,
                "Not able find checkpoint [%.*s] information for cksum"
                "[%d] rc [0x %x]", pName->length, pName->value, cksum, rc);
        clHeapFree(iov.dataBuffer);
        return rc;
    }
    tempsum = cksum;

    /*
     * For a given checkpoint, there can be multiple opens.
     * So iterate through all possible handles.
     */
    while((nodeHdl != 0) && (tempsum == cksum))
    {
        ClCntKeyHandleT key = 0;

        rc = clCntNodeUserDataGet(gClntInfo.ckptHdlList, nodeHdl,
                (ClCntDataHandleT *) &pCkptHdl);
        if( CL_OK != rc )
        {
            clLogError(CL_CKPT_AREA_CLIENT, CL_CKPT_CTX_NOTIFICATION,
                    "Not able find checkpoint [%.*s] information rc [0x %x]",
                    pName->length, pName->value, rc);
            clHeapFree(iov.dataBuffer);
            return rc;
        }
        rc = clCntNodeUserKeyGet(gClntInfo.ckptHdlList,nodeHdl,&key);
        if( CL_OK != rc )
        {
            clLogError(CL_CKPT_AREA_CLIENT, CL_CKPT_CTX_NOTIFICATION,
                    "Not able find checkpoint [%.*s] information rc [0x %x]",
                    pName->length, pName->value, rc);
            clHeapFree(iov.dataBuffer);
            return rc;
        }
        tempsum = (ClUint32T)(ClWordT) key;

        /* 
         * Checkout the data associated with the checkpoint handle. 
         */
        rc = ckptHandleCheckout(*pCkptHdl, CL_CKPT_CHECKPOINT_HDL, 
                (void **)&pHdlInfo);
        if( CL_OK != rc )
        {
            clLogError(CL_CKPT_AREA_CLIENT, CL_CKPT_CTX_NOTIFICATION, 
                       "Checkpoint handle [%#llX] checkout failed",
                       *pCkptHdl);
            clHeapFree(iov.dataBuffer);
            return rc;
        }

        /*
         * Get the svc handle associated with the ckpt handle and lock the
         * mutex associated with that service handle.
         */
        svcHdl = pHdlInfo->ckptSvcHdl;

        /* 
         * Checkout the data associated with the service handle. 
         */
        rc = ckptHandleCheckout(svcHdl, CL_CKPT_SERVICE_HDL, 
                (void **)&pInitInfo);
        if(rc != CL_OK)
        {
            clHandleCheckin(gClntInfo.ckptDbHdl, *pCkptHdl);
            clHeapFree(iov.dataBuffer);
            return rc;
        }


        /*
         * Immediate consumption related event. Call the registered
         * callback.
         */
        notifyCallback = pHdlInfo->notificationCallback;
        pCookie        = pHdlInfo->pCookie;
        /* 
         * Checkin the data associated with the checkpoint handle. 
         */
        clHandleCheckin(gClntInfo.ckptDbHdl, *pCkptHdl);

        /* 
         * Checkin the data associated with the service handle 
         */
        clHandleCheckin(gClntInfo.ckptDbHdl, svcHdl);

        if( notifyCallback != NULL 
            &&
            pHdlInfo->ckptName.length == pName->length
            &&
            !strncmp((const ClCharT*)pHdlInfo->ckptName.value, 
                     (const ClCharT*)pName->value, pName->length))
        {
            notifyCallback(*pCkptHdl, pName, &iov, 1, pCookie);
        }
        rc = clCntNextNodeGet(gClntInfo.ckptHdlList,nodeHdl,&nodeHdl);
        if( CL_OK != rc )
        {
            break;
        }
    }

    clHeapFree(iov.dataBuffer);

    return CL_OK;
}

ClRcT
VDECL_VER(clCkptWriteUpdationNotification, 4, 0, 0)(ClNameT                 *pName,
                                ClUint32T               numSections,
                                ClCkptIOVectorElementT  *pIoVector)
{
    ClRcT                        rc             = CL_OK;
    ClUint32T                    cksum          = 0;
    ClCkptNotificationCallbackT  notifyCallback = 0;
    ClCntNodeHandleT             nodeHdl        = NULL;
    ClUint32T                    tempsum        = 0;
    ClCkptHdlT                   *pCkptHdl      = NULL;
    CkptHdlDbT                   *pHdlInfo      = NULL;
    ClCkptSvcHdlT                svcHdl         = CL_CKPT_INVALID_HDL;
    CkptInitInfoT                *pInitInfo     = NULL;
    ClPtrT                       pCookie        = NULL;

    /*
     * Get all the handles associated with the checkpoint for which the
     * notification is received. 
     */
    clCksm32bitCompute ((ClUint8T *)pName->value, pName->length, &cksum);
    rc = clCntNodeFind(gClntInfo.ckptHdlList, (ClPtrT)(ClWordT)cksum, &nodeHdl);
    if( CL_OK != rc )
    {
        clLogError(CL_CKPT_AREA_CLIENT, CL_CKPT_CTX_NOTIFICATION,
                "Not able find checkpoint [%.*s] information for cksum"
                "[%d] rc [0x %x]", pName->length, pName->value, cksum, rc);
        return rc;
    }
    tempsum = cksum;

    /*
     * For a given checkpoint, there can be multiple opens.
     * So iterate through all possible handles.
     */
    while((nodeHdl != 0) && (tempsum == cksum))
    {
        ClCntKeyHandleT key = 0;

        rc = clCntNodeUserDataGet(gClntInfo.ckptHdlList, nodeHdl,
                (ClCntDataHandleT *) &pCkptHdl);
        if( CL_OK != rc )
        {
            clLogError(CL_CKPT_AREA_CLIENT, CL_CKPT_CTX_NOTIFICATION,
                    "Not able find checkpoint [%.*s] information rc [0x %x]",
                    pName->length, pName->value, rc);
            return rc;
        }
        rc = clCntNodeUserKeyGet(gClntInfo.ckptHdlList,nodeHdl,&key);
        if( CL_OK != rc )
        {
            clLogError(CL_CKPT_AREA_CLIENT, CL_CKPT_CTX_NOTIFICATION,
                    "Not able find checkpoint [%.*s] information rc [0x %x]",
                    pName->length, pName->value, rc);
            return rc;
        }
        tempsum = (ClUint32T)(ClWordT) key;

        /* 
         * Checkout the data associated with the checkpoint handle. 
         */
        rc = ckptHandleCheckout(*pCkptHdl, CL_CKPT_CHECKPOINT_HDL, 
                (void **)&pHdlInfo);
        if( CL_OK != rc )
        {
            clLogError(CL_CKPT_AREA_CLIENT, CL_CKPT_CTX_NOTIFICATION, 
                       "Checkpoint handle [%#llX] checkout failed",
                       *pCkptHdl);
            return rc;
        }

        /*
         * Get the svc handle associated with the ckpt handle and lock the
         * mutex associated with that service handle.
         */
        svcHdl = pHdlInfo->ckptSvcHdl;

        /* 
         * Checkout the data associated with the service handle. 
         */
        rc = ckptHandleCheckout(svcHdl, CL_CKPT_SERVICE_HDL, 
                (void **)&pInitInfo);
        if(rc != CL_OK)
        {
            clHandleCheckin(gClntInfo.ckptDbHdl, *pCkptHdl);
            return rc;
        }


        /*
         * Immediate consumption related event. Call the registered
         * callback.
         */
        notifyCallback = pHdlInfo->notificationCallback;
        pCookie        = pHdlInfo->pCookie;
        /* 
         * Checkin the data associated with the checkpoint handle. 
         */
        clHandleCheckin(gClntInfo.ckptDbHdl, *pCkptHdl);

        /* 
         * Checkin the data associated with the service handle 
         */
        clHandleCheckin(gClntInfo.ckptDbHdl, svcHdl);

        if( notifyCallback != NULL 
            &&
            pHdlInfo->ckptName.length == pName->length
            &&
            !strncmp((const ClCharT*)pHdlInfo->ckptName.value, 
                     (const ClCharT*)pName->value, pName->length))
        {
            notifyCallback(*pCkptHdl, pName, pIoVector, numSections, 
                           pCookie);
        }
        rc = clCntNextNodeGet(gClntInfo.ckptHdlList,nodeHdl,&nodeHdl);
        if( CL_OK != rc )
        {
            break;
        }
    }

    return CL_OK;
}

/*
 *  Helps detect pending callbacks.
 */
 
ClRcT clCkptSelectionObjectGet(ClCkptSvcHdlT       ckptSvcHdl,
                               ClSelectionObjectT  *pSelectionObj)
{
    ClRcT          rc         = CL_OK;
    CkptInitInfoT  *pInitInfo = NULL;
    ClInt32T       fileDes[2] = {0};

    /*
     * Check whether the client library is initialized or not.
     */
    if(gClntInfo.ckptDbHdl == 0)
    {
        CKPT_DEBUG_E(("CKPT Client is not yet initialized"));
        return CL_CKPT_ERR_NOT_INITIALIZED;
    }

    /*
     * Validate input parameter.
     */
    if(pSelectionObj == NULL) return CL_CKPT_ERR_NULL_POINTER;

    /*
     * Checkout the data associated with the service handle.
     */
    rc = ckptHandleCheckout(ckptSvcHdl, CL_CKPT_SERVICE_HDL, 
            (void **)&pInitInfo);
    if(rc != CL_OK)
    {
        CKPT_DEBUG_E(("Passed handle is invalid\n"));
        return CL_CKPT_ERR_INVALID_HANDLE;
    }

    /*
     * Lock the selection obj related mutex.
     */
    clOsalMutexLock(pInitInfo->ckptSelObjMutex);
    
    /*
     * If alreay called, return the associated fds.
     */
    if(pInitInfo->cbFlag == CL_TRUE)
    {
        *pSelectionObj = pInitInfo->readFd;

        /*
         * Unlock the selection obj related mutex.
         */
        clOsalMutexUnlock(pInitInfo->ckptSelObjMutex);

        /* 
         * Checkin the data associated with the service handle 
         */
        clHandleCheckin(gClntInfo.ckptDbHdl, ckptSvcHdl);
        return CL_OK;
    }   
    
    /*
     * Create Two fds.
     */
    if( pipe(fileDes) < 0 ) rc = CL_CKPT_ERR_NO_RESOURCE; 
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR,
            ("Ckpt:Error during getting Descriptors rc[0x %x]\n",rc),
            rc);
    pInitInfo->readFd   = fileDes[0];
    pInitInfo->writeFd  = fileDes[1];
    pInitInfo->cbFlag   = CL_TRUE;
    
    /*
     * Create Queue to have Callbacks.
     */
    rc = clQueueCreate(0, ckptQueueCallback,
            ckptQueueDestroyCallback,
            &pInitInfo->cbQueue);
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR,
            ("Ckpt:Error Creating Queue for callbacks rc[0x %x]\n",rc),
            rc);
    *pSelectionObj = pInitInfo->readFd;
exitOnError:
    {
        /*
         * Unlock the selection obj related mutex.
         */
        clOsalMutexUnlock(pInitInfo->ckptSelObjMutex);

        /* 
         * Checkin the data associated with the service handle 
         */
        clHandleCheckin(gClntInfo.ckptDbHdl, ckptSvcHdl);
        return rc; 
    }
}



/*
 *  Invokes the pending callbacks.
 */
 
ClRcT clCkptDispatch(ClCkptSvcHdlT     ckptSvcHdl,
                     ClDispatchFlagsT  dispatchFlag)
{
    CkptInitInfoT     *pInitInfo  = NULL;
    ClRcT             rc          = CL_OK;
    CkptOpenCbInfoT   *pOpenInfo  = NULL;
    CkptSyncCbInfoT   *pSyncInfo  = NULL;
    CkptQueueInfoT    *pQueueInfo = NULL;
    ClUint32T         size        = 0;
    ClCharT           chr         = 'c';

    /*
     * Check whether the client library is initialized or not.
     */
    if(gClntInfo.ckptDbHdl == 0)
    {
        CKPT_DEBUG_E(("CKPT Client is not yet initialized"));
        return CL_CKPT_ERR_NOT_INITIALIZED;
    }
    
    /*
     * Checkout the data associated with the service handle.
     */
    rc = ckptHandleCheckout(ckptSvcHdl, CL_CKPT_SERVICE_HDL, 
                            (void **)&pInitInfo);
    if(rc != CL_OK)
    {
      CKPT_DEBUG_E(("\nPassed handle is invalid\n"));
      return CL_CKPT_ERR_INVALID_HANDLE;
    }
    
    /*
     * Lock the selection obj related mutex.
     */
    clOsalMutexLock(pInitInfo->ckptSelObjMutex);
    
    /*
     * Return ERROR if selection object get not called.
     */
    if(pInitInfo->cbFlag != CL_TRUE) rc = CL_CKPT_ERR_OP_NOT_PERMITTED;
    CKPT_ERR_CHECK(CL_CKPT_LIB,CL_DEBUG_ERROR,
     ("Ckpt:Error not allowed to dispatch rc[0x %x]\n",rc),
      rc);
      
    /*
     * Depending upon the dispatch flag, take appropriate actions.
     */
    switch(dispatchFlag) 
    {
        case CL_DISPATCH_ONE:
            {
                /*
                 * Call the first callback in teh queue.
                 */
                rc = clQueueNodeDelete(pInitInfo->cbQueue,
                        (ClQueueDataT *)&pQueueInfo);
                if(pQueueInfo != NULL)
                {
                    if(read(pInitInfo->readFd, (void *)&chr, 1) != 1)
                    {
                        clLogError("CKP", "DIS", "Read from dispatch pipe returned [%s]",
                                   strerror(errno));
                    }
                    
                    /*
                     * Depending upon callback id, call either the
                     * open or synchronize callback function.
                     */
                    if(pQueueInfo->callbackId == CL_CKPT_OPEN_CALLBACK)
                    {
                        pOpenInfo = (CkptOpenCbInfoT *)pQueueInfo->pData;
                        pInitInfo->pCallback->checkpointOpenCallback(
                                pOpenInfo->invocation,
                                pOpenInfo->ckptHdl,
                                pOpenInfo->error);
                    }
                    else
                    {
                        pSyncInfo = (CkptSyncCbInfoT *)pQueueInfo->pData;
                        pInitInfo->pCallback->checkpointSynchronizeCallback(
                                pSyncInfo->invocation,
                                pSyncInfo->error);
                    }
                }	      
                break;     
            }    
        case CL_DISPATCH_ALL:
            {
                /*
                 * Sequentially call all the pending callback functions
                 * stored in the queue.
                 */
                rc = clQueueSizeGet(pInitInfo->cbQueue,&size);
                while( size > 0)
                {
                    rc = clQueueNodeDelete(pInitInfo->cbQueue,
                            (ClQueueDataT *)&pQueueInfo);

                    if(pQueueInfo != NULL)
                    {
                        if(read(pInitInfo->readFd, (void *)&chr, 1) != 1)
                        {
                           clLogError("CKP", "DIS", "Read from dispatch pipe returned [%s]",
                                      strerror(errno));
                        }
                        
                        /*
                         * Depending upon callback id, call either the
                         * open or synchronize callback function.
                         */
                        if(pQueueInfo->callbackId == CL_CKPT_OPEN_CALLBACK)
                        {
                            pOpenInfo = (CkptOpenCbInfoT *)pQueueInfo->pData;
                            pInitInfo->pCallback->checkpointOpenCallback(
                                    pOpenInfo->invocation,
                                    pOpenInfo->ckptHdl,
                                    pOpenInfo->error);
                        }
                        else
                        {
                            pSyncInfo = (CkptSyncCbInfoT *)pQueueInfo->pData;
                            pInitInfo->pCallback->
                              checkpointSynchronizeCallback(
                                    pSyncInfo->invocation,
                                    pSyncInfo->error);
                        }
                    }
                    size--;	      
                } 
                break;     
            }  
        case CL_DISPATCH_BLOCKING:
            {
                /*
                 * Block for the callbacks.
                 */
                while(1)
                {
                    if(read(pInitInfo->readFd, (void *)&chr, 1) != 1)
                        clLogError("CKP", "DISPATCH", "Read from dispatch pipe returned [%s]",
                                   strerror(errno));
                    rc = clQueueNodeDelete(pInitInfo->cbQueue,
                            (ClQueueDataT *)&pQueueInfo);

                    if(pQueueInfo != NULL)
                    {
                        /*
                         * Depending upon callback id, call either the
                         * open or synchronize callback function.
                         */
                        if(pQueueInfo->callbackId == CL_CKPT_OPEN_CALLBACK)
                        {
                            pOpenInfo = (CkptOpenCbInfoT *)pQueueInfo->pData;
                            pInitInfo->pCallback->checkpointOpenCallback(
                                    pOpenInfo->invocation,
                                    pOpenInfo->ckptHdl,
                                    pOpenInfo->error);
                        }
                        else
                        {
                            pSyncInfo = (CkptSyncCbInfoT *)pQueueInfo->pData;
                            pInitInfo->pCallback->
                            checkpointSynchronizeCallback(
                                    pSyncInfo->invocation,
                                    pSyncInfo->error);
                        }
                    }
                } 
            }
            break;     
        default: /*RETURN error*/ 
            {  
                CKPT_DEBUG_E(("Invalid dispatch flag passed"));
                rc = CL_CKPT_ERR_INVALID_PARAMETER;
                break;
            }   
    }	
exitOnError:
   {
        /*
         * Unlock the selection obj related mutex.
         */
        clOsalMutexUnlock(pInitInfo->ckptSelObjMutex);
    
        /* 
         * Checkin the data associated with the service handle 
         */
        clHandleCheckin(gClntInfo.ckptDbHdl, ckptSvcHdl);
        return rc;  
   }
}

/*
 * Allow 1 callback per application. to allow/disallow replica change events.
 */
ClRcT clCkptReplicaChangeRegister(ClRcT (*pCkptReplicaChangeCallback)
                                  (const ClNameT *pCkptName, ClIocNodeAddressT replicaAddr))
{
    if(gClCkptReplicaChangeCallback)
        return CKPT_RC(CL_ERR_INITIALIZED);
    if(!pCkptReplicaChangeCallback)
        return CKPT_RC(CL_ERR_INVALID_PARAMETER);
    gClCkptReplicaChangeCallback = pCkptReplicaChangeCallback;
    return CL_OK;
}

ClRcT clCkptReplicaChangeDeregister(void)
{
    gClCkptReplicaChangeCallback = NULL;
    return CL_OK;
}

ClBoolT clCkptDifferentialCheckpointStatusGet(void)
{
    if(gClDifferentialCkpt == -1)
    {
        gClDifferentialCkpt = clParseEnvBoolean("CL_ASP_DIFFERENTIAL_CKPT") ? 1 : 0;
    }
    return gClDifferentialCkpt > 0 ? CL_TRUE : CL_FALSE;
}

