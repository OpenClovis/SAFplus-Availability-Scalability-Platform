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
 * ModuleName  : amf
 * File        : clAms.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This is the main AMS server side file.
 ***************************** Editor Commands ********************************
 * For vi/vim
 * :set shiftwidth=4
 * :set softtabstop=4
 * :set expandtab
 *****************************************************************************/
 
/******************************************************************************
 * Include files needed to compile this file
 *****************************************************************************/

#define __SERVER__
#include <clAms.h>
#include <clAmsPolicyEngine.h>
#include <clAmsParser.h>
#include <clAmsMgmtServerApi.h>
#include <clAmsSAServerApi.h>
#include <clLogApi.h>
#include <clAmsModify.h>
#include <clAmsServerUtils.h>
#include <clAmsDebugCli.h>
#include <clAmsEntityTrigger.h>
#include <clAmsCkpt.h>
#include <clCpmCommon.h>
#include <clAmsEventServerApi.h>
#include <clAmsEntityUserData.h>
#include <clCpmInternal.h>
#include <clList.h>
#include <clHandleIpi.h>

#define CONFIG_FILE_NAME        "clAmfConfig.xml"
#define DEFN_FILE_NAME          "clAmfDefinitions.xml"

/******************************************************************************
 * Global data structures
 *****************************************************************************/

/*
 * The global data structure for AMS
 */

ClAmsT  gAms;

ClBoolT gAmsDBRead = CL_FALSE;

ClCpmAmsToCpmCallT *gAmsToCpmCallbackFuncs = NULL;

ClCpmCpmToAmsCallT gCpmToAmsCallbackFuncs = {
    _clAmsSACSIHAStateGet,
    _clAmsSACSIQuiescingComplete,
    _clAmsSAComponentErrorReport,
    _clAmsSACSIOperationResponse,
    _clAmsSAComponentOperationResponse,
    _clAmsSANodeJoin,
    _clAmsSANodeLeave,
    _clAmsSAPGTrackAdd,
    _clAmsSAPGTrackStop,
    _clAmsSACkptServerReady,
    _clAmsSAAmsStateChange,
    _clAmsSAEventServerReady,
    _clAmsSANodeAdd,
    _clAmsSANodeRestart,
};

#undef __CLIENT__
#include "clAmsMgmtServerFuncTable.h"

typedef struct ClAmsFaultQueue
{
    ClAmsEntityT entity;
    ClListHeadT  list;
}ClAmsFaultQueueT;

static CL_LIST_HEAD_DECLARE(gClAmsFaultQueue);
static ClOsalMutexT gClAmsFaultQueueLock;
static ClCharT gClAmfInstantiateCommand[CL_MAX_NAME_LENGTH];
/*
 * Function to install the Mgmt function table
 */

ClRcT
clAmsMgmtServerFuncInstall( void )
{

    AMS_CALL( clEoClientInstallTables(gAms.eoObject,
                                      CL_EO_SERVER_SYM_MOD(gAspFuncTable, AMFMgmtServer)));

    return CL_OK;
}

ClRcT
clAmsInitializeMgmtInterface(void)
{
    clAmsDebugCliInitialization();

#if defined (CL_AMS_MGMT_HOOKS)
    clAmsMgmtHookInitialize();
#endif
    clAmsEntityTriggerInitialize();

    return CL_OK;
}

static ClRcT amsClusterTimerCallback(void *arg)
{
    ClAmsSGFailoverHistoryKeyT *key = arg;
    ClAmsSGT *sg = NULL;
    ClAmsSGFailoverHistoryT *failoverHistory = NULL;
    ClRcT rc = CL_OK;

    clLogDebug("FAILOVER", "TIMER", "Failover history timer invoked for entity [%s], index [%d]",
               key->entity.name.value, key->index);

    clOsalMutexLock(gAms.mutex);
    rc = clAmsFailoverHistoryFind(&key->entity, key->index, &sg, &failoverHistory);
    if(rc != CL_OK)
    {
        clLogWarning("FAILOVER", "TIMER", "Unable to locate failover history for entity [%s], index [%d]",
                     key->entity.name.value, key->index);
        goto out_unlock;
    }
    /*
     * Delete this history
     */
    --sg->status.failoverHistoryCount;
    CL_ASSERT(sg->status.failoverHistoryCount >= 0);
    clListDel(&failoverHistory->list);

    out_unlock:
    clOsalMutexUnlock(gAms.mutex);
    
    if(failoverHistory)
    {
        if(failoverHistory->timer)
            clTimerDelete(&failoverHistory->timer);
        clHeapFree(failoverHistory);
    }
    clHeapFree(key);

    return CL_OK;
}

ClRcT
clAmsInitialize(
                CL_IN       ClAmsT              *ams,
                CL_IN       ClCpmAmsToCpmCallT  *cpmFuncs,
                CL_IN       ClCpmCpmToAmsCallT  *amsFuncs)

{
    ClRcT   rc = CL_OK;

    AMS_CHECKPTR ( !ams || !cpmFuncs || !amsFuncs );

    /*
     * This must be upfront, so all debug messages are caught and filtered.
     */
    ams->isEnabled = CL_FALSE;
    ams->debugFlags = CL_AMS_DEBUG_FLAGS_BETACODE;
    ams->debugLogToConsole = clParseEnvBoolean("AMS_DEBUG");

    AMS_FUNC_ENTER (("\n"));

    /*
     * Set state of AMS to service unavailable.
     */

    ams->serviceState = CL_AMS_SERVICE_STATE_UNAVAILABLE;

    /*
     * Instantiate AMS. This call could alternately come from CPM with
     * appropriate instantiate mode, or mode should be passed as an
     * argument.
     */ 
    
    ams->cpmRecoveryQuiesced = CL_FALSE;
    ams->timerCount = 0;
    ams->serverepoch = 0;

    clOsalMutexInit(&gClAmsFaultQueueLock);

    /*
     * Initialize the handle database for CCB related operations
     */

    AMS_CALL (clHandleDatabaseCreate(NULL, &ams->ccbHandleDB));

    /*
     * Initialize the counters for tracking operations
     */

    ams->ops.currentOp = ams->ops.lastOp = 0;

    ams->invocationCount = 0;

    /*
     * Initialize log counter
     */

    ams->logCount = 0;

    AMS_CALL (clOsalMutexCreate(&ams->mutex));

    AMS_CALL (clOsalCondCreate(&ams->terminateCond));

    AMS_CALL (clOsalMutexCreate(&ams->terminateMutex));

    AMS_CALL (clOsalMutexInit(&ams->ckptMutex));

    clAmsEntityInitialize();

    clAmsEntityUserDataInitialize();

    /*
     * Initialize the AMS - CPM interface, which enables the SAF SA API.
     */ 

    gAmsToCpmCallbackFuncs = cpmFuncs;
    memcpy ( amsFuncs, &gCpmToAmsCallbackFuncs, sizeof (ClCpmCpmToAmsCallT)); 

    AMS_CALL (clEoMyEoObjectGet(&ams->eoObject));

    rc = clTimerClusterRegister(amsClusterTimerCallback, NULL);
    CL_ASSERT(rc == CL_OK);

    AMS_CALL (clAmsInvocationListInstantiate(&ams->invocationList));

    AMS_CALL( clAmsDbInstantiate(&ams->db) );

    ams->ckptServerReady = CL_FALSE;
    ams->eventServerReady = CL_FALSE;
    ams->eventServerInitialized = CL_FALSE;

    return CL_OK;
}

ClRcT
clAmsStart(
           CL_IN       ClAmsT              *ams,
           CL_IN       const ClUint32T     mode)

{
    ClRcT   rc = CL_OK;

    /*
     * Check if already in the mode.
     */
    if(ams->mode == mode)
    {
        clLogWarning("AMS", "INI", 
                     "AMS mode is already [%s]. Skipping initialization", 
                     mode == CL_AMS_INSTANTIATE_MODE_ACTIVE ? "active" : "standby");
        return rc; 
    }

    AMS_FUNC_ENTER (("\n"));

    AMS_LOG (CL_DEBUG_TRACE,
             ("Instantiating AMS, Mode = %s\n",
              CL_AMS_STRING_INSTANTIATE_MODE(mode)));

    /*
     * Instantiate AMS. This call could alternately come from CPM with
     * appropriate instantiate mode, or mode should be passed as an
     * argument.
     */ 
    ams->isEnabled = CL_TRUE;
    ams->mode = mode;
    ams->serverepoch = time(NULL); 

    AMS_OP_INCR(&gAms.ops);

    /*
     * Initialize the AMS - CPM interface, which enables the SAF SA API.
     */ 
    AMS_CALL (clAmsMgmtServerFuncInstall());

    if ( ( (mode) & (CL_AMS_INSTANTIATE_MODE_ACTIVE) ) )
    {
        ClIocNodeAddressT localAddress = clIocLocalAddressGet();
        ClIocNodeAddressT masterAddress = 0;
        ClInt32T retries = 0;
        ClTimerTimeOutT delay = {.tsSec = 2, .tsMilliSec = 0};

        rc = clCpmMasterAddressGet(&masterAddress);

        if (CL_OK != rc)
        {
            clLogCritical("AMS", "BOO",
                          "The cluster is in inconsistent state. "
                          "This node is the master, but master address get failed "
                          "with error [%#x]", rc);
            return rc;
        }

        /*
         * Perceived xport config inconsistency
         * This could either mean an invalid cluster config
         * or a transitioning phase where the last active is also shutting down
         * while we are starting up on this node and the topology bind for AMF
         * also exists on that standby. We retry before giving up since we are fine with the GMS    
         * view and the peer could be in the middle of exiting the cluster.
         */
        while(masterAddress != localAddress 
              && 
              retries++ < 10)
        {
            clOsalTaskDelay(delay);
            rc = clCpmMasterAddressGet(&masterAddress);
            if(rc != CL_OK)
            {
                clLogCritical("AMS", "BOO",
                              "The cluster is in inconsistent state. "
                              "This node is the master, but master address get failed "
                              "with error [%#x]", rc);
                return rc;
            }
        }

        if(masterAddress != localAddress)
        {
            ClTimerTimeOutT delay = {.tsSec = 5, .tsMilliSec = 0 };
            clLogCritical("AMS", "BOO",
                          "Inconsistency between GMS and IOC configuration detected, "
                          "master address as per GMS is [%#x], but master address "
                          "as per IOC is [%#x]",
                          localAddress, masterAddress);
            clLogCritical("AMS", "BOO", "This node would be restarted in [%d] secs", delay.tsSec);
            cpmRestart(&delay, "Controller");
            return CL_AMS_RC(CL_ERR_INVALID_STATE);
        }

        clAmsInitializeMgmtInterface();

        rc = CL_AMS_RC(CL_ERR_NOT_EXIST);

        if ( ( (mode) & (CL_AMS_INSTANTIATE_USE_CHECKPOINT) ))
        {
            /*
             * Read the persistent DB if present
             */
            clOsalMutexLock(ams->mutex);
            rc = clAmsCkptDBRead();
            clOsalMutexUnlock(ams->mutex);
        }

        if(rc != CL_OK)
        {
            AMS_LOG(CL_DEBUG_INFO, ("Loading AMS config from XML file\n"));
       
            parse_xml:
            rc = clAmsParserMain (DEFN_FILE_NAME,CONFIG_FILE_NAME);
            if (CL_OK != rc)
            {
                clLogError("AMS", "BOO", "AMS config parsing failed with [%#x]", rc);
                return rc;
            }
            AMS_CALL (clAmsValidateConfig(ams,CL_AMS_ENTITY_VALIDATE_ALL));
        }
        else
        {
            gAmsDBRead = CL_TRUE;
        }

        AMS_CALL ( clOsalMutexLock(ams->mutex) );

        if (( rc = clAmsPeClusterInstantiate(ams) )
            != CL_OK)
        {
            if(gAmsDBRead && CL_GET_ERROR_CODE(rc) == CL_ERR_INVALID_STATE)
            {
                clLogWarning("DB", "READ", "Cluster instantiate from AMS db failed during DB pre-processing phase."
                             "Now loading DB from XML file");
                gAmsDBRead = CL_FALSE;
                clAmsDbTerminate(&ams->db);
                if((rc = clAmsDbInstantiate(&ams->db)) != CL_OK)
                {
                    clOsalMutexUnlock(ams->mutex);
                    clLogError("DB", "READ", "DB instantiate failed with [%#x]", rc);
                    return rc;
                }
                clOsalMutexUnlock(ams->mutex);
                goto parse_xml;
            }
            AMS_CALL ( clOsalMutexUnlock(ams->mutex) );
            return CL_AMS_RC (rc);
        } 
        AMS_CALL ( clOsalMutexUnlock(ams->mutex) );
    }
    else if ( ( (mode) & (CL_AMS_INSTANTIATE_MODE_STANDBY) ) )
    {
        ams->serviceState = CL_AMS_SERVICE_STATE_UNAVAILABLE;
    }
    else
    {

        /*
         * Invalid instantiate mode 
         */ 

        AMS_LOG (CL_DEBUG_ERROR,
                 ("Instantiating AMS With Invalid Instantiate Mode \n"));

        return CL_AMS_RC (CL_AMS_ERR_INVALID_ARGS);

    }

    return CL_OK;
}


/*
 * clAmsFinalize
 * --------------
 * Finalizes/delete an AMS entity. This function stops the AMS service, all
 * entities under the purview of AMS are sent terminate signals and then
 * the AMS database is cleaned up. Typically, you would call this only if
 * you need to do a full cluster reset.
 *
 */

ClRcT
clAmsFinalize(
        CL_IN       ClAmsT      *ams,
        CL_IN       const ClUint32T   mode)
{
    ClRcT rc = CL_OK;
    
    AMS_OP_INCR(&gAms.ops);

    AMS_CHECKPTR ( !ams );

    AMS_LOG (CL_DEBUG_TRACE,
             ("Terminating AMS, Mode = %s\n",
              ( ( (mode) & (CL_AMS_TERMINATE_MODE_GRACEFUL) ) != CL_FALSE ) ? "Graceful" : "Fast"));

    /*
     * This is to take care of scenario when AMS finalize has been
     * called before AMS initialize is called
     */

    if ( gAms.isEnabled == CL_FALSE )
    {
        return CL_OK;
    }

    clAmsEntityTriggerFinalize();

    clAmsEntityUserDataFinalize();

    clAmsDebugCliFinalization();

#if defined (CL_AMS_MGMT_HOOKS)
    clAmsMgmtHookFinalize();
#endif

    /*
     * This condition will be executed in the scenario when only system 
     * controller node is shutting down or payload reset is disabled
     */

    if ( (mode & CL_AMS_TERMINATE_MODE_SC_ONLY) 
         ||
         gClAmsPayloadResetDisable )
            
    {
        goto exitfn;
    }

    AMS_CALL ( clOsalMutexLock(ams->mutex) );

    AMS_CHECK_RC_ERROR_AND_UNLOCK_MUTEX ( clAmsPeClusterTerminate(ams), ams->mutex ); 

    AMS_CALL ( clOsalMutexUnlock(ams->mutex) );

    if ( ( (mode) & (CL_AMS_TERMINATE_MODE_GRACEFUL) ) != CL_FALSE )
    {
        ClTimerTimeOutT timeout = {0};

        AMS_LOG(CL_DEBUG_TRACE,
                ("Waiting %dms for AMS termination to complete..\n",
                 CL_AMS_TERMINATE_TIMEOUT));

        CL_AMS_TIMER_CONVERT(CL_AMS_TERMINATE_TIMEOUT, timeout);
    
        if ( ams->serviceState != CL_AMS_SERVICE_STATE_STOPPED )
        {
            clOsalMutexLock(ams->terminateMutex);
            clOsalCondWait(
                        ams->terminateCond,
                        ams->terminateMutex,
                        timeout);
            clOsalMutexUnlock(ams->terminateMutex);
        }
    }

    AMS_CALL ( clAmsCkptFree(ams) );

    clEoClientUninstallTables(ams->eoObject,
                              CL_EO_SERVER_SYM_MOD(gAspFuncTable, AMFMgmtServer));

    /*
     * XXX: Should the event free also be done when only the SC only 
     * shutdown happens
     */

    if ( ams->eventServerInitialized == CL_TRUE )
    {
        clAmsNotificationEventFinalize();
    }

    AMS_CALL (clHandleDatabaseDestroy(ams->ccbHandleDB));
    ams->ccbHandleDB = CL_HANDLE_INVALID_VALUE;

exitfn:

    clOsalMutexLock(ams->mutex);

    ams->isEnabled = CL_FALSE;
    ams->serviceState = CL_AMS_SERVICE_STATE_UNAVAILABLE;
    ams->timerCount = 0;

    clOsalMutexUnlock(ams->mutex);

    clAmsDbTerminate(&ams->db);

    AMS_LOG(CL_DEBUG_INFO, ("AMS Termination Completed\n"));

    return rc;

}

static ClRcT 
clAmsFaultQueueFindNoLock(ClAmsEntityT *entity, void **faultEntry)
{
    ClListHeadT *iter = NULL;
    CL_LIST_FOR_EACH(iter, &gClAmsFaultQueue)
    {
        ClAmsFaultQueueT *entry = CL_LIST_ENTRY(iter, ClAmsFaultQueueT, list);
        if(!strncmp(entry->entity.name.value, entity->name.value, entry->entity.name.length))
        {
            if(faultEntry) *faultEntry = (void*)entry;
            return CL_OK;
        }
    }
    return CL_AMS_RC(CL_ERR_NOT_EXIST);
}

ClRcT 
clAmsFaultQueueFind(ClAmsEntityT *entity, void **entry)
{
    ClRcT rc = CL_OK;
    clOsalMutexLock(&gClAmsFaultQueueLock);
    rc = clAmsFaultQueueFindNoLock(entity, entry);
    clOsalMutexUnlock(&gClAmsFaultQueueLock);
    return rc;
}
/*
 * Add an entity to the fault queue. We have to reset it when
 * a repair happens.
 */
ClRcT
clAmsFaultQueueAdd(ClAmsEntityT *entity)
{
    ClAmsFaultQueueT *entry = NULL;
    clOsalMutexLock(&gClAmsFaultQueueLock);
    if(clAmsFaultQueueFindNoLock(entity, NULL) == CL_OK)
    {
        clOsalMutexUnlock(&gClAmsFaultQueueLock);
        return CL_OK;
    }
    entry = clHeapCalloc(1, sizeof(*entry));
    CL_ASSERT(entry != NULL);
    memcpy(&entry->entity, entity, sizeof(entry->entity));
    clListAddTail(&entry->list, &gClAmsFaultQueue);
    clOsalMutexUnlock(&gClAmsFaultQueueLock);
    clLogDebug("AMF", "FLT-ADD", "Entity [%s] added to the fault queue",
               entity->name.value);
    return CL_OK;
}

ClRcT
clAmsFaultQueueDelete(ClAmsEntityT *entity)
{
    ClRcT rc = CL_OK;
    ClAmsFaultQueueT *entry = NULL;
    clOsalMutexLock(&gClAmsFaultQueueLock);
    rc = clAmsFaultQueueFindNoLock(entity, (void**)&entry);
    if(rc != CL_OK)
    {
        clOsalMutexUnlock(&gClAmsFaultQueueLock);
        return rc;
    }
    clListDel(&entry->list);
    clOsalMutexUnlock(&gClAmsFaultQueueLock);
    clHeapFree(entry);
    clLogDebug("AMF", "FLT-DEL", "Entity [%s] deleted from fault queue",
               entity->name.value);
    return CL_OK;
}

ClRcT
clAmsFaultQueueDestroy(void)
{
    clOsalMutexLock(&gClAmsFaultQueueLock);
    while(!CL_LIST_HEAD_EMPTY(&gClAmsFaultQueue))
    {
        ClAmsFaultQueueT *entry = NULL;
        ClListHeadT *first = gClAmsFaultQueue.pNext;
        entry = CL_LIST_ENTRY(first, ClAmsFaultQueueT, list);
        clListDel(first);
        clHeapFree(entry);
    }
    clOsalMutexUnlock(&gClAmsFaultQueueLock);
    return CL_OK;
}

ClRcT clAmsCheckNodeJoinState(const ClCharT *pNodeName)
{
    ClRcT rc = CL_OK;
    ClAmsEntityRefT entityRef = {{0}};
    if(!pNodeName)
        return rc;
    clOsalMutexLock(gAms.mutex);
    if(!gAms.isEnabled 
       || 
       gAms.serviceState == CL_AMS_SERVICE_STATE_UNAVAILABLE)
    {
        clLogNotice("NODE", "JOIN", "Returning try again for node join as ams state is not up");
        rc = CL_AMS_RC(CL_ERR_TRY_AGAIN);
        goto out_unlock;
    } 
    clNameSet(&entityRef.entity.name, pNodeName);
    ++entityRef.entity.name.length;
    entityRef.entity.type = CL_AMS_ENTITY_TYPE_NODE;
    rc = clAmsEntityDbFindEntity(&gAms.db.entityDb[CL_AMS_ENTITY_TYPE_NODE], &entityRef);
    if(rc == CL_OK && entityRef.ptr)
    {
        ClAmsNodeT *node = (ClAmsNodeT*)entityRef.ptr;
        clLogDebug("NODE", "JOIN", "Got registration from node [%s] with presence state [%s] "
                   "recovery [%s], member [%s], wasMemberBefore [%s]", pNodeName,
                   CL_AMS_STRING_P_STATE(node->status.presenceState),
                   CL_AMS_STRING_RECOVERY(node->status.recovery),
                   CL_AMS_STRING_NODE_ISCLUSTERMEMBER(node->status.isClusterMember),
                   node->status.wasMemberBefore ? "Yes" : "No");
        if(node->status.isClusterMember == CL_AMS_NODE_IS_NOT_CLUSTER_MEMBER
           &&
           node->status.wasMemberBefore 
           &&
           node->status.presenceState == CL_AMS_PRESENCE_STATE_INSTANTIATED)
        {
            clLogNotice("NODE", "JOIN", "Returning try again as node [%s] is being failed over", pNodeName);
            rc = CL_AMS_RC(CL_ERR_TRY_AGAIN);
            goto out_unlock;
        }
        /*
         * Got a registration request from a node which is already a member of the cluster.
         */
        if(node->status.isClusterMember == CL_AMS_NODE_IS_CLUSTER_MEMBER)
        {
            /*  
             * Reset in case the node had split just at the time at which the node joined.
             */
            if(node->status.presenceState == CL_AMS_PRESENCE_STATE_UNINSTANTIATED)
            {
                node->status.wasMemberBefore = CL_TRUE;
                node->status.isClusterMember = CL_AMS_NODE_IS_NOT_CLUSTER_MEMBER;
            }
            else /* Node failed & recovered before Keepalives could indicate the issue (happens when AMF kill -9) */
            {
                /* So fail it */
                clLogWarning("AMF", "EVT", "Node [%s] is reentering cluster before its failure was discovered.  Failing it now.",pNodeName);
                clAmsPeNodeHasLeftCluster(node,CL_FALSE);
                /* cpmFailoverNode(node->config.id, CL_FALSE); */
                clLogWarning("AMF", "EVT", "Node [%s] failure completed.",pNodeName);
            }
            
            rc = CL_AMS_RC(CL_ERR_TRY_AGAIN);
            goto out_unlock;
        }
    }
    else
    {
        /*
         * We let the caller: CPM dictate the terms here. as the node
         * could be a dynamically added one not yet there in ams.
         */
        rc = CL_OK;
    }

    out_unlock:
    clOsalMutexUnlock(gAms.mutex);
    return rc;
}

/*
 * Save the instantiate command for amf for later access if required.
 */
void clAmsSetInstantiateCommand(ClInt32T argc, ClCharT **argv)
{
    register ClInt32T i;
    ClInt32T bytes = 0;
    for(i = 0; i < argc; ++i)
        bytes += snprintf(gClAmfInstantiateCommand + bytes, sizeof(gClAmfInstantiateCommand)-bytes,
                          "%s%s", i ? " " : "", 
                          argv[i]);
}

const ClCharT *clAmsGetInstantiateCommand(void)
{
    return (const ClCharT*)gClAmfInstantiateCommand;
}

static ClRcT clAmsCCBHandleDBCleanupCallback(ClHandleDatabaseHandleT db, ClHandleT handle, ClPtrT cookie)
{
    ClRcT rc = CL_OK;
    ClIocNotificationT *pNotification = (ClIocNotificationT *) cookie;
    clAmsMgmtCCBT *ccbInstance = NULL;
    ClBoolT isDelete = CL_FALSE;

    if ((pNotification->id == CL_IOC_COMP_DEATH_NOTIFICATION)
        && (pNotification->nodeAddress.iocPhyAddress.nodeAddress == (ClIocNodeAddressT)CL_HDL_NODE_ADDR(handle) )
        && (pNotification->nodeAddress.iocPhyAddress.portId == (ClIocPortT)CL_HDL_PORT_ADDR(handle)))
    {
        isDelete = CL_TRUE;
    }

    if (((pNotification->id == CL_IOC_NODE_LEAVE_NOTIFICATION) || (pNotification->id == CL_IOC_NODE_LINK_DOWN_NOTIFICATION))
        && (pNotification->nodeAddress.iocPhyAddress.nodeAddress == (ClIocNodeAddressT)CL_HDL_NODE_ADDR(handle) ))
    {
        isDelete = CL_TRUE;
    }

    if (isDelete)
    {
        rc = clOsalMutexLock(gAms.mutex);
        if(rc != CL_OK) return rc;

        rc = clHandleCheckout(db, handle, (ClPtrT*)&ccbInstance);
        if(rc != CL_OK)
        {
            clOsalMutexUnlock(gAms.mutex);
            return rc;
        }

        clAmsCCBOpListTerminate(&ccbInstance->ccbOpListHandle);
        clHandleDestroy(db, handle);
        clHandleCheckin(db, handle);

        rc = clOsalMutexUnlock(gAms.mutex);
        if(rc != CL_OK) return rc;
    }

    return rc;
}

ClRcT clAmsCCBHandleDBCleanup(ClIocNotificationT *pNotification)
{
    ClRcT rc = CL_OK;

    if (CL_HANDLE_INVALID_VALUE != gAms.ccbHandleDB)
    {
        rc = clHandleWalk(gAms.ccbHandleDB, clAmsCCBHandleDBCleanupCallback, (ClPtrT)pNotification);
    }

    return rc;
}

