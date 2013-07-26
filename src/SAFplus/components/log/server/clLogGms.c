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
#include <clLogGms.h>
#include <clLogCommon.h>
#include <clCpmApi.h>
#include <clLogSvrCommon.h>
#include <clLogDebug.h>

#include <clLogSvrEo.h>
#include <clLogStreamOwner.h>
#include <clLogStreamOwnerCkpt.h>
#include <clLogMasterCkpt.h>
#include <clLogMaster.h>
#include <clLogFileOwnerDeputy.h>
#include <clIocLogicalAddresses.h>
#include <clCpmExtApi.h>

static ClHandleT gIocCallbackHandle = CL_HANDLE_INVALID_VALUE;
static void 
clLogTrackCallback(ClGmsClusterNotificationBufferT *notificationBuffer,
                   ClUint32T                       numberOfMembers,
                   ClRcT                           rc);
extern ClRcT
clLogMasterEntryTLUpdate(ClLogSvrEoDataT        *pSvrEoEntry, 
                         ClLogSvrCommonEoDataT  *pSvrCommonEoData,
                         ClUint32T              haState);

static ClRcT
clLogAddrUpdate(ClIocNodeAddressT  leader, 
                ClIocNodeAddressT  deputy);

ClGmsHandleT  hGms = CL_HANDLE_INVALID_VALUE;

ClGmsCallbacksT logGmsCallbacks = {
                               NULL,
                               (ClGmsClusterTrackCallbackT) clLogTrackCallback,
                               NULL,
                               NULL,
                               };
static void
clLogIocNodedownCallback(ClIocNotificationIdT eventId, 
                         ClPtrT               pArg,
                         ClIocAddressT        *pAddress)
{
    ClRcT                  rc                = CL_OK;
    ClRcT                  ret               = CL_OK;
    ClLogSvrCommonEoDataT  *pSvrCommonEoData = NULL;
    ClIocNodeAddressT      tmpMasterAddr     = 0;

    rc = clLogSvrEoEntryGet(NULL, &pSvrCommonEoData);
    if( CL_OK != rc )
    {
        clLogError("SVR", "IOC", "Failed to get svr common data rc[0x %x]",
                rc);
        return ;
    }    
    if( (eventId == CL_IOC_NODE_LEAVE_NOTIFICATION 
         ||
         eventId == CL_IOC_NODE_LINK_DOWN_NOTIFICATION)
        &&
        pAddress->iocPhyAddress.nodeAddress == pSvrCommonEoData->masterAddr)
    {
        /*
         * this particular event callback will be called when the current
         * master is shutting down or killed. 
         */
        clLogNotice("SVR", "IOC", 
                    "IOC callback invoked for node leave of [%#x]",
                    pSvrCommonEoData->masterAddr);

        /* The current deputy node will be selected as master
         * while waiting for the GMS elects new master/deputy.
         */
        if (pSvrCommonEoData->deputyAddr != CL_IOC_RESERVED_ADDRESS 
                && pSvrCommonEoData->deputyAddr != -1)
            rc = clLogAddrUpdate(pSvrCommonEoData->deputyAddr, -1);
        else
        {
            ClTimerTimeOutT delay = {.tsSec = 0, .tsMilliSec = 50 };
            ret = clCpmMasterAddressGetExtended(&tmpMasterAddr, 3, &delay);
            if (ret == CL_OK)
            {
                rc = clLogAddrUpdate(tmpMasterAddr, -1);
            }
        }
    }
    else if( (eventId == CL_IOC_NODE_LEAVE_NOTIFICATION 
              ||
              eventId == CL_IOC_NODE_LINK_DOWN_NOTIFICATION)
             &&
             pAddress->iocPhyAddress.nodeAddress == pSvrCommonEoData->deputyAddr)
    {
        pSvrCommonEoData->deputyAddr = CL_IOC_RESERVED_ADDRESS;
    }
    return ;
}
static
ClRcT
clLogCpmNotificationCallbacksUpdate(ClIocNodeAddressT leader)
{
    ClRcT                 rc       = CL_OK;
    ClIocPhysicalAddressT compAddr = {0};

    /* deregister the old adress */
    if(CL_HANDLE_INVALID_VALUE != gIocCallbackHandle)
    {
        clCpmNotificationCallbackUninstall(&gIocCallbackHandle);
    }
    compAddr.nodeAddress = leader; 
    compAddr.portId      = CL_IOC_LOG_PORT;
    /* register for new address */
    if( CL_HANDLE_INVALID_VALUE != gIocCallbackHandle )
    {
        clCpmNotificationCallbackInstall(compAddr, clLogIocNodedownCallback, 
                NULL, &gIocCallbackHandle);
    }
    return rc;
}

static ClRcT
clLogAddrUpdate(ClIocNodeAddressT  leader, 
                ClIocNodeAddressT  deputy)
{
    ClRcT                  rc                = CL_OK;
    ClLogSvrCommonEoDataT  *pSvrCommonEoData = NULL;
    ClLogSvrEoDataT        *pSvrEoEntry      = NULL;
    ClIocNodeAddressT      localAddr         = CL_IOC_RESERVED_ADDRESS;

    CL_LOG_DEBUG_TRACE(("Enter:"));

    localAddr = clIocLocalAddressGet();
    if( CL_IOC_RESERVED_ADDRESS == localAddr )
    {
        CL_LOG_DEBUG_ERROR(("clIocLocalAddressGet() failed"));
        return CL_LOG_RC(CL_ERR_TRY_AGAIN);
    }
    
    rc = clLogSvrEoEntryGet(&pSvrEoEntry, &pSvrCommonEoData);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clLogSvrEoEntryGet(): rc[0x %x]", rc));
        return rc;
    }    
    
    if( ((CL_IOC_RESERVED_ADDRESS == pSvrCommonEoData->masterAddr) && (localAddr == leader)) || 
        ((CL_IOC_RESERVED_ADDRESS == pSvrCommonEoData->deputyAddr) && (localAddr == deputy)) ||
        (clCpmIsSCCapable() && (localAddr != leader) && (localAddr != deputy)) )
    {
        pSvrCommonEoData->masterAddr = leader;
        pSvrCommonEoData->deputyAddr = deputy;

        rc = clLogStreamOwnerGlobalBootup();
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clLogMasterInit(): rc[0x %x]", rc));
            return rc;
        }    
        rc = clLogMasterBootup();
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clLogMasterInit(): rc[0x %x]", rc));
        }
        if( localAddr == leader )
        {
            rc = clLogMasterEntryTLUpdate(pSvrEoEntry, pSvrCommonEoData, CL_IOC_TL_ACTIVE);
        }

        /*
         * Update the callbacks for leader change 
         */
        clLogCpmNotificationCallbacksUpdate(leader);

        CL_LOG_DEBUG_TRACE(("Master: %d Deputy: %d ",
                    pSvrCommonEoData->masterAddr, pSvrCommonEoData->deputyAddr));
        return rc;
    }

    if( (pSvrCommonEoData->masterAddr != leader) && (localAddr == leader) )/*switch over*/
    {
        pSvrCommonEoData->masterAddr = leader;
        pSvrCommonEoData->deputyAddr = deputy;
        /*
         * Deputy becomes master
         */
        rc = clLogMasterEntryTLUpdate(pSvrEoEntry, pSvrCommonEoData,
                                      CL_IOC_TL_ACTIVE);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("failed to update the TL entry"));
            return rc;
        }
        rc = clLogStreamOwnerGlobalStateRecover(leader, CL_TRUE);
        if( CL_OK != rc )
        {
            return rc;
        }    
        rc = clLogMasterGlobalCkptRead(pSvrCommonEoData, CL_TRUE);
        if( CL_OK != rc )
        {
            /*
             * FIXME - We need to destroy the global SO entries
             */
            return rc;
        }
        rc = clLogFileOwnerMasterStateRecover();
        if( CL_OK != rc )
        {
            /*
             * FIXME - We need to revert the destroy the master and global SO
             * entries
             */
            return rc;
        }
        /*
         * Update the callbacks for leader change 
         */
        clLogCpmNotificationCallbacksUpdate(leader);
        return rc;
    }
    if( (localAddr == pSvrCommonEoData->masterAddr) && (localAddr != leader) ) 
             
    {
        clLogMasterEntryTLUpdate(pSvrEoEntry, pSvrCommonEoData, 2);
    }
    
    /*
     * If the master address is getting changed update the callbacks 
     */
    if( pSvrCommonEoData->masterAddr != leader )
    {
        clLogCpmNotificationCallbacksUpdate(leader);
    }
    pSvrCommonEoData->masterAddr = leader;
    pSvrCommonEoData->deputyAddr = deputy;
    CL_LOG_DEBUG_TRACE(("Master: %d Deputy: %d ", pSvrCommonEoData->masterAddr,
                        pSvrCommonEoData->deputyAddr));

    CL_LOG_DEBUG_TRACE(("Exit"));
    return rc;
}

static 
void clLogTrackCallback(ClGmsClusterNotificationBufferT *notificationBuffer,
                        ClUint32T                       numberOfMembers,
                        ClRcT                           rc)
{
    clLogAddrUpdate(notificationBuffer->leader, notificationBuffer->deputy);
}

ClRcT
clLogGmsInit(void)
{   
    ClRcT                            rc         = CL_OK;
    ClGmsClusterNotificationBufferT  notBuffer  = {0};
    ClVersionT                       version    = {'B', 0x1, 0x1};
    ClUint32T                        numRetries = 0;

    CL_LOG_DEBUG_TRACE(("Enter"));
    
    do
    {
        rc = clGmsInitialize(&hGms, &logGmsCallbacks,
                             &version);
        if( CL_OK != rc )
        {
            usleep(100);
        }
        numRetries++;
    }while((rc != CL_OK) && 
           (CL_GET_ERROR_CODE(rc) == CL_ERR_TRY_AGAIN) && 
           (numRetries < CL_LOG_MAX_RETRIES));

    if( rc != CL_OK )
    {
        CL_LOG_DEBUG_ERROR(("clGmsInitialize(): rc[0x %x]\n", rc)); 
        return rc;
    }
    rc = clGmsClusterTrack( hGms,
                            CL_GMS_TRACK_CHANGES | CL_GMS_TRACK_CURRENT, 
                            &notBuffer);
    if (CL_OK != rc)
    {
        clGmsFinalize(hGms);
        CL_LOG_DEBUG_ERROR(("clGmsClusterTrack(): rc[0x %x]\n", rc));
        return rc;
    }
    rc = clLogAddrUpdate(notBuffer.leader, notBuffer.deputy);
    if (CL_OK != rc)
    {
        CL_LOG_DEBUG_ERROR(("clLogMasterAddrUpdate(): rc[0x %x]\n", rc));
    }

    CL_LOG_DEBUG_TRACE(("Exit: rc[0x %x]\n", rc));
    return rc;
}

ClRcT
clLogGmsFinalize(void)
{
    ClRcT  rc = CL_OK;

    CL_LOG_DEBUG_TRACE(("Enter"));
    if( CL_OK != (rc = clGmsFinalize(hGms)) )
    {
        CL_LOG_DEBUG_ERROR(("clGmsFinalize(): rc[0x %x]", rc));
    }    

    CL_LOG_DEBUG_TRACE(("Exit: rc[0x %x]\n", rc));
    return rc;
}    

ClRcT
clLogMasterEntryTLUpdate(ClLogSvrEoDataT        *pSvrEoEntry, 
                         ClLogSvrCommonEoDataT  *pSvrCommonEoData,
                         ClUint32T              haState)
{
    ClRcT         rc         = CL_OK;
    ClNameT       logSvrName = {0};
    ClIocTLInfoT  tlInfo     = {0};

    CL_LOG_DEBUG_TRACE(("Enter"));

    if( 0 != pSvrEoEntry->logCompId )
    {
        rc = clIocTransparencyDeregister(pSvrEoEntry->logCompId);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clIocTransparencyDeregister():"
                        "rc[0x %x]", rc));
        }
        if( 2 == haState )
        {
            /*
             * Prev master just deregistering the entry
             */
            pSvrEoEntry->logCompId = 0;
            return CL_OK;
        }
    }
    else
    {
        rc = clCpmComponentNameGet(pSvrEoEntry->hCpm, &logSvrName);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clCpmComponentNameGet(): rc[0x %x]",
                        rc));
            return rc;
        }
        rc = clCpmComponentIdGet(pSvrEoEntry->hCpm, &logSvrName, 
                &pSvrEoEntry->logCompId);
        if( CL_OK != rc )
        {
            CL_LOG_DEBUG_ERROR(("clCpmComponentIdGet(): rc[0x %x]", rc));
            return rc;
        }
    }

    tlInfo.logicalAddr = CL_IOC_LOG_LOGICAL_ADDRESS;

    tlInfo.compId                   = pSvrEoEntry->logCompId;
    tlInfo.contextType              = CL_IOC_TL_GLOBAL_SCOPE;
    tlInfo.physicalAddr.nodeAddress = clIocLocalAddressGet(); 
    tlInfo.physicalAddr.portId      = CL_IOC_LOG_PORT;
    tlInfo.haState                  = haState;
    rc = clIocTransparencyRegister(&tlInfo);
    if( CL_OK != rc )
    {
        CL_LOG_DEBUG_ERROR(("clIocTransparencyRegister(): rc[0x %x]",
                    rc));

    }

    CL_LOG_DEBUG_TRACE(("Exit: rc"));
    return rc;
}
