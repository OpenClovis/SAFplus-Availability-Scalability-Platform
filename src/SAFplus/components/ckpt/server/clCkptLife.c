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
 * File        : clCkptLife.c
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
*
*   This file contains Checkpoint service life cycle related functions
*
*
*****************************************************************************/
#include <clCommon.h>
#include <clCkptSvr.h>
#include "clCkptSvrIpi.h"
#include <clCkptUtils.h>
#include <clEoApi.h>
#include "clCkptMaster.h"
#include "unistd.h"
#include "ckptEoServer.h"
#include <clTimerApi.h>
#include <ckptEoServer.h>
#include <ckptEoClient.h>
#include <ckptClntEoClient.h>
#include <clCpmExtApi.h>
#include <clIocIpi.h>


CkptSvrCbT  *gCkptSvr = NULL;
static ClHandleT  gIocCallbackHandle = CL_HANDLE_INVALID_VALUE;
static void
clCkptIocNodedownCallback(ClIocNotificationIdT eventId, 
                          ClPtrT               pArg,
                          ClIocAddressT        *pAddress);

/* 
 * Function for initializing the Checkpointing server.
 */
 
ClRcT clCkptSvrInitialize(void)
{
    ClRcT             rc         = CL_OK;
    /* Variable related ckptDataBackup feature thats not supported
       ClUint8T          ckptRead   = 0;
    */
    ClTimerTimeOutT   timeOut    = {0}; 
    ClIocNodeAddressT deputy = 0;
    ClIocNodeAddressT master = 0;
    ClBoolT           addressUpdate = CL_FALSE;
    SaNameT           appName    = {0};   
 
    /*
     * Allocate the memory for server control block.
     */
    rc = ckptSvrCbAlloc(&gCkptSvr);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR, 
                   ("Checkpoint service CB create failed rc[0x %x]\n", rc), rc); 

    /*
     * Mark server as not ready.
     */
    gCkptSvr->serverUp        = CL_FALSE;
    gCkptSvr->condVarWaiting  = CL_TRUE;    

    /*
     * Create condition variable that indicate the receipt
     * of master and deputy ckpt addresses from gms.
     */
    clOsalCondCreate(&gCkptSvr->condVar);
    clOsalMutexCreate(&gCkptSvr->mutexVar);
    
    gCkptSvr->masterInfo.masterAddr = CL_CKPT_UNINIT_ADDR;
    gCkptSvr->masterInfo.deputyAddr = CL_CKPT_UNINIT_ADDR;
    gCkptSvr->masterInfo.compId     = CL_CKPT_UNINIT_VALUE; 
    gCkptSvr->masterInfo.clientHdlCount = 0;
    gCkptSvr->masterInfo.masterHdlCount = 0; 

    /*
     * Initialize gms to get the master and deputy addresses.
     */
    clOsalMutexLock(&gCkptSvr->ckptClusterSem);
    rc = clCkptMasterAddressesSet();
    master = gCkptSvr->masterInfo.masterAddr;
    deputy = gCkptSvr->masterInfo.deputyAddr;
    clOsalMutexUnlock(&gCkptSvr->ckptClusterSem);

    rc = clEoMyEoObjectGet(&gCkptSvr->eoHdl);
    rc = clEoMyEoIocPortGet(&gCkptSvr->eoPort);
   
    /*
     * Install the ckpt native table.
     */
    rc = clCkptEoClientInstall();
    CL_ASSERT(rc == CL_OK);
    
    rc = clCkptEoClientTableRegister(CL_IOC_CKPT_PORT);
    CL_ASSERT(rc == CL_OK);

    /*
     * Initialize the event client library.
     */
    ckptEventSvcInitialize();
    
    /* 
     * Initialize ckpt lib for persistent memory.
     * Obtain ckptRead flag that will tell whether ckpt server needs to 
     * read data from persistent memory or not.
     */

    /*
      Feature not yet supported see bug 6017 -- Do not forget to uncomment ckptDataBackupFinalize
      rc = ckptDataBackupInitialize(&ckptRead);
    */

    if(rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_WARNING, NULL,
                   CL_LOG_MESSAGE_2_LIBRARY_INIT_FAILED, "ckpt", rc);
    }

    if( gCkptSvr->localAddr == master )
    {
        /*
         * The node coming up is master. If deputy exists, syncup with deputy
         * else syncup with persistant memory (if existing).
         */
        rc = clCpmComponentNameGet(gCkptSvr->amfHdl, &appName);
        if(clCpmIsCompRestarted(appName))
        {
            if((deputy != CL_CKPT_UNINIT_ADDR) 
               &&
               (deputy != -1))
            {
                rc = ckptMasterDatabaseSyncup(deputy);
                /*
                 * If deputy server is not reachable then pull info from the 
                 * persistent memory.
                 */
                if(CL_RMD_TIMEOUT_UNREACHABLE_CHECK(rc))
                {
                    clLogNotice(CL_CKPT_AREA_ACTIVE, "DBS", 
                                "Database Syncup with ckpt master failed with RMD timeout error. rc 0x%x",rc);
                    /* This is dead code as the feature is not supported
                       if(ckptRead == 1)
                       {
                       rc = ckptPersistentMemoryRead();
                       }
                    */
                }
            }
        }
        /* This is dead code as the feature is not supported
           else
           {
           if(ckptRead == 1)
           {
           rc = ckptPersistentMemoryRead();
           }
           }
        */
    }
    
    /*TODO:This check is only for seleting the deputy or for what */
    clOsalMutexLock(gCkptSvr->mutexVar);    
    if( (clIocLocalAddressGet() != master) &&
        (gCkptSvr->condVarWaiting == CL_TRUE)     )
    {
        /*
         * Ensure that the node gets the master and deputy addresses before
         * proceeding further.
         */
        timeOut.tsSec            = 5;
        timeOut.tsMilliSec       = 0;
        clOsalCondWait(gCkptSvr->condVar, gCkptSvr->mutexVar, timeOut);
        gCkptSvr->condVarWaiting = CL_FALSE;
        addressUpdate = CL_TRUE;
    }
    clOsalMutexUnlock(gCkptSvr->mutexVar);    

    /*  
     * We double check again incase we had waited for the gms cluster track.
     * but with the right lock sequence below instead of shoving it in the
     * condwait above.
     */
    if(addressUpdate == CL_TRUE)
    {
        clOsalMutexLock(&gCkptSvr->ckptClusterSem);
        master = gCkptSvr->masterInfo.masterAddr;
        deputy = gCkptSvr->masterInfo.deputyAddr;
        clOsalMutexUnlock(&gCkptSvr->ckptClusterSem);
    }
    /* 
     * Announce the arrival to peer in the n/w. Master server announces to 
     * all and other servers announce to master server.
     */
    ckptSvrArrvlAnnounce();
    
    /*
     * If the node coming up is deputy, syncup the metadata and ckpt info 
     * from the master. Treat all SC capable nodes except the master as deputy.
     */
    if(  gCkptSvr->localAddr != master && clCpmIsSCCapable())
    {
        /*
         * Just freezing the deputy till syncup, not to receive any master
         * related calls, this is to ensure that deputy is in full synup with
         * master and any calls from master to update deputy will sleep on the
         * lock.
         */
        CKPT_LOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
        ckptMasterDatabaseSyncup(gCkptSvr->masterInfo.masterAddr);
        CKPT_UNLOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
    }
    
    /* 
     * Server is up.
     */
    gCkptSvr->serverUp = CL_TRUE;
    return rc;
    exitOnError:
    {
        return rc;
    }
}



/* This routine brings back a checkpoint server into life 
 *  Possible cases why this can be triggered are
 *      1. Restart for upgradation (not being addressed immediately)
 *      2. Restart as part of a repair action
 *  Restart as part of node reboot
*/

ClRcT  clCkptRestart()
{
    return CL_OK;
}



/* 
 * Routine to shutdown a CKPT server.
 */
 
ClRcT   ckptShutDown()
{
    ClOsalMutexIdT  ckptActiveSem = CL_HANDLE_INVALID_VALUE;
    /*
     * Uninstall the notification callback function
     */
    if(CL_HANDLE_INVALID_VALUE != gIocCallbackHandle)
    {
        clCpmNotificationCallbackUninstall(&gIocCallbackHandle);
    }
    /*
     * Inform peers about the departure.
     */
    ckptSvrDepartureAnnounce();

    /*
     * Finalize the event library.
     */
    ckptEventSvcFinalize();

    /*
     * Uninstall the native client table.
     */
    clEoClientUninstall (gCkptSvr->eoHdl, CL_EO_NATIVE_COMPONENT_TABLE_ID);

    /*
     * Finalize the gms library.
     */
    clGmsFinalize(gCkptSvr->gmsHdl);

    /*
     * Cleanup the resources acquired bu the ckpt server.
     */
    clOsalMutexLock(&gCkptSvr->ckptClusterSem);
    CKPT_LOCK(gCkptSvr->ckptActiveSem);
    /* the server is going down setting this flag as FALSE */
    gCkptSvr->serverUp = CL_FALSE;
    ckptActiveSem = gCkptSvr->ckptActiveSem;
    ckptSvrCbFree(gCkptSvr);
    CKPT_UNLOCK(ckptActiveSem);
    clOsalMutexUnlock(&gCkptSvr->ckptClusterSem);
    clOsalMutexDelete(ckptActiveSem);
    
    return CL_OK;
}

void
clCkptIocNodedownCallback(ClIocNotificationIdT eventId, 
                           ClPtrT               pArg,
                           ClIocAddressT        *pAddress)
{
    ClRcT             ret        = CL_OK;
    ClIocNodeAddressT masterAddr = 0;
    ClIocNodeAddressT deputyAddr = 0;
    ClIocNodeAddressT tmpMasterAddr = 0;

    if (gCkptSvr == NULL || gCkptSvr->serverUp == CL_FALSE) 
        return ; 
    /*
     * If master node is down, ask deputy to change the master address 
     */
    clOsalMutexLock(&gCkptSvr->ckptClusterSem);
    CKPT_LOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
    clLogDebug("ACT", "IOC", "Received ioc notification [%d] for node [%d], port [%d]. "
               "Current ckpt master is node [%d]",
               eventId, pAddress->iocPhyAddress.nodeAddress, pAddress->iocPhyAddress.portId,
               gCkptSvr->masterInfo.masterAddr);
    if( (eventId == CL_IOC_NODE_LEAVE_NOTIFICATION ||
         eventId == CL_IOC_NODE_LINK_DOWN_NOTIFICATION)
        &&
        pAddress->iocPhyAddress.nodeAddress == gCkptSvr->masterInfo.masterAddr)
    {
        /*
         * this particular event callback will be called when the current
         * master is shutting down or killed. 
         */
        masterAddr = gCkptSvr->masterInfo.masterAddr;
        deputyAddr = gCkptSvr->masterInfo.deputyAddr;
        clLogNotice(CL_CKPT_AREA_ACTIVE, "IOC", 
                    "IOC callback invoked for node leave of [%#x]. Current deputy [%#x]", 
                    masterAddr, deputyAddr);

        /* The current deputy node will be selected as master
         * while waiting for the GMS elects new master/deputy.
         */
        if (deputyAddr != CL_IOC_RESERVED_ADDRESS && deputyAddr != -1)
            clCkptMasterAddressUpdate(deputyAddr, -1);
        else
        {
            ClTimerTimeOutT delay = {.tsSec = 0, .tsMilliSec = 50 };
            ret = clCpmMasterAddressGetExtended(&tmpMasterAddr, 3, &delay);
            if (ret == CL_OK)
            {
                clCkptMasterAddressUpdate(tmpMasterAddr, -1);
            }
        }

        CKPT_UNLOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
        clOsalMutexUnlock(&gCkptSvr->ckptClusterSem);
        /*
         * Call master down notification to make appropriate changes to active
         * address 
         */ 
         ckptPeerDown(masterAddr, CL_CKPT_NODE_DOWN, 0);
         return;
    }
    else if(eventId == CL_IOC_NODE_LEAVE_NOTIFICATION  
            || 
            eventId == CL_IOC_NODE_LINK_DOWN_NOTIFICATION)
    {
        if(pAddress->iocPhyAddress.nodeAddress == gCkptSvr->masterInfo.deputyAddr)
        {
            gCkptSvr->masterInfo.deputyAddr = CL_IOC_RESERVED_ADDRESS;
            CKPT_UNLOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
            clOsalMutexUnlock(&gCkptSvr->ckptClusterSem);
        }
        else
        {
            CKPT_UNLOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
            clOsalMutexUnlock(&gCkptSvr->ckptClusterSem);
            ckptPeerDown(pAddress->iocPhyAddress.nodeAddress, CL_CKPT_NODE_DOWN, 0);
        }
        return;
    }

    CKPT_UNLOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
    clOsalMutexUnlock(&gCkptSvr->ckptClusterSem);

}

ClRcT
clCkptIocCallbackUpdate(void)
{
    ClRcT                 rc       = CL_OK;
    ClIocPhysicalAddressT compAddr = {0};

    /* deregister the old adress */
    if(CL_HANDLE_INVALID_VALUE != gIocCallbackHandle)
    {
        return rc;
    }
    compAddr.nodeAddress = CL_IOC_BROADCAST_ADDRESS;
    compAddr.portId      = CL_IOC_CKPT_PORT;
    /* register for new address */
    clCpmNotificationCallbackInstall(compAddr, clCkptIocNodedownCallback, 
                                       NULL, &gIocCallbackHandle);
    return rc;
}
