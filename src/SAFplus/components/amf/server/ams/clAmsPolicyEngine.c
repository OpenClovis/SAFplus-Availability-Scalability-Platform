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
 * File        : clAmsPolicyEngine.c
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *
 * This file defines AMS policy functions for managing the state machines
 * associated with the life cycle of various AMS entities. These functions are 
 * AMS internal and form the core functionality of AMS.
 *
 *
 ***************************** Editor Commands ********************************
 * For vi/vim
 * :set shiftwidth=4
 * :set softtabstop=4
 * :set expandtab
 *****************************************************************************/
 
/******************************************************************************
 * Include files needed to compile this file
 *****************************************************************************/

#include <clCntApi.h>
#include <clOsalApi.h>
#include <clHeapApi.h>
#include <clAmsErrors.h>
#include <clErrorApi.h>
#include <clAmsTypes.h>
#include <clAmsEntities.h>
#include <clAmsServerUtils.h>
#include <clAmsPolicyEngine.h>
#include <clAmsDatabase.h>
#include <clAmsSAServerApi.h>
#include <clAmsModify.h>
#include <clAmsInvocation.h>
#include <clAmsMgmtHooks.h>

#include <clCpmInternal.h>
#include <clCpmAms.h>
#include <clCpmCommon.h>
#include <clAmsNotifications.h>
#include <clList.h>
#include <mPlusN.h>
#include <custom.h>

#define AMS_CPM_INTEGRATION
#define INVOCATION

#define CL_COMP_UNREACHABLE_ERROR(rc) (0)
#if 0
#define CL_COMP_UNREACHABLE_ERROR(rc) ( CL_GET_ERROR_CODE(rc) == CL_IOC_ERR_COMP_UNREACHABLE || \
                                        CL_GET_ERROR_CODE(rc) == CL_IOC_ERR_HOST_UNREACHABLE || \
                                        CL_GET_ERROR_CODE(rc) == CL_CPM_ERR_FORWARDING_FAILED)
#endif
        
extern ClAmsT   gAms;

/******************************************************************************
 * Global Definitions
 *****************************************************************************/
typedef struct ClAmsProxiedSUEntryT
{
    ClAmsSUT *su;
    ClListHeadT list;
}ClAmsProxiedSUEntryT;

typedef struct ClAmsRemoteProxiedSUsWalkArg
{
    ClAmsNodeT *node;
    ClListHeadT *proxiedSUList;
}ClAmsRemoteProxiedSUsWalkArgT;

typedef struct ClAmsCSIReplayFilter
{
    ClNameT *node;
    ClBoolT clearInvocation;
} ClAmsCSIReplayFilterT;

ClAmsEntityMethodsT gClAmsPeEntityDefaultMethods = 
{
    clAmsEntityPrint,
    clAmsEntityValidateConfig,
    clAmsEntityValidateRelationships,
};

ClAmsNodeMethodsT gClAmsPeNodeDefaultMethods = 
{
    {
        clAmsEntityPrint,
        clAmsNodeValidateConfig,
        clAmsNodeValidateRelationships,
    },
    clAmsPeNodeSUFailoverTimeout,
};

ClAmsAppMethodsT gClAmsPeAppDefaultMethods = 
{
    {
        clAmsEntityPrint,
        clAmsAppValidateConfig,
        clAmsAppValidateRelationships,
    },
};

ClAmsSGMethodsT gClAmsPeSGDefaultMethods = 
{
    {
        clAmsEntityPrint,
        clAmsSGValidateConfig,
        clAmsSGValidateRelationships,
    },
    clAmsPeSGInstantiateTimeout,
    clAmsPeSGAdjustTimeout,
    clAmsPeSGAdjustProbationTimeout,
};

ClAmsSUMethodsT gClAmsPeSUDefaultMethods = 
{
    {
        clAmsEntityPrint,
        clAmsSUValidateConfig,
        clAmsSUValidateRelationships,
    },
    clAmsPeSUSURestartTimeout,
    clAmsPeSUCompRestartTimeout,
    clAmsPeSUProbationTimeout,
    clAmsPeSUAssignmentTimeout,
};

ClAmsSIMethodsT gClAmsPeSIDefaultMethods = 
{
    {
        clAmsEntityPrint,
        clAmsSIValidateConfig,
        clAmsSIValidateRelationships,
    },
};

ClAmsCompMethodsT gClAmsPeCompDefaultMethods = 
{
    {
        clAmsEntityPrint,
        clAmsCompValidateConfig,
        clAmsCompValidateRelationships,
    },
    clAmsPeCompInstantiateTimeout,
    clAmsPeCompTerminateTimeout,
    clAmsPeCompCleanupTimeout,
    clAmsPeCompAmStartTimeout,
    clAmsPeCompAmStopTimeout,
    clAmsPeCompQuiescingCompleteTimeout,
    clAmsPeCompAssignCSITimeout,
    clAmsPeCompRemoveCSITimeout,
    clAmsPeCompProxiedCompInstantiateTimeout,
    clAmsPeCompProxiedCompCleanupTimeout,
    clAmsPeCompInstantiateDelayTimeout,
};

ClAmsCSIMethodsT gClAmsPeCSIDefaultMethods = 
{
    {
        clAmsEntityPrint,
        clAmsCSIValidateConfig,
        clAmsCSIValidateRelationships,
    },
};

static ClRcT
clAmsPeReplayCSIRemoveCallbacks(ClAmsInvocationT *pInvocations,
                                ClInt32T numInvocations,
                                ClAmsCSIReplayFilterT *filters,
                                ClUint32T numFilters,
                                ClUint32T switchoverMode);

static ClRcT 
clAmsPeSGFailoverHistoryRecover(ClAmsNodeT *node, ClAmsCompT *faultyComp);

static ClRcT
amsPeCompUpdateProxiedComponents(
                                 CL_IN ClAmsCompT *proxy,
                                 CL_IN ClAmsCompCSIRefT *proxyCSIRef,
                                 ClBoolT reassignCSI);

static ClRcT
clAmsPeNodeComputeAdminStateExtended(
        CL_IN   ClAmsNodeT *node,
        CL_OUT  ClAmsAdminStateT *adminState,
        CL_IN   ClAmsAdminStateT nodeAdminState);

static void
clAmsPeSUComputeAdminStateExtended(
        CL_IN       ClAmsSUT *su,
        CL_OUT      ClAmsAdminStateT *adminState,
        CL_IN       ClAmsAdminStateT suAdminState);

ClRcT
clAmsPeRemoteProxiedSUsForNode(ClAmsNodeT *node,
                               ClListHeadT *proxiedSUList);


/******************************************************************************
 * Cluster Functions
 *****************************************************************************/

static ClBoolT clAmsPeCheckCSIDependentsAssigned(ClAmsSUT *su, ClAmsCSIT *csi);

static __inline__ ClBoolT clAmsPeCheckNodeHasLeft(ClAmsNodeT *node)
{
    ClCpmLT *cpmL = NULL;
    ClBoolT nodeLeft = CL_FALSE;
    clOsalMutexLock(gpClCpm->cpmTableMutex);
    cpmNodeFindLocked(node->config.entity.name.value, &cpmL);
    if(!cpmL || !cpmL->pCpmLocalInfo || 
       cpmL->pCpmLocalInfo->status == CL_CPM_EO_DEAD)             
    {                  
        nodeLeft = CL_TRUE;
    }
    clOsalMutexUnlock(gpClCpm->cpmTableMutex);
    return nodeLeft;
}

/*-----------------------------------------------------------------------------
 * Functions invoked externally, main entry and exit fns to AMS PE
 *---------------------------------------------------------------------------*/
 
/*
 * clAmsPeClusterInstantiate
 * -------------------------
 * This function is called to kick off the AMF operation for a cluster.
 * Processing is started after a delay to enable the cluster to settle
 * down.
 */

ClRcT
clAmsPeClusterInstantiate(
        CL_IN ClAmsT *ams) 
{
    ClAmsEntityDbT *nodeDb = NULL;
    ClAmsEntityDbT *sgDb = NULL;
    ClRcT rc = CL_OK;

    AMS_CHECKPTR ( !ams );

    AMS_FUNC_ENTER (("\n"));

    if ( (ams->serviceState == CL_AMS_SERVICE_STATE_STARTINGUP)   ||
         (ams->serviceState == CL_AMS_SERVICE_STATE_RUNNING)      ||
         (ams->serviceState == CL_AMS_SERVICE_STATE_SHUTTINGDOWN) )
    {
        AMS_LOG (CL_DEBUG_TRACE,
            ("Cluster is already instantiated.\n"));

        return CL_OK;
    }

    AMS_LOG (CL_DEBUG_TRACE,
             ("Instantiating Cluster\n"));

    /*
     * Preprocess the AMS DB and establish any necessary relationships
     */

    if( (rc = clAmsPePreprocessDb(ams) ) != CL_OK)
    {
        clLogError("CLUSTER", "INIT", "AMS DB pre-processing failed with [%#x]", rc);
        return CL_AMS_RC(CL_ERR_INVALID_STATE);
    }

    ams->epoch = time(NULL);

    ams->serviceState = CL_AMS_SERVICE_STATE_STARTINGUP;

    /*
     * Instantiate all nodes in the db but only if their dependencies
     * are satisfied. The process of instantiation builds the ordered
     * instantiable SU list for each SG (assuming SGs are defined).
     */

    nodeDb = &ams->db.entityDb[CL_AMS_ENTITY_TYPE_NODE];

    if ( nodeDb->numEntities ) 
    {
        AMS_CALL ( clAmsEntityDbWalk(nodeDb,
                        (ClAmsEntityCallbackT) clAmsPeNodeColdplugJoinCluster) ); 
    }
    else
    {
        AMS_LOG (CL_DEBUG_TRACE,
                 ("Node db is empty. Continuing..\n") ); 
    }

    /*
     * Instantiate/start processing for all SGs.
     */

    // Future: Change this to application group 

    sgDb = &ams->db.entityDb[CL_AMS_ENTITY_TYPE_SG];

    if ( sgDb->numEntities ) 
    {
        AMS_CALL ( clAmsEntityDbWalk(sgDb,
                        (ClAmsEntityCallbackT) clAmsPeSGInstantiate) );
    }
    else
    {
        AMS_LOG (CL_DEBUG_TRACE,
                 ("SG db is empty. Continuing..\n") );
    }

    /*
     * Change state of ams service to running
     */

    ams->serviceState = CL_AMS_SERVICE_STATE_RUNNING;

    return CL_OK;
}

/*
 * clAmsPeClusterTerminate
 * -----------------------
 * Called when AMS is to be shut down.
 */

ClRcT
clAmsPeClusterTerminate(
        CL_IN ClAmsT *ams) 
{
    AMS_CHECKPTR ( !ams );

    AMS_FUNC_ENTER (("\n"));

    AMS_LOG (CL_DEBUG_TRACE,
             ("Terminating Cluster: Shutting down all SGs\n"));

    ams->serviceState = CL_AMS_SERVICE_STATE_SHUTTINGDOWN;

    /*
     * First, remove work from all SGs, so the SUs will checkpoint their
     * state appropriately.
     */

    // Future: Change this to application group 

    ClAmsEntityDbT *sgDb = &ams->db.entityDb[CL_AMS_ENTITY_TYPE_SG];
    
    if ( clAmsEntityDbWalk(
                sgDb,
                (ClAmsEntityCallbackT) clAmsPeSGHasAssignments ) )
    {
        AMS_CALL ( clAmsEntityDbWalk(sgDb,
                        (ClAmsEntityCallbackT) clAmsPeSGShutdown) );
    }
    else
    {
        AMS_CALL ( clAmsPeClusterTerminateCallback_Step1(ams) );
    }

    return CL_OK;
}

/*
 * clAmsPeClusterTerminateCallback_Step1
 * -------------------------------------
 * Callback for cluster terminate. Called from SG/Application shutdown callback.
 */

ClRcT
clAmsPeClusterTerminateCallback_Step1(
        CL_IN ClAmsT *ams) 
{
    ClAmsEntityDbT  *sgDb;

    AMS_CHECKPTR ( !ams );

    AMS_FUNC_ENTER (("\n"));

    if ( ams->serviceState != CL_AMS_SERVICE_STATE_SHUTTINGDOWN )
    {
        AMS_LOG (CL_DEBUG_TRACE,
            ("Terminating Cluster: Not in ShuttingDown state. Ignoring..\n"));

        return CL_OK;
    }

    // Future: Change this to application group 

    sgDb = &ams->db.entityDb[CL_AMS_ENTITY_TYPE_SG];
   
    if ( clAmsEntityDbWalk(sgDb,
                    (ClAmsEntityCallbackT) clAmsPeSGHasAssignments) )
    {
        AMS_LOG (CL_DEBUG_TRACE,
            ("Terminating Cluster: There are still assigned SUs. Ignoring..\n"));

        return CL_OK;
    }

    AMS_LOG (CL_DEBUG_TRACE,
             ("Terminating Cluster: Terminating all SUs in all SGs\n"));

    if ( clAmsEntityDbWalk(sgDb,
                    (ClAmsEntityCallbackT) clAmsPeSGIsInstantiated) )
    {
        AMS_CALL ( clAmsEntityDbWalk(sgDb,
                        (ClAmsEntityCallbackT) clAmsPeSGTerminate) );
    }
    else
    {
        AMS_CALL ( clAmsPeClusterTerminateCallback_Step2(ams) );
    }

    return CL_OK;
}

/*
 * clAmsPeClusterTerminateCallback_Step2
 * -------------------------------------
 * Callback for cluster terminate. Called from SG/Application terminate callback.
 */

ClRcT
clAmsPeClusterTerminateCallback_Step2(
        CL_IN ClAmsT *ams) 
{
    ClAmsEntityDbT *sgDb;

    AMS_CHECKPTR ( !ams );

    AMS_FUNC_ENTER (("\n"));

    if ( ams->serviceState != CL_AMS_SERVICE_STATE_SHUTTINGDOWN )
    {
        AMS_LOG (CL_DEBUG_TRACE,
            ("Terminating Cluster: Not in ShuttingDown state. Ignoring..\n"));

        return CL_OK;
    }

    // Future: Change this to application group 

    sgDb = &ams->db.entityDb[CL_AMS_ENTITY_TYPE_SG];

    if ( clAmsEntityDbWalk(sgDb,
                    (ClAmsEntityCallbackT) clAmsPeSGIsInstantiated) )
    {
        AMS_LOG (CL_DEBUG_TRACE,
            ("Terminating Cluster: There are still instantiated SUs. Ignoring..\n"));

        return CL_OK;
    }

#ifdef NOTNEEDEDFORNOW

    ClAmsEntityDbT *nodeDb;

    AMS_LOG (CL_DEBUG_TRACE,
             ("Terminating Cluster: Terminating all Nodes\n"));

    /*
     * Then, terminate all nodes in the db. All SUs will have been terminated
     * as part of SG terminate, so this will simply reset internal state. This
     * may be useful when we allow a cluster reset.
     */
    nodeDb = &ams->db.entityDb[CL_AMS_ENTITY_TYPE_NODE];

    if ( nodeDb->numEntities ) 
    {
        /*
         * Future: While node dependencies will be taken into account in the
         * termination process below, just walking the node list will not give
         * a specific ordering of termination which may be desired in some
         * cases.
         */

        AMS_CALL ( clAmsEntityDbWalk(nodeDb,
                        (ClAmsEntityCallbackT) clAmsPeNodeReset) ); 

        //AMS_CALL ( clAmsEntityDbWalk(nodeDb,
                        //(ClAmsEntityCallbackT) clAmsPeNodeTerminate) ); 
    }

#endif

    AMS_LOG (CL_DEBUG_TRACE,
             ("Terminating Cluster: Stopping AMF\n"));

    ams->serviceState = CL_AMS_SERVICE_STATE_STOPPED;

    /*
     * Signal the thread that originally called cluster terminate that all
     * async actions have been completed. The calling thread waits on this
     * condition variable, only if shutdown mode is graceful.
     */

    if ( ams->timerCount )
    {
        AMS_LOG (CL_DEBUG_TRACE,
             ("Terminating Cluster: Waiting for [%d] pending timers to pop\n",
              ams->timerCount));
    }
    else
    {
        AMS_LOG (CL_DEBUG_TRACE,
             ("Terminating Cluster: AMF Stopped\n"));

        AMS_CALL ( clOsalMutexLock(ams->terminateMutex) );
        AMS_CALL ( clOsalCondSignal(ams->terminateCond) );
        AMS_CALL ( clOsalMutexUnlock(ams->terminateMutex) );
    }

    return CL_OK;
}

/*
 * clAmsPeClusterFaultReport
 * -------------------------
 * This fn is called to report a fault on the cluster as a whole.
 */

ClRcT
clAmsPeClusterFaultReport(
        CL_IN       ClAmsT                      *ams,
        CL_INOUT    ClAmsLocalRecoveryT         *recovery,
        CL_INOUT    ClUint32T                   *escalation)
{
    AMS_LOG(CL_DEBUG_CRITICAL,
            ("ALERT: Critical Fault! Terminating Cluster\n"));

    
#ifdef AMS_CPM_INTEGRATION
    _clAmsSAClusterReset(ams);
#endif

    return CL_OK;
}

/******************************************************************************
 * SG Functions
 *****************************************************************************/

/*-----------------------------------------------------------------------------
 * Functions called from the Management API
 *---------------------------------------------------------------------------*/
 
/*
 * clAmsPeSGUnlock
 * ---------------
 * Unlocking a SG allows AMS to make work assignments to the constituent
 * SUs in the SG (assuming conditions are met).
 */

ClRcT
clAmsPeSGUnlock(
        CL_IN ClAmsSGT *sg) 
{
    ClAmsAdminStateT adminState;

    AMS_CHECK_SG ( sg );

    AMS_FUNC_ENTER ( ("SG [%s]\n",sg->config.entity.name.value) );

    AMS_ENTITY_LOG (sg, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("Admin Operation [Unlock] on SG [%s]\n",
             sg->config.entity.name.value));

    if(clAmsInvocationsPendingForSG(sg))
    {
        clLogInfo("SG", "UNLOCK", 
                  "SG [%s] has invocations pending. Deferring unlock",
                  sg->config.entity.name.value);
        return CL_AMS_RC(CL_ERR_TRY_AGAIN);
    }

    CL_AMS_SET_A_STATE(sg, CL_AMS_ADMIN_STATE_UNLOCKED);

    adminState = clAmsPeSGComputeAdminState(sg);

    if ( adminState == CL_AMS_ADMIN_STATE_UNLOCKED )
    {
        AMS_CALL ( clAmsPeSGEvaluateWork(sg) );
    }

    /*
     * We currently do not get an async unlock callback, so we will call it 
     * here. Future, this should come from SGInstantiateCallback (not there).
     */

    AMS_CALL ( clAmsPeSGUnlockCallback(sg, CL_OK) );

    return CL_OK;
}

ClRcT
clAmsPeSGUnlockCallback(
        CL_IN       ClAmsSGT            *sg,
        CL_IN       ClUint32T           error)
{
    return CL_OK;
}

/*
 * clAmsPeSGLockAssignment
 * -----------------------
 * When a SG is administratively put in locked-assignment, SUs within the
 * SG can be instantiated but they cannot be assigned any work. The actions
 * vary depending on current state of the SG.
 */

ClRcT
clAmsPeSGLockAssignment(
        CL_IN ClAmsSGT *sg) 
{
    ClAmsAdminStateT adminState;

    AMS_CHECK_SG ( sg );

    AMS_FUNC_ENTER ( ("SG [%s]\n",sg->config.entity.name.value) );

    AMS_ENTITY_LOG (sg, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("Admin Operation [Lock Assignment] on SG [%s]\n",
             sg->config.entity.name.value));

    if(clAmsInvocationsPendingForSG(sg))
    {
        clLogInfo("SG", "LOCK", 
                  "SG [%s] has invocations pending. Deferring lock assignment",
                  sg->config.entity.name.value);
        return CL_AMS_RC(CL_ERR_TRY_AGAIN);
    }

    adminState = clAmsPeSGComputeAdminState(sg);

    CL_AMS_SET_A_STATE(sg, CL_AMS_ADMIN_STATE_LOCKED_A);

    if ( adminState == CL_AMS_ADMIN_STATE_LOCKED_I )
    {
        AMS_CALL ( clAmsPeSGInstantiate(sg) );
    }
    else if ( adminState == CL_AMS_ADMIN_STATE_UNLOCKED )
    {
        AMS_CALL ( clAmsPeSGRemoveWork(sg, CL_AMS_ENTITY_SWITCHOVER_IMMEDIATE) );
    }

    return CL_OK;
}

ClRcT
clAmsPeSGLockAssignmentCallback(
        CL_IN       ClAmsSGT            *sg,
        CL_IN       ClUint32T           error)
{
    if ( clAmsPeSGHasAssignments(sg) )
    {
        AMS_ENTITY_LOG (sg, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("SG [%s] has assignments. Ignoring Lock Assignment Callback..\n",
             sg->config.entity.name.value));

        return CL_OK;
    }

    AMS_ENTITY_LOG (sg, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
        ("Admin Operation [Lock Assignment] on SG [%s] is now complete\n",
         sg->config.entity.name.value));

    return CL_OK;
}

/*
 * clAmsPeSGLockInstantiation
 * --------------------------
 * Lock instantiation is the lowest energy state for a SG. When a SG goes
 * into this state, all constituent SUs are terminated but they remain in
 * the instantiable SU list and can be brought to life by reinstantiating
 * them.
 */

ClRcT
clAmsPeSGLockInstantiation(
        CL_IN ClAmsSGT *sg) 
{
    ClAmsAdminStateT adminState;

    AMS_CHECK_SG ( sg );

    AMS_FUNC_ENTER ( ("SG [%s]\n",sg->config.entity.name.value) );

    AMS_ENTITY_LOG (sg, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("Admin Operation [Lock Instantiation] on SG [%s]\n",
             sg->config.entity.name.value));

    if(clAmsInvocationsPendingForSG(sg))
    {
        clLogInfo("SG", "LOCKI", 
                  "SG [%s] has pending invocations. Deferring lock instantiation",
                  sg->config.entity.name.value);
        return CL_AMS_RC(CL_ERR_TRY_AGAIN);
    }

    adminState = clAmsPeSGComputeAdminState(sg);
    
    CL_AMS_SET_A_STATE(sg, CL_AMS_ADMIN_STATE_LOCKED_I);

    if ( adminState == CL_AMS_ADMIN_STATE_LOCKED_A )
    {
        AMS_CALL ( clAmsPeSGTerminate(sg) );
    }

    return CL_OK;
}

ClRcT
clAmsPeSGLockInstantiationCallback(
        CL_IN       ClAmsSGT            *sg,
        CL_IN       ClUint32T           error)
{
    ClAmsAdminStateT adminState;

    AMS_CHECK_SG ( sg );

    AMS_FUNC_ENTER ( ("SG [%s]\n",sg->config.entity.name.value) );

    adminState = clAmsPeSGComputeAdminState(sg);
    
    if ( adminState != CL_AMS_ADMIN_STATE_LOCKED_I )
    {
        AMS_ENTITY_LOG (sg, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("Lock Instantiation callback for SG [%s] in admin state [%s] has no effect. Continuing..\n",
             sg->config.entity.name.value,
             CL_AMS_STRING_A_STATE(adminState)));

       return CL_OK; 
    }

    AMS_CALL ( clAmsEntityListWalkGetEntity(
                    &sg->config.suList,
                    (ClAmsEntityCallbackT)clAmsPeSUMarkUninstantiable) );

    return CL_OK;
}

/*
 * clAmsPeSGShutdown
 * -----------------
 * This function administratively shuts down a service group. For all SUs 
 * that are a member of the SG, this is treated as a shutting down of the SU, 
 * meaning that no new work assignments are possible and existing work 
 * assignments must be gracefully terminated.
 */

ClRcT
clAmsPeSGShutdown(
        CL_IN ClAmsSGT *sg) 
{
    ClAmsAdminStateT oldAdminState;

    AMS_CHECK_SG ( sg );

    AMS_FUNC_ENTER ( ("SG [%s]\n",sg->config.entity.name.value) );

    AMS_ENTITY_LOG (sg, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("Admin Operation [Shutdown] on SG [%s]\n",
             sg->config.entity.name.value));

    if(clAmsInvocationsPendingForSG(sg))
    {
        clLogInfo("SG", "SHUTDOWN", "SG [%s] has pending invocations. Deferring shutdown",
                  sg->config.entity.name.value);
        return CL_AMS_RC(CL_ERR_TRY_AGAIN);
    }

    oldAdminState = clAmsPeSGComputeAdminState(sg);
    
    CL_AMS_SET_A_STATE(sg, CL_AMS_ADMIN_STATE_SHUTTINGDOWN);

    if ( oldAdminState == CL_AMS_ADMIN_STATE_UNLOCKED )
    {
        AMS_CALL ( clAmsPeSGRemoveWork(sg, CL_AMS_ENTITY_SWITCHOVER_GRACEFUL) );
    }

    return CL_OK;
}

/*
 * clAmsPeSGShutdownCallback
 * -------------------------
 * This fn is called asynchronously after a SG shutdown is complete,
 * typically after all work is removed from a SU.
 */

ClRcT
clAmsPeSGShutdownCallback(
        CL_IN       ClAmsSGT        *sg,
        CL_IN       ClUint32T       error)
{
    ClAmsAdminStateT adminState;

    AMS_CHECK_SG ( sg );

    AMS_FUNC_ENTER ( ("SG [%s]\n",sg->config.entity.name.value) );

    adminState = clAmsPeSGComputeAdminState(sg);
    
    if ( clAmsPeSGHasAssignments(sg) )
    {
        AMS_ENTITY_LOG (sg, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("SG [%s] has assignments. Ignoring Shutdown Callback..\n",
             sg->config.entity.name.value));

        return CL_OK;
    }

    if ( adminState != CL_AMS_ADMIN_STATE_SHUTTINGDOWN )
    {
        AMS_ENTITY_LOG (sg, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("Shutdown callback for SG [%s] in admin state [%s] has no effect. Continuing..\n",
             sg->config.entity.name.value,
             CL_AMS_STRING_A_STATE(adminState)));

       return CL_OK; 
    }

    AMS_ENTITY_LOG (sg, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
        ("Admin Operation [Shutdown] on SG [%s] is now complete\n",
         sg->config.entity.name.value));

    if ( sg->config.adminState == CL_AMS_ADMIN_STATE_SHUTTINGDOWN )
    {
        CL_AMS_SET_A_STATE(sg, CL_AMS_ADMIN_STATE_LOCKED_A);
    }

    // Future : Call application shutdown callback which calls clusterterminate
    // The extern below is temporary, until a separate cluster entity is
    // added.

    extern ClAmsT gAms; 

    if ( gAms.serviceState == CL_AMS_SERVICE_STATE_SHUTTINGDOWN )
    {
        AMS_CALL ( clAmsPeClusterTerminateCallback_Step1(&gAms) );
    }

    return CL_OK;
}

/*-----------------------------------------------------------------------------
 * Functions called as a result of AMS actions
 *----------------------------------------------------------------------------*/

/*
 * clAmsPeSGInstantiate
 * --------------------
 * Start an SG. This means starting the SUs that are part of the SG based
 * on the SG's redundancy model and loading strategy.
 */

ClRcT
clAmsPeSGInstantiate(
        CL_IN ClAmsSGT *sg) 
{
    ClAmsAdminStateT adminState;

    AMS_CHECK_SG ( sg );

    AMS_FUNC_ENTER (("SG [%s]\n",sg->config.entity.name.value));

    adminState = clAmsPeSGComputeAdminState(sg);
    
    if ( (adminState != CL_AMS_ADMIN_STATE_UNLOCKED) &&
         (adminState != CL_AMS_ADMIN_STATE_LOCKED_A) )
    {
        AMS_ENTITY_LOG (sg, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("Instantiating SG [%s] in admin state [%s] has no effect. Continuing..\n",
             sg->config.entity.name.value,
             CL_AMS_STRING_A_STATE(adminState)));

        return CL_OK;
    }

    if ( sg->status.isStarted == CL_TRUE )
    {
        AMS_ENTITY_LOG (sg, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("SG [%s] is already started. Ignoring instantiate request..\n",
            sg->config.entity.name.value));
        
        return CL_OK;
    }

    /*
     * Kick off the instantiation of any necessary SUs and then
     * wait for them to instantiate.
     */

    AMS_ENTITY_LOG (sg, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
             ("Instantiating SG [%s]. Marking eligible SUs as instantiable.\n",
              sg->config.entity.name.value));

    AMS_CALL ( clAmsEntityListWalkGetEntity(
                    &sg->config.suList,
                    (ClAmsEntityCallbackT)clAmsPeSUMarkInstantiable) );

    AMS_CALL ( clAmsPeSGEvaluateWork(sg) );


    AMS_CALL ( clAmsEntityTimerStart(
                    (ClAmsEntityT *)sg,
                    CL_AMS_SG_TIMER_INSTANTIATE) );

    return CL_OK;
}

/*
 * clAmsPeSGInstantiateTimeout
 * ---------------------------
 * After kicking off the instantiation of SUs in an SG, it is desirable
 * that the initial assignment of work be as close to the desired state 
 * of the SG. Hence, clAmsPeSGInstantiate sets up a timer and action 
 * continues in this function after an async break. This timer should 
 * be large enough (system designer configured) that all of the desired 
 * SUs will have instantiated.
 */

ClRcT
clAmsPeSGInstantiateTimeout(
        CL_IN ClAmsEntityTimerT *timer) 
{
    ClAmsSGT *sg;
    ClAmsAdminStateT adminState;

    AMS_CHECKPTR ( !timer );

    AMS_CHECK_SG ( sg = (ClAmsSGT *) timer->entity );

    AMS_FUNC_ENTER (("SG [%s]\n",sg->config.entity.name.value));

    AMS_CALL ( clAmsEntityTimerStop(
                    (ClAmsEntityT *) sg,
                    CL_AMS_SG_TIMER_INSTANTIATE) );

    adminState = clAmsPeSGComputeAdminState(sg);
    
    if ( (adminState == CL_AMS_ADMIN_STATE_LOCKED_A) ||
         (adminState == CL_AMS_ADMIN_STATE_UNLOCKED) )
    {
        sg->status.isStarted = CL_TRUE;
        
        CL_AMS_SET_EPOCH(sg);

        AMS_CALL ( clAmsPeSGEvaluateWork(sg) );
    }

    return CL_OK;
}

/*
 * clAmsPeSGTerminate
 * ------------------
 * Terminate is the opposite of instantiate.
 */

ClRcT
clAmsPeSGTerminate(
        CL_IN ClAmsSGT *sg)
{
    AMS_CHECK_SG ( sg );

    AMS_FUNC_ENTER ( ("SG [%s]\n",sg->config.entity.name.value) );

    AMS_CALL ( clAmsEntityTimerStop(
                    (ClAmsEntityT *) sg,
                    CL_AMS_SG_TIMER_INSTANTIATE) );

    CL_AMS_RESET_EPOCH(sg);

    sg->status.isStarted = CL_FALSE;

    if ( clAmsPeSGIsInstantiated(sg) )
    {
        AMS_CALL ( clAmsEntityListWalkGetEntity(
                        &sg->config.suList,
                        (ClAmsEntityCallbackT) clAmsPeSUTerminate) );
    }
    else
    {
        AMS_CALL ( clAmsPeSGTerminateCallback(sg, CL_OK) );
    }
    
    return CL_OK;
}

/*
 * clAmsPeSGTerminateCallback
 * --------------------------
 * This is called from SU terminate callback.
 */

ClRcT
clAmsPeSGTerminateCallback(
        CL_IN ClAmsSGT *sg,
        CL_IN ClRcT error)
{
    ClAmsAdminStateT adminState;

    AMS_CHECK_SG ( sg );

    AMS_FUNC_ENTER ( ("SG [%s]\n",sg->config.entity.name.value) );

    if ( clAmsPeSGIsInstantiated(sg) )
    {
        AMS_ENTITY_LOG(sg, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("SG [%s] is still instantiated. Ignoring SG terminate callback\n",
             sg->config.entity.name.value))

        return CL_OK;
    }

    adminState = clAmsPeSGComputeAdminState(sg);
    
    if ( adminState == CL_AMS_ADMIN_STATE_LOCKED_I )
    {
        AMS_CALL ( clAmsPeSGLockInstantiationCallback(sg, error) );
    }

    // Future : Call application terminate callback which calls clusterterminate
    // The extern below is temporary, until a separate cluster entity is added.

    extern ClAmsT gAms; 

    if ( gAms.serviceState == CL_AMS_SERVICE_STATE_SHUTTINGDOWN )
    {
        AMS_CALL ( clAmsPeClusterTerminateCallback_Step2(&gAms) );
    }

    return CL_OK;
}

/*
 * clAmsPeSGEvaluateWork
 * ---------------------
 * Given a SG, this function evaluates the status of the SG and based on 
 * its redundancy model figures out what changes are needed in terms of
 * instantiating, assigning SUs and SIs. Then it checks to see if any
 * SUs are available and instantiates/assigns them appropriately.
 *
 * This function can be called when the system is first started up as
 * well as when a new SU, Node or SI is added to the system.
 *
 * The symmetric opposite of this fn is actually composed of two parts,
 * one is clAmsSGRemoveWork and the other is clAmsSGTerminate.
 */

ClRcT
clAmsPeSGEvaluateWork(
                      CL_IN ClAmsSGT *sg
                      )
{
    ClAmsAdminStateT adminState;

    AMS_CHECK_SG ( sg );

    AMS_FUNC_ENTER (("SG [%s]\n",sg->config.entity.name.value));

    /*
     * Verify that AMS is up and running.
     */

    extern ClAmsT gAms;

    if ( (gAms.serviceState != CL_AMS_SERVICE_STATE_STARTINGUP) &&
         (gAms.serviceState != CL_AMS_SERVICE_STATE_RUNNING) )
    {
        return CL_OK;
    }

    /*
     * We can evaluate work for a SG if its computed admin state is 
     * unlocked or lock-a. The gAms hack is temporary until we have
     * a separate cluster entity and the computed admin state of SG
     * includes the cluster state.
     */

    adminState = clAmsPeSGComputeAdminState(sg);

    if ( (adminState != CL_AMS_ADMIN_STATE_UNLOCKED) && (adminState != CL_AMS_ADMIN_STATE_LOCKED_A) )
    {
        AMS_ENTITY_LOG(sg, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("SG [%s] has computed admin state [%s]. SU instantiation and assignment not possible. Continuing..\n",
             sg->config.entity.name.value,
             CL_AMS_STRING_A_STATE(adminState)));

        return CL_OK;
    }

    /*
     * If this SG is not fully operational because of initial waiting period
     * don't do any instantiations or assignments.
     */

    if ( !sg->status.isStarted )
    {
        AMS_ENTITY_LOG(sg, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("SG [%s] in initial waiting period. SU instantiation and assignment will proceed after timeout..\n",
             sg->config.entity.name.value))

        return CL_OK;
    }

    AMS_ENTITY_LOG (sg, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
             ("Evaluating work for SG [%s]\n",
              sg->config.entity.name.value));

    /*
     * Run the adjustment procedure on the SG.
     */
    clAmsPeSGAutoAdjust(sg);

    /*
     * Instantiation is allowed if SG is unlocked or lock_a.
     */

    AMS_CALL ( clAmsPeSGInstantiateSUs(sg) );

    /*
     * Assignment is only allowed if SG is unlocked.
     */

    if ( adminState != CL_AMS_ADMIN_STATE_UNLOCKED )
    {
        AMS_ENTITY_LOG(sg, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("SG [%s] has computed admin state [%s]. SU assignment not possible. Continuing..\n",
             sg->config.entity.name.value,
             CL_AMS_STRING_A_STATE(adminState)));

        return CL_OK;
    }

    AMS_CALL ( clAmsPeSGAssignSUs(sg) );

    return CL_OK;
}

/*
 * clAmsPeSGInstantiateSUs
 * -----------------------
 * Instantiate the SUs in a SG based on availability and needs of SG. This fn
 * works for all SG redundancy models if the configuration is right.
 */

ClRcT
clAmsPeSGInstantiateSUs(
        CL_IN   ClAmsSGT *sg)
{
    ClUint32T instantiated, needToInstantiate, canInstantiate, instantiating;

    AMS_FUNC_ENTER ( ("SG [%s]\n",sg->config.entity.name.value) );

    /*
     * Step 1: Determine the number of SUs that this SG needs to instantiate.
     * Then figure out how many can be instantiated based on the number of
     * SUs in the instantiable list. This number reflects the maximum possible
     * SUs that can be instantiated but it may not be possible to instantiate
     * all of them as not all may be unlocked (taking into account node and
     * SG admin state).
     */

    instantiated = sg->status.instantiatedSUList.numEntities   +
                   sg->status.inserviceSpareSUList.numEntities +
                   sg->status.assignedSUList.numEntities;

    needToInstantiate = sg->config.numPrefInserviceSUs - instantiated;

    canInstantiate = AMS_MIN(sg->status.instantiableSUList.numEntities,
                             needToInstantiate);

    instantiating = 0;

    AMS_ENTITY_LOG(sg, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("Instantiating SUs for SG [%s] : Instantiated [%d], Need [%d], Available [%d], Instantiating [%d]\n",
             sg->config.entity.name.value,
             instantiated,
             needToInstantiate,
             sg->status.instantiableSUList.numEntities,
             canInstantiate));

    if ( canInstantiate )
    {
        ClAmsEntityRefT *entityRef;

        entityRef = clAmsEntityListGetFirst(&sg->status.instantiableSUList);
        ClUint32T numEntities=  sg->status.instantiableSUList.numEntities;
        while ( (entityRef != (ClAmsEntityRefT *) NULL) && (canInstantiate > 0) )
        {
            ClRcT rc;
            ClAmsSUT *su = (ClAmsSUT *)entityRef->ptr;
            ClAmsNodeT *node ;

            /*
             * Get a ptr to the next SU in line. We do this upfront, because
             * SU instantiate will modify the instantiableSUList.
             */
            entityRef = clAmsEntityListGetNext(&sg->status.instantiableSUList,entityRef);

            
            if (!AMS_ENTITY_OK ( su, CL_AMS_ENTITY_TYPE_SU))
            {
                clDbgCodeError(CL_ERR_INVALID_PARAMETER, ("instantiableSUList contains an invalid SU entity ref: [%p]",(void*) su));
                continue;                
            }
            
            node = (ClAmsNodeT*)su->config.parentNode.ptr;
            if (!node)
            {
                clLogWarning("AMF","PE","Attempt to instantiate a SU that has no parent node");
                continue;                
            }
            
            /*
             * Check if the SU is instantiable and if so, instantiate it.
             */

            if( clAmsPeSUIsInstantiable(su) != CL_OK )
            {
                AMS_ENTITY_LOG(su, CL_AMS_MGMT_SUB_AREA_MSG, CL_LOG_SEV_DEBUG, ("SU [%s] is not instantiable. Ignoring SU..\n", su->config.entity.name.value));                
                continue;
            }

            /*
             * If a SU is restarting, let it alone.
             */

            if ( su->status.presenceState == CL_AMS_PRESENCE_STATE_RESTARTING )
            {
                clLogWarning("AMF","PE","Cannot instantiate an SU that is restarting");
                continue;
            }

            /* 
             * An SU is instantiated only if permitted by its computed admin 
             * state. 
             *
             * Future: We may want to give preference for unlocked SUs during
             * instantiation so we don't end up with a situation where there
             * are sufficient SUs instantiated, but all are lock assigned and
             * thus not assignable. If there are any unlocked spare SUs lower
             * down the rank list, they will not get instantiated. Note, SAF
             * has no direction on this.
             */

            if ((rc=clAmsPeSUInstantiate(su))!=CL_OK)
            {
                clLogError("AMF","PE","SU instantiate error [0x%x:%s]",rc,clErrorToString(rc));
            }
            

            /*
             * Check number of instantiableSUList. If more than one entity is removed,
             * we force the instantiate to restart from the begining to avoid dangling pointer crash the system.
             */ 
            if(sg->status.instantiableSUList.numEntities < numEntities -1)
            {   
                entityRef = clAmsEntityListGetFirst(&sg->status.instantiableSUList);
            }

            numEntities = sg->status.instantiableSUList.numEntities;
            canInstantiate--;
            instantiating++;
        }

        if ( instantiating )
        {
            AMS_ENTITY_LOG (sg, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
                 ("Instantiating SUs for SG [%s] : Instantiating total [%d] SUs\n", 
                  sg->config.entity.name.value,
                  instantiating));
        }

        if ( canInstantiate )
        {
            AMS_ENTITY_LOG (sg, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
                 ("Instantiating SUs for SG [%s] : Need [%d] more SUs\n", 
                  sg->config.entity.name.value,
                  canInstantiate));
        }
    }

    return CL_OK;
}

/*
 * clAmsPeSGAssignSUs
 * ------------------
 * Based on the redundancy model, this fn demuxes out to the appropriate fn
 * for assigning SIs to SUs.
 */

ClRcT
clAmsPeSGAssignSUs(
                   CL_IN   ClAmsSGT *sg
                   )
{
    AMS_CHECK_SG ( sg );

    AMS_FUNC_ENTER ( ("SG [%s]\n",sg->config.entity.name.value) );

    if ( !sg->config.suList.numEntities )
    {
        return CL_OK;
    }

    switch ( sg->config.redundancyModel )
    {
        case CL_AMS_SG_REDUNDANCY_MODEL_NO_REDUNDANCY:
        case CL_AMS_SG_REDUNDANCY_MODEL_TWO_N:
        case CL_AMS_SG_REDUNDANCY_MODEL_M_PLUS_N:
        {
            /*
             * The same fn works for all of the above redundancy models as long
             * as the various SG attributes are configured appropriately.
             *
             * No Redundancy:   M=1     N=0     S=0..s
             * 2N:              M=1     N=1     S=0..s
             * M+N:             M=1..m  N=1..n  S=0..s
             */

            AMS_CALL ( clAmsPeSGAssignSUMPlusN(sg) );

            break;
        }

        case CL_AMS_SG_REDUNDANCY_MODEL_N_WAY:
        {
            //AMS_CALL ( clAmsPeSGAssignSUNWay(sg) );

            break;
        }

        case CL_AMS_SG_REDUNDANCY_MODEL_N_WAY_ACTIVE:
        {
            //AMS_CALL ( clAmsPeSGAssignSUNWayActive(sg) );

            break;
        }

        case CL_AMS_SG_REDUNDANCY_MODEL_CUSTOM:
        {
            /*
             * User controlled redundancy.
             */
            AMS_CALL ( clAmsPeSGAssignSUCustom(sg) );
            break;
        }
        
        default:
        {
            AMS_ENTITY_LOG(sg, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_ERROR,
                    ("Error: Invalid Redundancy model [%d] for SG [%s]. Exiting..\n",
                     sg->config.redundancyModel,
                     sg->config.entity.name.value));

            return CL_AMS_RC(CL_AMS_ERR_BAD_CONFIG);
        }
    }

    return CL_OK;
}

/*
 * clAmsPeSGFindSIForActiveAssignment
 * ----------------------------------
 * This fn figures out the next preferred SI that can be assigned the 
 * active HA state. This takes the SIs dependencies into account as
 * well as its rank. Note, the SI dependencies can be on _any_ SI in
 * the cluster, not just SIs within the same SG.
 *
 * This fn should work for all SG models.
 */

ClRcT
clAmsPeSGFindSIForActiveAssignment(
        CL_IN ClAmsSGT *sg,
        CL_INOUT ClAmsSIT **targetSI) 
{
    ClAmsEntityRefT *entityRef;
    ClAmsSIT* lookAfter = NULL;

    AMS_CHECK_SG ( sg );
    AMS_CHECKPTR ( !targetSI );

    AMS_FUNC_ENTER ( ("SG [%s]\n",sg->config.entity.name.value) );

    if (*targetSI) lookAfter = *targetSI;

    /*
     * For SI preference loading strategy, try to find the SI which has an assignable preferred SU first
     */
    if(!sg->config.autoAdjust && sg->config.loadingStrategy == CL_AMS_SG_LOADING_STRATEGY_BY_SI_PREFERENCE)
    {
        for (entityRef = clAmsEntityListGetFirst(&sg->config.siList);
             entityRef !=  NULL;
             entityRef = clAmsEntityListGetNext(&sg->config.siList, entityRef)) 
        {
            ClAmsEntityRefT *suRef;
            ClAmsSIT *si = (ClAmsSIT*)entityRef->ptr;
            AMS_CHECK_SI(si);
            if(lookAfter == si) continue;
            if(clAmsPeSIIsActiveAssignable(si) != CL_OK)
                continue;
            for(suRef = clAmsEntityListGetFirst(&si->config.suList);
                suRef != NULL;
                suRef = clAmsEntityListGetNext(&si->config.suList, suRef))
            {
                ClAmsSUT *su = (ClAmsSUT*)suRef->ptr;
                if(clAmsPeSUIsAssignable(su) != CL_OK) 
                    continue;
                if(su->status.readinessState != CL_AMS_READINESS_STATE_INSERVICE)
                    continue;
                if(su->status.numStandbySIs || su->status.numQuiescedSIs)
                    continue;
                if(su->status.numActiveSIs >= sg->config.maxActiveSIsPerSU)
                    continue;
                *targetSI = si;
                return CL_OK;
            }
        }
        lookAfter = NULL;
    }
    
    *targetSI = NULL;

    for ( entityRef = clAmsEntityListGetFirst(&sg->config.siList);
          entityRef != (ClAmsEntityRefT *) NULL;
          entityRef = clAmsEntityListGetNext(&sg->config.siList, entityRef) )
    {
        ClAmsSIT *si = (ClAmsSIT *)entityRef->ptr;
        ClRcT rc2;

        AMS_CHECK_SI ( si );

        /* If the caller wants us to start at a particular spot, then keep going until we find that spot
           Note that this causes an iteration over all SIs to be O(N^2) so it would be better to return the
           entityRef if this code needs to run efficiently
         */
        if (lookAfter) 
        {
            if (si == lookAfter) lookAfter= NULL;
            continue;
        }
        
        
        rc2 = clAmsPeSIIsActiveAssignable(si);

        if ( rc2 == CL_OK )
        {
            *targetSI = si;
            return CL_OK;
        }

        if ( rc2 != CL_AMS_RC(CL_AMS_ERR_SI_NOT_ASSIGNABLE) )
        {
            *targetSI = NULL;
            return rc2;
        }
    }

    return CL_AMS_RC(CL_ERR_NOT_EXIST);
}

/*
 * clAmsPeSGFindSIForStandbyAssignment
 * -----------------------------------
 * This fn figures out the next preferred SI that can be given a standby 
 * assignment. The logic for finding an SI requiring standby assignments 
 * is as follows:
 *
 * (1) It must already have an active assignment
 * (2) First give preference to an SI that has zero standby assignments
 * (3) If (2) fails, then find the SI with the largest percentage needs
 * (4) The order of selection is based on si rank.
 *
 * This fn should work for all SG models.
 */

ClRcT
clAmsPeSGFindSIForStandbyAssignment(
                                    CL_IN ClAmsSGT *sg,
                                    CL_OUT ClAmsSIT **targetSI,
                                    CL_IN ClAmsSIT **scannedSIList,
                                    CL_IN ClUint32T numScannedSIs)
{
    ClAmsEntityRefT *entityRef;
    ClInt32T pendingNeeds;
    ClInt32T nextBestNeeds = 0;

    AMS_CHECK_SG ( sg );
    AMS_CHECKPTR ( !targetSI );

    AMS_FUNC_ENTER ( ("SG [%s]\n",sg->config.entity.name.value) );

    *targetSI = NULL;

    for ( entityRef = clAmsEntityListGetFirst(&sg->config.siList);
          entityRef != (ClAmsEntityRefT *) NULL;
          entityRef = clAmsEntityListGetNext(&sg->config.siList, entityRef) )
    {
        ClAmsSIT *si = (ClAmsSIT *)entityRef->ptr;
        ClRcT rc2;

        AMS_CHECK_SI ( si );

        if(scannedSIList)
        {
            /*  
             * Check if this SI is part of the scanned SI list.
             * and skip this if its already scanned.
             */
            ClUint32T i;
            for(i = 0; i < numScannedSIs && scannedSIList[i] != si; ++i);
            if(i != numScannedSIs)
            {
                /*
                 * This was already scanned. Go to the next one.
                 */
                continue;
            }
        }

        rc2 = clAmsPeSIIsStandbyAssignable(si);

        if ( rc2 == CL_OK )
        {
            if ( !si->status.numStandbyAssignments )
            {
                *targetSI = si;
                return CL_OK;
            }
            
            pendingNeeds = si->config.numStandbyAssignments - 
                si->status.numStandbyAssignments;

            if(pendingNeeds > 0 )
            {
                pendingNeeds = ( pendingNeeds * 100 ) /
                    si->config.numStandbyAssignments;

                if ( pendingNeeds > nextBestNeeds )
                {
                    nextBestNeeds = pendingNeeds;
                    *targetSI = si;
                }
            }
        }
        else if ( rc2 != CL_AMS_RC(CL_AMS_ERR_SI_NOT_ASSIGNABLE) )
        {
            *targetSI = NULL;
            return rc2;
        }
    }

    if ( *targetSI )
    {
        return CL_OK;
    }

    return CL_AMS_RC(CL_ERR_NOT_EXIST);
}


/*
 * clAmsPeSGComputeMaxActiveSU
 * ---------------------------
 * The max active SUs in a SG is a configuration parameter. However, in
 * some cases, we may desire to vary it if the number of available SUs 
 * in the SG falls below a threshold.
 */

ClUint32T
clAmsPeSGComputeMaxActiveSU(
        CL_IN ClAmsSGT *sg)
{
    ClUint32T instantiable, maxSUs, threshold;
    ClUint32T alpha = sg->config.alpha;
    ClUint32T beta = sg->config.beta;

    AMS_FUNC_ENTER ( ("SG [%s]\n",sg->config.entity.name.value) );

    instantiable = sg->status.instantiatedSUList.numEntities   +
                   sg->status.inserviceSpareSUList.numEntities +
                   sg->status.assignedSUList.numEntities       +
                   sg->status.instantiableSUList.numEntities;

    threshold    = sg->config.numPrefActiveSUs + sg->config.numPrefStandbySUs;

    maxSUs       = ( instantiable >= threshold ) ? 
                        sg->config.numPrefActiveSUs : 
                        sg->config.numPrefActiveSUs * alpha / 100;

    /*
     *Scale down for standbys.
     */
    if(beta 
       &&
       instantiable <= sg->config.numPrefActiveSUs)
    {
        beta = sg->config.numPrefStandbySUs * beta / 100;
        beta = CL_MIN(beta, instantiable) ;
        maxSUs = instantiable - beta;
    }

    if ( !maxSUs && instantiable )
    {
        maxSUs = 1;
    }

    return maxSUs;
}

/*
 * clAmsPeSGComputeMaxStandbySU
 * ----------------------------
 * The max standby SUs in a SG is a configuration parameter. However, in
 * some cases, we may desire to vary it if the number of available SUs 
 * in the SG falls below a threshold.
 */

ClUint32T
clAmsPeSGComputeMaxStandbySU(
        CL_IN ClAmsSGT *sg)
{
    ClUint32T beta = sg->config.beta;
    ClUint32T standbySUs = sg->config.numPrefStandbySUs;
    AMS_FUNC_ENTER ( ("SG [%s]\n",sg->config.entity.name.value) );
    if(standbySUs && beta)
        standbySUs = standbySUs * beta / 100;
    return standbySUs;
}

/*
 * clAmsPeSGColocationCheckFails
 * -----------------------------
 * This function returns true if co-located SUs are not allowed for active
 * and standby assignments for an SG and the two SUs, su1 and su2 are
 * considered "colocated". Otherwise it returns false.
 *
 * Colocation may take node dependencies into account.
 */

ClUint32T
clAmsPeSGColocationCheckFails(
        CL_IN ClAmsSGT *sg,
        CL_IN ClAmsSUT *su1,
        CL_IN ClAmsSUT *su2)
{
    ClAmsNodeT *n1 = (ClAmsNodeT *) su1->config.parentNode.ptr;
    ClAmsNodeT *n2 = (ClAmsNodeT *) su2->config.parentNode.ptr;

    AMS_CHECK_NODE ( n1 );
    AMS_CHECK_NODE ( n2 );

    if ( sg->config.isCollocationAllowed == CL_TRUE )
    {
        return CL_FALSE;
    }

    // TODO add clAmsOpsEntitiesAreRelatedByAssociation(n1, n2) here

    if ( (n1 == n2) )
    {
        return CL_TRUE;
    }

    return CL_FALSE;
}

/*
 * clAmsPeSGRemoveWork
 * -------------------
 * This function removes all work assignments from all SUs that are currently
 * assigned. The SUs move into the instantiated list if their readiness state
 * is !inservice else they move to inserviceSpare list. The SUs are not
 * terminated.
 */

ClRcT
clAmsPeSGRemoveWork(
        CL_IN   ClAmsSGT *sg,
        CL_IN   ClUint32T switchoverMode)
{
    ClAmsEntityRefT *entityRef;

    AMS_CHECK_SG ( sg );

    AMS_FUNC_ENTER ( ("SG [%s]\n",sg->config.entity.name.value) );

    if ( clAmsPeSGHasAssignments(sg) )
    {
        entityRef = clAmsEntityListGetFirst(&sg->status.assignedSUList);

        while ( entityRef != (ClAmsEntityRefT *) NULL )
        {
            ClAmsSUT * su = (ClAmsSUT *) entityRef->ptr;

            /*
             * Get the next SU in the list upfront, because SUSwitchover can 
             * destroy the list.
             */

            entityRef = clAmsEntityListGetNext(&sg->status.assignedSUList, entityRef);

            /*
             * Switchover implements the reduction logic for this sg, it quiesces,
             * removes and reassigns SIs as necessary.
             */

            AMS_CALL ( clAmsPeSUSwitchoverWork(su, switchoverMode) );
        }
    }
    else
    {
        if ( sg->config.adminState == CL_AMS_ADMIN_STATE_LOCKED_A )
        {
            return clAmsPeSGLockAssignmentCallback(sg, CL_OK);
        }

        if ( sg->config.adminState == CL_AMS_ADMIN_STATE_SHUTTINGDOWN )
        {
            return clAmsPeSGShutdownCallback(sg, CL_OK);
        }
    }

    return CL_OK;
}

/*
 * clAmsPeSGUpdateSIDependents
 * ---------------------------
 * Update SI dependents
 */

ClRcT
clAmsPeSGUpdateSIDependents(
        CL_IN   ClAmsSGT *sg)
{
    ClAmsEntityRefT *entityRef;

    AMS_CHECK_SG (sg);
    
    AMS_FUNC_ENTER ( ("SG [%s]\n",sg->config.entity.name.value) );

    for ( entityRef = clAmsEntityListGetFirst(&sg->config.siList);
          entityRef != (ClAmsEntityRefT *) NULL;
          entityRef = clAmsEntityListGetNext(&sg->config.siList, entityRef) )
    {
        ClAmsSIT *si = (ClAmsSIT *) entityRef->ptr;

        AMS_CHECK_SI ( si );

        AMS_CALL ( clAmsPeSIUpdateDependents(si) );
    }

    return CL_OK;
}

/*-----------------------------------------------------------------------------
 * Useful utility functions for SG
 *---------------------------------------------------------------------------*/

/*
 * clAmsPeSGComputeAdminState
 * --------------------------
 * The computed admin state for a SG is dependent on the SG's own admin state
 * and the admin state of the application group and the cluster of which it 
 * is a member.
 */

ClAmsAdminStateT clAmsPeSGComputeAdminState(CL_IN   ClAmsSGT *sg)
{
    CL_ASSERT(AMS_ENTITY_OK (sg, CL_AMS_ENTITY_TYPE_SG));
    AMS_FUNC_ENTER ( ("SG [%s]\n",sg->config.entity.name.value) );

    // Future: Add support for application group and cluster. 
    return sg->config.adminState;
}

/*
 * clAmsPeSGUpdateReadinessState
 * -----------------------------
 * A SG does not have readiness state, but its constituent SU and Components do.
 * This fn is called to update their readiness state as a result of a change in
 * the admin state of the SG.
 */

ClRcT
clAmsPeSGUpdateReadinessState(
        CL_IN   ClAmsSGT *sg)
{
    ClAmsEntityRefT *entityRef;

    AMS_CHECK_SG (sg);

    /*
     * Update state of its constituent components
     */

    for ( entityRef = clAmsEntityListGetFirst(&sg->config.suList);
          entityRef != (ClAmsEntityRefT *) NULL;
          entityRef = clAmsEntityListGetNext(&sg->config.suList, entityRef) )
    {
        ClAmsSUT *su = (ClAmsSUT *) entityRef->ptr;

        AMS_CHECK_SU ( su );

        AMS_CALL ( clAmsPeSUUpdateReadinessState(su) );
    }

    return CL_OK;
}

/*
 * clAmsPeSGHasAssignments
 * -----------------------
 * Utility function to quickly verify if a SG is shutdown or not.
 */

ClRcT
clAmsPeSGHasAssignments(
        CL_IN ClAmsSGT *sg) 
{
    ClAmsEntityRefT *entityRef;

    AMS_CHECK_SG ( sg );

    AMS_FUNC_ENTER ( ("SG [%s]\n",sg->config.entity.name.value) );

    if ( sg->status.assignedSUList.numEntities )
    {
        return CL_TRUE;
    }

    for ( entityRef = clAmsEntityListGetFirst(&sg->config.suList);
          entityRef != (ClAmsEntityRefT *) NULL;
          entityRef = clAmsEntityListGetNext(&sg->config.suList,entityRef) )
    {
        ClAmsSUT *su = (ClAmsSUT *) entityRef->ptr;

        AMS_CHECK_SU ( su );

        if ( su->status.numActiveSIs || 
             su->status.numStandbySIs ||
             su->status.numQuiescedSIs )
        {
            return CL_TRUE;
        }
    }

    return CL_FALSE;
}

/*
 * clAmsPeSGIsInstantiated
 * -----------------------
 * Return CL_OK if SG is running/has instantiated SUs.
 */

ClRcT
clAmsPeSGIsInstantiated(
        CL_IN   ClAmsSGT *sg)
{
    AMS_CHECK_SG (sg);

    if ( sg->status.inserviceSpareSUList.numEntities || 
         sg->status.instantiatedSUList.numEntities ||
         sg->status.assignedSUList.numEntities )
    {
        return CL_TRUE;
    }

    return CL_FALSE;
}

/******************************************************************************
 * Node Functions
 *****************************************************************************/

/*-----------------------------------------------------------------------------
 * Functions called from the Management API
 *---------------------------------------------------------------------------*/

/*
 * clAmsPeNodeUnlock
 * -----------------
 * When a node is unlocked, the status of all SUs on that node must be 
 * reevaluated as per the needs of the SU's SG.
 */

ClRcT
clAmsPeNodeUnlock(
                  CL_IN       ClAmsNodeT        *node)
{
    ClAmsAdminStateT adminState;

    AMS_CHECK_NODE ( node );

    AMS_FUNC_ENTER ( ("Node [%s]\n",node->config.entity.name.value) );

    if ( !node->config.isASPAware )
    {
        AMS_ENTITY_LOG (node, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
                        ("Node [%s] is not ASP aware. Nothing to do.\n",
                         node->config.entity.name.value));
        
        return CL_AMS_RC(CL_ERR_NO_OP);
    }

    AMS_ENTITY_LOG ( node, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
                     ("Admin Operation [Unlock] on Node [%s]\n",
                      node->config.entity.name.value));

    if(clAmsInvocationsPendingForNode(node))
    {
        clLogInfo("NODE", "UNLOCK", 
                  "Node [%s] has invocations pending. Deferring unlock",
                  node->config.entity.name.value);
        return CL_AMS_RC(CL_ERR_TRY_AGAIN);
    }
    
    AMS_CALL ( clAmsPeNodeComputeAdminStateExtended(node, &adminState, CL_AMS_ADMIN_STATE_UNLOCKED) );

    if(adminState == CL_AMS_ADMIN_STATE_UNLOCKED
       &&
       node->config.isASPAware)
    {
        if(node->status.presenceState == CL_AMS_PRESENCE_STATE_INSTANTIATING
           ||
           node->status.presenceState == CL_AMS_PRESENCE_STATE_RESTARTING
           ||
           node->status.presenceState == CL_AMS_PRESENCE_STATE_TERMINATING)
        {
            clLogInfo("NODE", "UNLOCK",
                      "Node [%s] has presence state [%s]. Deferring unlock",
                      node->config.entity.name.value, 
                      CL_AMS_STRING_P_STATE(node->status.presenceState));
            return CL_AMS_RC(CL_ERR_TRY_AGAIN);
        }
    }

    CL_AMS_SET_A_STATE(node, CL_AMS_ADMIN_STATE_UNLOCKED);

    if ( adminState == CL_AMS_ADMIN_STATE_UNLOCKED )
    {
        AMS_CALL ( clAmsPeNodeEvaluateWork(node) );
    }

    /*
     * We currently do not get an async unlock callback, so we will call it 
     * here. Future, this should come from NodeEvaluateWorkCallback, which
     * needs to be called from NodeInstantiateCallback.
     */

    AMS_CALL ( clAmsPeNodeUnlockCallback(node, CL_OK) );

    return CL_OK;
}

ClRcT
clAmsPeNodeUnlockCallback(
        CL_IN       ClAmsNodeT          *node,
        CL_IN       ClUint32T           error)
{
    return CL_OK;
}

/*
 * clAmsPeNodeLockAssignment
 * -------------------------
 * When a node is in the locked assignment state all of its constituent SUs
 * are deemed to be in the locked assignment state, which means they can be
 * instantiated but cannot accept work assignments.
 */

ClRcT
clAmsPeNodeLockAssignment(
        CL_IN       ClAmsNodeT        *node)
{
    ClAmsAdminStateT adminState;
    
    AMS_CHECK_NODE ( node );

    AMS_FUNC_ENTER ( ("Node [%s]\n",node->config.entity.name.value) );

    if ( !node->config.isASPAware )
    {
        AMS_ENTITY_LOG (node, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
                 ("Node [%s] is not ASP aware. Nothing to do.\n",
                  node->config.entity.name.value));
        
        return CL_AMS_RC(CL_ERR_NO_OP);
    }

    AMS_ENTITY_LOG ( node, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("Admin Operation [Lock Assignment] on Node [%s]\n",
             node->config.entity.name.value));

    if(clAmsInvocationsPendingForNode(node))
    {
        clLogInfo("NODE", "LOCK", 
                  "Node [%s] has invocations pending. Deferring lock",
                  node->config.entity.name.value);
        return CL_AMS_RC(CL_ERR_TRY_AGAIN);
    }

    AMS_CALL ( clAmsPeNodeComputeAdminState(node, &adminState) );

    if(adminState == CL_AMS_ADMIN_STATE_LOCKED_I
       &&
       node->config.isASPAware
       &&
       node->status.presenceState != CL_AMS_PRESENCE_STATE_UNINSTANTIATED)
    {
        clLogInfo("NODE", "LOCK",
                  "Node [%s] has presence state [%s]. Deferring lock",
                  node->config.entity.name.value, 
                  CL_AMS_STRING_P_STATE(node->status.presenceState));
        return CL_AMS_RC(CL_ERR_TRY_AGAIN);
    }

    CL_AMS_SET_A_STATE(node, CL_AMS_ADMIN_STATE_LOCKED_A);

    if ( adminState == CL_AMS_ADMIN_STATE_LOCKED_I )
    {
        AMS_CALL ( clAmsPeNodeInstantiate(node) );
    }
    else if ( adminState == CL_AMS_ADMIN_STATE_UNLOCKED )
    {
        AMS_CALL ( clAmsPeNodeSwitchoverWork(node,
                        CL_AMS_ENTITY_SWITCHOVER_IMMEDIATE) );
    }

    return CL_OK;
}

/*
 * clAmsPeNodeLockAssignmentCallback
 * ---------------------------------
 * This is invoked from either node instantiate callback or node switchover
 * callback to indicate that node is now in locked assignment. Note, this
 * fn can also be invoked if a node is brought into the cluster with a 
 * default lock assignment state.
 */

ClRcT
clAmsPeNodeLockAssignmentCallback(
        CL_IN       ClAmsNodeT          *node,
        CL_IN       ClUint32T           error)
{
    AMS_CHECK_NODE ( node );

    AMS_FUNC_ENTER ( ("Node [%s]\n",node->config.entity.name.value) );

    return CL_OK;
}

/*
 * clAmsPeNodeLockInstantiation
 * ----------------------------
 * Locked instantiation is the lowest energy state for a node. It is not
 * instantiated and all SUs on the node are terminated but they stay on 
 * the instantiable SU lists of their respective SGs. 
 */

ClRcT
clAmsPeNodeLockInstantiation(
        CL_IN       ClAmsNodeT        *node)
{
    ClAmsAdminStateT adminState;

    AMS_CHECK_NODE ( node );

    AMS_FUNC_ENTER ( ("Node [%s]\n",node->config.entity.name.value) );

    if ( !node->config.isASPAware )
    {
        AMS_ENTITY_LOG (node, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
                 ("Node [%s] is not ASP aware. Nothing to do.\n",
                  node->config.entity.name.value));
        
        return CL_AMS_RC(CL_ERR_NO_OP);
    }

    AMS_ENTITY_LOG (node, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("Admin Operation [Lock Instantiation] on Node [%s]\n",
             node->config.entity.name.value));

    if(clAmsInvocationsPendingForNode(node))
    {
        clLogInfo("NODE", "LOCKI", 
                  "Node [%s] has invocations pending. Deferring lock instantiation",
                  node->config.entity.name.value);
        return CL_AMS_RC(CL_ERR_TRY_AGAIN);
    }

    AMS_CALL ( clAmsPeNodeComputeAdminState(node, &adminState) );

    CL_AMS_SET_A_STATE(node, CL_AMS_ADMIN_STATE_LOCKED_I);

    if ( adminState == CL_AMS_ADMIN_STATE_LOCKED_A )
    {
        if ( node->status.presenceState != CL_AMS_PRESENCE_STATE_UNINSTANTIATED )
        {
            AMS_CALL ( clAmsPeNodeTerminate(node) );
        }
        else
        {
            AMS_CALL ( clAmsPeNodeLockInstantiationCallback(node, CL_OK) );
        }
    }

    return CL_OK;
}

#if 1 // LGER mod for test
ClRcT
clAmsPeNodeForceLockInstantiationOperation(
        CL_IN       ClAmsNodeT        *node)
{
    ClAmsAdminStateT adminState;

    AMS_CHECK_NODE ( node );

    AMS_FUNC_ENTER ( ("Node [%s]\n",node->config.entity.name.value) );

    if ( !node->config.isASPAware )
    {
        AMS_ENTITY_LOG (node, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
                 ("Node [%s] is not ASP aware. Nothing to do.\n",
                  node->config.entity.name.value));

        return CL_AMS_RC(CL_ERR_NO_OP);
    }

    clLogNotice("NODE", "LOCK-FORCE", "Admin Operation [Forced lock instantiation trigger] on Node [%s]",
                node->config.entity.name.value);

    AMS_CALL ( clAmsPeNodeComputeAdminState(node, &adminState) );

    if(adminState == CL_AMS_ADMIN_STATE_LOCKED_I)
    {
        return CL_AMS_RC(CL_ERR_NO_OP);
    }

    CL_AMS_SET_A_STATE(node, CL_AMS_ADMIN_STATE_LOCKED_I);

    AMS_CALL ( clAmsPeNodeTerminate(node) );

    return CL_OK;
}
#endif


/*
 * clAmsPeNodeLockInstantiationCallback
 * ------------------------------------
 * This fn is invoked when a node has been terminated and the cause is
 * due to the admin state being in lock instantiation.
 */

ClRcT
clAmsPeNodeLockInstantiationCallback(
        CL_IN       ClAmsNodeT          *node,
        CL_IN       ClUint32T           error)
{
    return CL_OK;
}

/*
 * clAmsPeNodeShutdown
 * -------------------
 * A shutdown informs the AMS that all SIs assigned to SUs in the node should
 * be switched over gracefully.
 */

ClRcT
clAmsPeNodeShutdown(
        CL_IN       ClAmsNodeT        *node)
{
    ClAmsAdminStateT oldAdminState;

    AMS_CHECK_NODE ( node );

    AMS_FUNC_ENTER ( ("Node [%s]\n",node->config.entity.name.value) );

    if ( !node->config.isASPAware )
    {
        AMS_ENTITY_LOG (node, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
                 ("Node [%s] is not ASP aware. Nothing to do.\n",
                  node->config.entity.name.value));
        
        return CL_AMS_RC(CL_ERR_NO_OP);
    }

    AMS_ENTITY_LOG ( node, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("Admin Operation [Shutdown] on Node [%s]\n",
             node->config.entity.name.value));

    if(clAmsInvocationsPendingForNode(node))
    {
        clLogInfo("NODE", "SHUTDOWN", "Node [%s] has invocations pending. Deferring shutdown",
                  node->config.entity.name.value);
        return CL_AMS_RC(CL_ERR_TRY_AGAIN);
    }

    AMS_CALL ( clAmsPeNodeComputeAdminState(node, &oldAdminState) );

    CL_AMS_SET_A_STATE(node, CL_AMS_ADMIN_STATE_SHUTTINGDOWN);

    if ( oldAdminState == CL_AMS_ADMIN_STATE_UNLOCKED )
    {
        AMS_CALL ( clAmsPeNodeSwitchoverWork(node,
                        CL_AMS_ENTITY_SWITCHOVER_GRACEFUL) );
    }

    return CL_OK;
}

/*
 * clAmsPeNodeShutdownCallback
 * ---------------------------
 * This fn is called when an SU shutdown is completed. It checks to see if all
 * SUs in the node have successfully switched over. If yes, the state of the node
 * is transitioned appropriately. If the result indicates an error, a fault
 * escalation can take place.
 */

ClRcT
clAmsPeNodeShutdownCallback(
        CL_IN       ClAmsNodeT      *node,
        CL_IN       ClUint32T       result)
{

    AMS_CHECK_NODE ( node );

    AMS_FUNC_ENTER ( ("Node [%s]\n",node->config.entity.name.value) );

    if ( node->config.adminState != CL_AMS_ADMIN_STATE_SHUTTINGDOWN )
    {
        AMS_ENTITY_LOG(node, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("Shutdown callback for Node [%s] in admin state [%s] has no effect. Continuing..\n",
             node->config.entity.name.value,
             CL_AMS_STRING_A_STATE(node->config.adminState)));

        return CL_OK;
    }

    CL_AMS_SET_A_STATE(node, CL_AMS_ADMIN_STATE_LOCKED_A);

    return CL_OK;
}

/*
 * clAmsPeNodeRestart
 * ------------------
 * This fn is invoked to inform the PE that a node should be restarted.
 */

ClRcT
clAmsPeNodeRestart(
        CL_IN   ClAmsNodeT *node,
        CL_IN   ClUint32T switchoverMode)
{
    ClAmsEntityRefT *entityRef;

    AMS_CHECK_NODE ( node );

    AMS_FUNC_ENTER ( ("Node [%s]\n", node->config.entity.name.value) );
    AMS_ENTITY_LOG ( node, CL_AMS_MGMT_SUB_AREA_MSG, CL_LOG_SEV_DEBUG,("Admin Operation [Restart] on Node [%s]\n",node->config.entity.name.value));

    if ( node->status.presenceState != CL_AMS_PRESENCE_STATE_INSTANTIATED )
    {
        AMS_ENTITY_LOG (node, CL_AMS_MGMT_SUB_AREA_MSG, CL_LOG_SEV_WARNING, ("Node [%s] is not instantiated. Resetting..\n", node->config.entity.name.value) );
        clAmsPeNodeReset(node);
        return clAmsPeNodeInstantiate(node);        
    }

    if(clAmsInvocationsPendingForNode(node))
    {
        clLogInfo("NODE", "LOCK", 
                  "Node [%s] has invocations pending. Deferring restart",
                  node->config.entity.name.value);
        return CL_AMS_RC(CL_ERR_TRY_AGAIN);
    }

    /*
     * If node is not restartable, the escalated action is cluster restart,
     * but that is drastic, so just print out a message asking user to do
     * a manual cluster restart.
     */

    if ( !node->config.isRestartable )
    {
        AMS_ENTITY_LOG (node, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("Node [%s] is not restartable. Recommend manual cluster restart..\n",
             node->config.entity.name.value) ); 

        return CL_AMS_RC(CL_ERR_NO_OP);
    }

    /*
     * Treat this as a restart of all SUs on a node
     */

    if ( !node->status.numInstantiatedSUs )
    {
        return clAmsPeNodeRestartCallback(node, CL_OK);
    }

    for ( entityRef = clAmsEntityListGetFirst(&node->config.suList);
          entityRef != (ClAmsEntityRefT *) NULL;
          entityRef = clAmsEntityListGetNext(&node->config.suList,entityRef) )
    {
        ClAmsSUT *su = (ClAmsSUT *) entityRef->ptr;

        AMS_CALL ( clAmsPeSURestart(su, switchoverMode) );
    }

    return CL_OK;
}

ClRcT
clAmsPeNodeRestartCallback(
        CL_IN       ClAmsNodeT          *node,
        CL_IN       ClUint32T           error)
{
    AMS_CHECK_NODE ( node );

    AMS_FUNC_ENTER ( ("Node [%s]\n", node->config.entity.name.value) );

    return CL_OK;
}

static ClRcT
clAmsPeSUStartProbation(ClAmsEntityT *entity)
{
    ClAmsSUT *su = (ClAmsSUT*)entity;
    ClAmsSGT *sg = NULL;
    if(!su) return CL_OK;
    sg = (ClAmsSGT*)su->config.parentSG.ptr;
    if(!sg) return CL_OK;
    if(sg->config.autoAdjust && 
       (su->config.rank || sg->config.redundancyModel == CL_AMS_SG_REDUNDANCY_MODEL_CUSTOM))
    {
        clLogInfo("SG", "ADJUST", "Starting SU [%s] adjustment probation timer after node repair",
                  su->config.entity.name.value);
        AMS_CALL(clAmsEntityTimerStart(entity, CL_AMS_SU_TIMER_PROBATION));
    }
    return CL_OK;
}

/*
 * clAmsPeNodeRepaired
 * -------------------
 * Indicates that a previously failed node has been repaired (either by an
 * external administrator (human or otherwise)or by the AMS itself) and
 * the node should be put back into service.
 */

ClRcT
clAmsPeNodeRepaired(
        CL_IN       ClAmsNodeT        *node)
{
    AMS_CHECK_NODE ( node );

    AMS_FUNC_ENTER ( ("Node [%s]\n",node->config.entity.name.value) );

    if ( node->status.operState == CL_AMS_OPER_STATE_ENABLED )
    {
        AMS_ENTITY_LOG (node, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("Admin Operation [Repaired] on Node [%s] has no effect in Oper State [Enabled]. Continuing..\n",
             node->config.entity.name.value) ); 

        return CL_AMS_RC(CL_ERR_NO_OP);
    }

    AMS_ENTITY_LOG ( node, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("Admin Operation [Repaired] on Node [%s]\n",
             node->config.entity.name.value));

    clAmsPeNodeReset(node);

    /*
     * Start the auto adjust probation for all SUs in the node after the repair.
     */
    AMS_CALL ( clAmsEntityListWalkGetEntity(&node->config.suList, clAmsPeSUStartProbation ) );

    AMS_CALL ( clAmsPeNodeInstantiate(node) );

    return CL_OK;
}

/*-----------------------------------------------------------------------------
 * Functions called as a result of other external events
 *---------------------------------------------------------------------------*/

/*
 * clAmsPeNodeJoinCluster
 * ----------------------
 * This fn provides AMS with the event input that a node has joined the
 * cluster. Typically this input would come from GMS to some wrapper code
 * that verifies that the node is configured in AMS and then calls this
 * function.
 */

ClRcT clAmsPeNodeJoinCluster(CL_IN   ClAmsNodeT *node)
{
    AMS_OP_INCR(&gAms.ops);

    AMS_CHECK_NODE ( node );

    AMS_FUNC_ENTER ( ("Node [%s]\n",node->config.entity.name.value) );
    
    /*
     * This handles the case when multiple join messages are generated for 
     * the same node.
     */

    if ( node->status.isClusterMember != CL_AMS_NODE_IS_NOT_CLUSTER_MEMBER )
    {
        AMS_ENTITY_LOG (node, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("Node [%s] is a cluster member. Ignoring join request..\n",
            node->config.entity.name.value));

        return CL_OK;
    }

    AMS_ENTITY_LOG (node, CL_AMS_MGMT_SUB_AREA_MSG, CL_LOG_SEV_INFO,("Node [%s] has joined cluster\n",node->config.entity.name.value));

    /*
     * Mark node as cluster member and auto repair is set, reset
     * the state of the node, including its oper state.
     */

    node->status.isClusterMember = CL_AMS_NODE_IS_CLUSTER_MEMBER;

    CL_AMS_SET_EPOCH(node);

    if ( node->status.wasMemberBefore )
    {
        if ( node->config.autoRepair )
        {
            clAmsPeNodeReset(node);
        }
        else
        {
            if ( node->status.recovery == CL_AMS_RECOVERY_NONE )
            {
                CL_AMS_SET_O_STATE(node, CL_AMS_OPER_STATE_ENABLED);
            }
        }

#ifdef SENSELESS_SAF_COMPLIANCE

        ClAmsAdminStateT adminState;

        AMS_CALL ( clAmsPeNodeComputeAdminState(node, &adminState) );

        if ( adminState != CL_AMS_ADMIN_STATE_LOCKED_I )
        {
            CL_AMS_SET_O_STATE(node, CL_AMS_OPER_STATE_ENABLED);
        }
#endif

    }
    else
    {
        CL_AMS_SET_O_STATE(node, CL_AMS_OPER_STATE_ENABLED);
    }

    /*
     * Try to instantiate this node and let node instantiate decide if the
     * node should be instantiated.
     */

    AMS_CALL ( clAmsPeNodeInstantiate(node) );

    return CL_OK;
}

/*
 * Replay the removing non-SC invocations.
 */
static void clAmsPeReplayCSIRemove2(ClAmsNodeT *node,
                                    ClAmsInvocationT *pInvocations,
                                    ClUint32T numInvocations,
                                    ClBoolT scFailover)
{
    if(scFailover)
    {
        ClAmsCSIReplayFilterT filters[] = {
            {
                .node = NULL,
                .clearInvocation = CL_TRUE,
            },
        };
        clAmsPeReplayCSIRemoveCallbacks(pInvocations, numInvocations, filters, 1, 
                                        CL_AMS_ENTITY_SWITCHOVER_IMMEDIATE | CL_AMS_ENTITY_SWITCHOVER_REPLAY);
    }
}

static void
clAmsPeReplayCSIRemoveInvocations(ClAmsNodeT *node, ClAmsInvocationT *pInvocations, 
                                  ClUint32T numInvocations, ClBoolT scFailover)
{
    if(pInvocations)
    {
        /*
         * Run the remove callbacks for the node that has left and the node becoming active
         */
        if(scFailover)
        {
            ClNameT localNodeName = {0};
            ClAmsCSIReplayFilterT filters[] = {
                { 
                    .node = &node->config.entity.name,
                    .clearInvocation = CL_FALSE,
                },
                {
                    .node = &localNodeName,
                    .clearInvocation = CL_TRUE,
                },
            };
            clCpmLocalNodeNameGet(&localNodeName);
            clAmsPeReplayCSIRemoveCallbacks(pInvocations, 
                                            numInvocations, 
                                            filters,
                                            2,
                                            CL_AMS_ENTITY_SWITCHOVER_FAST | CL_AMS_ENTITY_SWITCHOVER_REPLAY
                                            );
            clAmsPeReplayCSIRemove2(node, pInvocations, numInvocations, scFailover);
        }
        else
        {
            clAmsPeReplayCSIRemoveCallbacks(pInvocations, numInvocations, NULL, 0,
                                            CL_AMS_ENTITY_SWITCHOVER_FAST | CL_AMS_ENTITY_SWITCHOVER_REPLAY);
        }
    }
}

/*
 * clAmsPeNodeHasLeftCluster
 * -------------------------
 * This fn provides AMS with event input that a node has left the cluster. 
 * Note, this is a post event notification, node has already left and AMS
 * must now take recovery actions.
 */

ClRcT
clAmsPeNodeHasLeftCluster(
                          CL_IN   ClAmsNodeT *node,
                          CL_IN   ClBoolT scFailover)
{
    ClRcT rc = CL_OK;
    ClNameT *csiRemoveReplayNode = NULL;
    ClAmsInvocationT *pInvocations = NULL;
    ClInt32T numInvocations = 0;
    ClUint32T switchoverMode = CL_AMS_ENTITY_SWITCHOVER_FAST;

    AMS_OP_INCR(&gAms.ops);

    AMS_CHECK_NODE ( node );

    AMS_FUNC_ENTER ( ("Node [%s]\n",node->config.entity.name.value) );

    if(!scFailover)
        csiRemoveReplayNode = &node->config.entity.name;

    /*
     * Get pending CSI remove invocations for this node
     */
    clAmsGetCSIRemoveInvocations(csiRemoveReplayNode,
                                 &pInvocations,
                                 &numInvocations);

    /*
     * This handles the case when multiple HasLeftCluster messages are
     * generated for the same node as well as when a HasLeftCluster message
     * is generated for a node for which a previous IsLeavingCluster message
     * was generated.
     */

    if ( node->status.isClusterMember == CL_AMS_NODE_IS_NOT_CLUSTER_MEMBER 
         && 
         node->status.presenceState == CL_AMS_PRESENCE_STATE_UNINSTANTIATED)
    {
        AMS_ENTITY_LOG (node, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
                        ("Node [%s] is not a cluster member. Ignoring 'node has left' request..\n",
                         node->config.entity.name.value));
        /*
         * Replay any pending assign CSI invocation for the node becoming active
         */
        clAmsPeReplayCSIRemoveInvocations(node, pInvocations, numInvocations, scFailover);
        if(scFailover)
            clAmsReplayAssignCSIInvocation(&scFailover);
        if(pInvocations) clHeapFree(pInvocations);
        return CL_AMS_RC(CL_ERR_NOT_EXIST);
    }

    AMS_ENTITY_LOG (node, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
                    ("Node [%s] has left cluster: Step 1: Failing over all SUs on Node\n",
                     node->config.entity.name.value));

    CL_AMS_RESET_EPOCH(node);

    node->status.isClusterMember = CL_AMS_NODE_IS_NOT_CLUSTER_MEMBER;
    node->status.wasMemberBefore = CL_TRUE;

#ifdef SENSELESS_SAF_COMPLIANCE

    ClAmsAdminStateT adminState;

    AMS_CALL ( clAmsPeNodeComputeAdminState(node, &adminState) );

    if ( adminState != CL_AMS_ADMIN_STATE_LOCKED_I )
    {
        CL_AMS_SET_O_STATE(node, CL_AMS_OPER_STATE_DISABLED);
    }

#else

    CL_AMS_SET_O_STATE(node, CL_AMS_OPER_STATE_DISABLED);

#endif

    /*
     * Replay the remove CSI pending invocations first for the node going out
     * and the SC becoming active
     */
    clAmsPeReplayCSIRemoveInvocations(node, pInvocations, numInvocations, scFailover);
    if(pInvocations) 
        clHeapFree(pInvocations);

    /*
     * First replay any pending CSI sets. from the invocation
     */
    if(scFailover)
        clAmsReplayAssignCSIInvocation(&scFailover);

    /*
     * Stop all possible timers that could be running for this node _and_
     * its SUs. Clear the invocation list for these components as we won't
     * be getting any callbacks from these components.
     */

    rc = clAmsEntityClearOps((ClAmsEntityT *)node);
    
    if(rc != CL_OK)
    {
        AMS_LOG(CL_DEBUG_ERROR, ("AMS entity clear failed for node [%.*s] " \
                                 "with rc [%#x]\n", 
                                 node->config.entity.name.length,
                                 node->config.entity.name.value, rc));
        return rc;

    }

    if(scFailover)
        switchoverMode |= CL_AMS_ENTITY_SWITCHOVER_CONTROLLER;

    AMS_CHECK_RC_ERROR ( clAmsPeNodeSwitchoverWork(node, 
                                                   switchoverMode) );
    
    exitfn:
    return CL_OK;
}

ClRcT
clAmsPeNodeHasLeftClusterCallback_Step1(
        CL_IN   ClAmsNodeT *node,
        CL_IN   ClRcT error)
{
    AMS_CHECK_NODE ( node );

    AMS_FUNC_ENTER ( ("Node [%s]\n",node->config.entity.name.value) );

    AMS_ENTITY_LOG (node, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
        ("Node [%s] has left cluster: Step 2: Marking SUs on Node as terminated\n",
         node->config.entity.name.value) ); 

    if ( node->status.presenceState != CL_AMS_PRESENCE_STATE_UNINSTANTIATED )
    {
        AMS_CALL ( clAmsPeNodeTerminate(node) );
    }
    else
    {
        AMS_CALL ( clAmsPeNodeHasLeftClusterCallback_Step2(node, CL_OK) );
    }

    return CL_OK;
}

ClRcT
clAmsPeNodeHasLeftClusterCallback_Step2(
        CL_IN   ClAmsNodeT *node,
        CL_IN   ClRcT error)
{
    AMS_CHECK_NODE ( node );

    AMS_FUNC_ENTER ( ("Node [%s]\n",node->config.entity.name.value) );

    AMS_ENTITY_LOG (node, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
        ("Node [%s] has left cluster: Step 3: Informing CPM of task completion\n",
         node->config.entity.name.value) ); 

#ifdef AMS_CPM_INTEGRATION
    return _clAmsSANodeLeaveCompleted(&node->config.entity.name, CL_CPM_NODE_LEFT);
#endif

    return CL_OK;
}

/*
 * clAmsPeNodeIsLeavingCluster
 * ---------------------------
 * This fn provides AMS with event input that a node is interested in
 * leaving the cluster gracefully.  
 */

ClRcT
clAmsPeNodeIsLeavingCluster(
                            CL_IN   ClAmsNodeT *node,
                            CL_IN   ClBoolT scFailover)
{
    ClAmsInvocationT *pInvocations = NULL;
    ClInt32T numInvocations = 0;
    ClRcT rc = CL_OK;

    AMS_OP_INCR(&gAms.ops);

    AMS_CHECK_NODE ( node );

    AMS_FUNC_ENTER ( ("Node [%s]\n",node->config.entity.name.value) );
 
    /*
     * This handles the case when multiple HasLeftCluster messages are
     * generated for the same node.
     */

    if ( node->status.isClusterMember != CL_AMS_NODE_IS_CLUSTER_MEMBER )
    {
        AMS_ENTITY_LOG (node, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("Node [%s] is not a cluster member. Ignoring 'node is leaving' request..\n",
            node->config.entity.name.value));
        return CL_AMS_RC(CL_ERR_NOT_EXIST);
    }

    AMS_ENTITY_LOG (node, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
             ("Node [%s] is leaving cluster: Step 1: Switching over all SUs on Node\n",
              node->config.entity.name.value));

    CL_AMS_RESET_EPOCH(node);

    node->status.isClusterMember = CL_AMS_NODE_IS_LEAVING_CLUSTER;
    node->status.wasMemberBefore = CL_TRUE;

    /*
     * Get pending CSI remove invocations for this node and replay first
     */
    clAmsGetCSIRemoveInvocations(&node->config.entity.name,
                                 &pInvocations,
                                 &numInvocations);

    clAmsPeReplayCSIRemoveInvocations(node, pInvocations, numInvocations, CL_FALSE);

    if(pInvocations)
        clHeapFree(pInvocations);

    /*
     * Then replay any pending CSI sets. from the invocation
     */
    if(scFailover)
        clAmsReplayAssignCSIInvocation(NULL);
    
    /*
     * Stop all possible timers that could be running for this node _and_
     * its SUs. Clear the invocation list for these components as we won't
     * be getting any callbacks from these components.
     */
    
    rc = clAmsEntityClearOps((ClAmsEntityT *)node);

    if(rc != CL_OK)
    {
        AMS_LOG(CL_DEBUG_ERROR, ("AMS entity clear failed for node [%.*s] "\
                                 "with rc [%#x]\n", 
                                 node->config.entity.name.length,
                                 node->config.entity.name.value, rc));
        return rc;
    }

    AMS_CALL ( clAmsPeNodeSwitchoverWork(node, CL_AMS_ENTITY_SWITCHOVER_IMMEDIATE) );

    return CL_OK;
}

/*
 * clAmsPeNodeIsLeavingClusterCallback_Step*
 * -----------------------------------------
 * This fn is invoked when the termination of a node is complete and it
 * informs interested parties via the event API that AMS is now okay
 * with the node actually leaving the cluster.
 */

ClRcT
clAmsPeNodeIsLeavingClusterCallback_Step1(
        CL_IN   ClAmsNodeT *node,
        CL_IN   ClRcT error)
{
    AMS_CHECK_NODE ( node );

    AMS_FUNC_ENTER ( ("Node [%s]\n",node->config.entity.name.value) );

    AMS_ENTITY_LOG (node, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
        ("Node [%s] is leaving cluster: Step 2: Terminating all SUs on Node\n",
         node->config.entity.name.value) ); 

    if ( node->status.presenceState != CL_AMS_PRESENCE_STATE_UNINSTANTIATED )
    {
        AMS_CALL ( clAmsPeNodeTerminate(node) );
    }
    else
    {
        AMS_CALL ( clAmsPeNodeIsLeavingClusterCallback_Step2(node, CL_OK) );
    }

    return CL_OK;
}

ClRcT
clAmsPeNodeIsLeavingClusterCallback_Step2(
        CL_IN   ClAmsNodeT *node,
        CL_IN   ClRcT error)
{
    AMS_CHECK_NODE ( node );

    AMS_FUNC_ENTER ( ("Node [%s]\n",node->config.entity.name.value) );

    AMS_ENTITY_LOG (node, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
        ("Node [%s] is leaving cluster: Step 3: Informing CPM of task completion\n",
         node->config.entity.name.value) ); 

#ifdef SENSELESS_SAF_COMPLIANCE

    ClAmsAdminStateT adminState;

    AMS_CALL ( clAmsPeNodeComputeAdminState(node, &adminState) );

    if ( adminState != CL_AMS_ADMIN_STATE_LOCKED_I )
    {
        CL_AMS_SET_O_STATE(node, CL_AMS_OPER_STATE_DISABLED);
    }

#else

    CL_AMS_SET_O_STATE(node, CL_AMS_OPER_STATE_DISABLED);

#endif

    if ( node->status.isClusterMember != CL_AMS_NODE_IS_NOT_CLUSTER_MEMBER )
    {
        node->status.isClusterMember = CL_AMS_NODE_IS_NOT_CLUSTER_MEMBER ;

#ifdef AMS_CPM_INTEGRATION
        return _clAmsSANodeLeaveCompleted(&node->config.entity.name, CL_CPM_NODE_LEAVING);
#endif
    }

    return CL_OK;
}

/*
 * clAmsPeNodeColdplugJoinCluster
 * ------------------------------
 * This fn is called at cluster start time only and it checks if a node is
 * already a member of the cluster and then simulates a join event.
 */

ClRcT
clAmsPeNodeColdplugJoinCluster(
        CL_IN   ClAmsNodeT *node)
{
    AMS_CHECK_NODE ( node );

    AMS_FUNC_ENTER ( ("Node [%s]\n",node->config.entity.name.value) );

    if ( !node->config.isSwappable || 
         (node->status.isClusterMember == CL_AMS_NODE_IS_CLUSTER_MEMBER) )
    {
        AMS_CALL ( clAmsPeNodeJoinCluster(node) );
    }

    return CL_OK;
}

/*
 * clAmsPeNodeFaultReportProcess
 * -----------------------------
 * This fn is provides functionality used during both error reporting
 * on the node and gracefully/ungracefully restarting the node.
 */

ClRcT
clAmsPeNodeFaultReportProcess(
        CL_IN       ClAmsNodeT                  *node,
        CL_IN       ClAmsCompT                  *faultyComp,
        CL_INOUT    ClAmsLocalRecoveryT         *recovery,
        CL_OUT      ClUint32T                   *escalation)
{

    AMS_CHECKPTR( !recovery || !escalation );
    AMS_CHECK_NODE ( node );

    /*
     * Stop all possible timers that could be running for this node _and_
     * its SUs. Clear the invocation list for these components as we won't
     * be getting any callbacks from these components.
     */

    AMS_CALL ( clAmsEntityClearOps((ClAmsEntityT *)node) );

    clAmsEntityOpsClear((ClAmsEntityT*)node, &node->status.entity);

    if(!node->config.isASPAware)
    {
        *recovery = CL_AMS_RECOVERY_NODE_FAILFAST;
    }

    switch ( *recovery )
    {
        case CL_AMS_RECOVERY_NODE_SWITCHOVER:
        {
            node->status.isClusterMember = CL_AMS_NODE_IS_LEAVING_CLUSTER;

            AMS_CALL ( clAmsPeNodeSwitchoverWork(node,
                            CL_AMS_ENTITY_SWITCHOVER_IMMEDIATE) );

            break;
        }

        case CL_AMS_RECOVERY_NODE_FAILOVER:
        {
            ClBoolT recovery = CL_FALSE;
            node->status.isClusterMember = CL_AMS_NODE_IS_LEAVING_CLUSTER;
            CL_AMS_SET_O_STATE(node, CL_AMS_OPER_STATE_DISABLED);
            /*
             * Check for failover history recover
             */
            clAmsSGFailoverHistoryCascade(node, &faultyComp, &recovery);
            if(faultyComp && recovery == CL_TRUE)
            {
                node->status.recovery = CL_AMS_RECOVERY_NODE_HALT;
                if(clAmsPeSGFailoverHistoryRecover(node, faultyComp) != CL_OK)
                    recovery = CL_FALSE;
            }
            AMS_CALL ( clAmsPeNodeSwitchoverWork(node,
                            CL_AMS_ENTITY_SWITCHOVER_IMMEDIATE) );

            break;
        }
        
        case CL_AMS_RECOVERY_NODE_FAILFAST:
        {
            ClRcT rc = CL_OK;
            ClBoolT recovery = CL_FALSE;
            clAmsSGFailoverHistoryCascade(node, &faultyComp, &recovery);
            if(faultyComp && recovery == CL_TRUE)
            {
                node->status.recovery = CL_AMS_RECOVERY_NODE_HALT;
                if(clAmsPeSGFailoverHistoryRecover(node, faultyComp) != CL_OK)
                    recovery = CL_FALSE;
            }
            /*
             * Reset the node and node leave processing would take care
             * of the rest
             */
            if(!node->config.isASPAware)
            {
                node->status.isClusterMember = CL_AMS_NODE_IS_NOT_CLUSTER_MEMBER;
                CL_AMS_SET_O_STATE(node, CL_AMS_OPER_STATE_DISABLED);
                AMS_CALL ( clAmsPeNodeSwitchoverWork(node, CL_AMS_ENTITY_SWITCHOVER_IMMEDIATE) );
            }

            if( !recovery && 
                (rc = _clAmsSANodeFailFast(&node->config.entity.name,
                                           node->config.isASPAware) ) != CL_OK)
            {
                clLogError("FAULT", "FAILFAST", "Resetting node [%s] failed with [%#x]",
                           node->config.entity.name.value, rc);
            }
            
            if((!node->config.isASPAware) && node->config.autoRepair)
            {
                clAmsPeNodeReset(node);
            }
            break;
        }

        case CL_AMS_RECOVERY_CLUSTER_RESET:
        {
            AMS_CALL ( clAmsPeClusterFaultReport(&gAms, recovery, escalation) );

            break;
        }

        default:
        {
            return CL_AMS_RC(CL_ERR_NOT_IMPLEMENTED);
        }
    }

    return CL_OK;
}

/*
 * clAmsPeNodeFaultReport
 * ----------------------
 * This fn is invoked to inform AMS of a fault at a particular node. In response,
 * AMS does a switchover of SIs assigned to SUs on that node if standbys are
 * available, and then terminates the SUs on the node. If standbys are not
 * available, the SIs become unassigned. The fault may also be escalated to
 * cluster reset.
 *
 * This fn works for non-ASP nodes as SU count for these nodes should be 0.
 */

ClRcT
clAmsPeNodeFaultReport(
        CL_IN       ClAmsNodeT                  *node,
        CL_IN       ClAmsCompT                  *faultyComp,
        CL_INOUT    ClAmsLocalRecoveryT         *recovery,
        CL_OUT      ClUint32T                   *escalation)
{
    ClAmsLocalRecoveryT recommendedRecovery;

    AMS_CHECKPTR( !recovery || !escalation );
    AMS_CHECK_NODE ( node );

    AMS_FUNC_ENTER ( ("Node [%s]\n",node->config.entity.name.value) );

    /*
     * Compute and apply recovery action
     */

    recommendedRecovery = *recovery;

    AMS_CALL ( clAmsPeNodeComputeRecoveryAction(node, recovery, escalation) );

    if ( *recovery == CL_AMS_RECOVERY_NONE )
    {
        clLogDebug(CL_LOG_AREA_AMS, CL_LOG_CONTEXT_AMS_FAULT_NODE,
                   "Fault on Node [%s]: Recommended recovery = [%s], "\
                   "Computed recovery = [%s]. Ignoring fault",
                   node->config.entity.name.value,
                   CL_AMS_STRING_RECOVERY(recommendedRecovery),
                   CL_AMS_STRING_RECOVERY(*recovery));

        return CL_OK;
    }

    clLogDebug(CL_LOG_AREA_AMS, CL_LOG_CONTEXT_AMS_FAULT_NODE,
               "Fault on Node [%s]: Recommended recovery = [%s], "\
               "Computed recovery = [%s], Escalation = [%s]",
               node->config.entity.name.value,
               CL_AMS_STRING_RECOVERY(recommendedRecovery),
               CL_AMS_STRING_RECOVERY(*recovery),
               *escalation ? "True" : "False");

    return clAmsPeNodeFaultReportProcess(node, faultyComp, recovery, escalation);
}

/*
 * clAmsPeNodeFaultCallback_Step*
 * ------------------------------
 * These fns are internally invoked by AMS when the actions for a fault report
 * have completed.
 */

ClRcT
clAmsPeNodeFaultCallback_Step1(
        CL_IN       ClAmsNodeT          *node,
        CL_IN       ClUint32T           error)
{
    AMS_CHECK_NODE ( node );

    AMS_FUNC_ENTER ( ("Node [%s]\n",node->config.entity.name.value) );

    AMS_ENTITY_LOG (node, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
        ("Fault on Node [%s]: Terminating all SUs on Node\n",
         node->config.entity.name.value) ); 

    if ( error )
    {
        return clAmsPeNodeFaultCallback_Step2(node, error);
    }

    if ( node->status.presenceState != CL_AMS_PRESENCE_STATE_UNINSTANTIATED )
    {
        AMS_CALL ( clAmsPeNodeTerminate(node) );
    }
    else
    {
        AMS_CALL ( clAmsPeNodeFaultCallback_Step2(node, CL_OK) );
    }

    return CL_OK;
}

ClRcT
clAmsPeNodeFaultCallback_Step2(
        CL_IN       ClAmsNodeT          *node,
        CL_IN       ClUint32T           error)
{
    ClBoolT repairNecessary;
    ClAmsRecoveryT recovery;

    AMS_CHECK_NODE ( node );

    AMS_FUNC_ENTER ( ("Node [%s]\n",node->config.entity.name.value) );

    CL_AMS_SET_O_STATE(node, CL_AMS_OPER_STATE_DISABLED);

    node->status.isClusterMember = CL_AMS_NODE_IS_NOT_CLUSTER_MEMBER;
    node->status.wasMemberBefore = CL_TRUE;

    /*
     * The recovery action is now completed. 
     */

    repairNecessary = CL_TRUE;
    recovery = node->status.recovery;

    if ( error )
    {
        AMS_ENTITY_LOG(node, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("Fault on Node [%s]: Recovery [%s] Failed\n",
             node->config.entity.name.value,
             CL_AMS_STRING_RECOVERY(node->status.recovery)));
    }
    else
    {
        AMS_ENTITY_LOG(node, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("Fault on Node [%s]: Recovery [%s] Successful\n",
             node->config.entity.name.value,
             CL_AMS_STRING_RECOVERY(node->status.recovery)));

        if ( (node->status.recovery == CL_AMS_RECOVERY_NODE_FAILFAST) ||
             (node->status.recovery == CL_AMS_RECOVERY_NODE_FAILOVER) ||
             (node->status.recovery == CL_AMS_RECOVERY_NODE_HALT)     ||
             ((node->status.recovery == CL_AMS_RECOVERY_NODE_SWITCHOVER) && 
              (node->config.autoRepair == CL_TRUE)) )
        {
            repairNecessary = CL_FALSE;

            node->status.recovery = CL_AMS_RECOVERY_NONE;
        }
    }

#ifdef AMS_CPM_INTEGRATION

    ClRcT rc = CL_OK;

    AMS_ENTITY_LOG(node, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
        ("Fault on Node [%s]: Reporting fault to FM. Recovery [%s], Repair Necessary [%s]\n",
         node->config.entity.name.value,
         CL_AMS_STRING_RECOVERY(recovery),
         CL_AMS_STRING_BOOLEAN(repairNecessary)));

    if ( (rc = _clAmsSAFaultReportCallback(
                        (ClAmsEntityT *)node,
                        recovery,
                        repairNecessary)) != CL_OK )
    { 
        AMS_ENTITY_LOG(node, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_WARN,
            ("Error: Fault on Node [%s]: Error [0x%x] when reporting fault to FM. Continuing..\n", 
             node->config.entity.name.value,
             rc));
    }
#if 0
    if ( recovery == CL_AMS_RECOVERY_NODE_FAILFAST )
    { 
        if ( ( rc = _clAmsSANodeFailFast(
                                         &node->config.entity.name,
                                         node->config.isASPAware))
                != CL_OK )
        {
            AMS_ENTITY_LOG(node, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_ERROR,
                    ("Error: Fault on Node [%s]: Error [0x%x] when rebooting node\n",
                     node->config.entity.name.value,
                     rc));
        }
    }
#endif

    if ( recovery == CL_AMS_RECOVERY_NODE_SWITCHOVER
         ||
         recovery == CL_AMS_RECOVERY_NODE_FAILOVER)
    {
        if(node->config.autoRepair == CL_TRUE)
        { 
            /*
             * Reboot the node.
             */
            if ( ( rc = _clAmsSANodeFailOverRestart(
                                                    &node->config.entity.name,
                                                    node->config.isASPAware))
                 != CL_OK )
            {
                AMS_ENTITY_LOG(node, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_ERROR,
                               ("Error: Fault on Node [%s]: Error [0x%x] when reporting Node Failover to CPM\n", node->config.entity.name.value, rc));
            }
        }
        else
        {
            if( (rc = _clAmsSANodeFailOver(&node->config.entity.name, 
                                           node->config.isASPAware) ) != CL_OK)
            {
                AMS_ENTITY_LOG(node, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_ERROR,
                               ("Error: Fault on Node [%s]: Error [%#x] reporting node failover to CPM\n",
                                node->config.entity.name.value, rc));
            }
        }
    }

#endif

    return CL_OK;
}

/*-----------------------------------------------------------------------------
 * Functions called as a result of AMS actions
 *---------------------------------------------------------------------------*/

/*
 * clAmsPeNodeInstantiate
 * ----------------------
 * This function checks if a node is usable by the AMS and if it is, makes
 * its SUs available for use. 
 *
 * Note, adminState of node does not affect whether the SUs on a node are
 * instantiable, it only implicitly affects assignment. Hence, the admin
 * state is not explicitly considered below, it will be implicitly taken
 * into account during assignment.
 *
 * Also note that NodeInstantiate is a recursive function that calls itself
 * to instantiate dependent nodes.
 */

ClRcT
clAmsPeNodeInstantiate(
        CL_IN       ClAmsNodeT        *node)
{
    AMS_CHECK_NODE ( node );

    AMS_FUNC_ENTER ( ("Node [%s]\n",node->config.entity.name.value) );

    /*
     * Are we reinstantiating the node?
     */

    if ( node->status.presenceState != CL_AMS_PRESENCE_STATE_UNINSTANTIATED )
    {
        AMS_ENTITY_LOG (node, CL_AMS_MGMT_SUB_AREA_MSG, CL_LOG_SEV_WARNING,
            ("Node [%s] is in Presence State [%s]. Ignoring instantiate request..\n",
             node->config.entity.name.value,
             CL_AMS_STRING_P_STATE(node->status.presenceState)) ); 

        return CL_OK;
    }

    /*
     * Check if this node is instantiable. This also checks that the dependencies 
     * of the node are instantiable.
     */

    if ( clAmsPeNodeIsInstantiable(node) != CL_OK ) 
    {
        AMS_ENTITY_LOG (node, CL_AMS_MGMT_SUB_AREA_MSG, CL_LOG_SEV_WARNING,
            ("Node [%s] is not instantiable. Ignoring instantiate request..\n",
            node->config.entity.name.value));

        return CL_OK;
    }

    /*
     * Mark all SU on this node as instantiable, ie available to SG for 
     * instantiation.
     */

    AMS_ENTITY_LOG (node, CL_AMS_MGMT_SUB_AREA_MSG,  CL_LOG_SEV_INFO,
             ("Instantiating Node [%s]. Marking SUs on node as instantiable\n",
              node->config.entity.name.value));

    node->status.numInstantiatedSUs = 0;

    AMS_CALL ( clAmsEntityListWalkGetEntity(
                    &node->config.suList,
                    (ClAmsEntityCallbackT)clAmsPeSUMarkInstantiable) );


    /*
     * We directly set the Presence State of node to instantiated, not going
     * through an intermediate instantiating state.
     */

    CL_AMS_SET_P_STATE(node, CL_AMS_PRESENCE_STATE_INSTANTIATED);

    /*
     * Evaluate the work needs of the node, which translates to evaluating 
     * the work needs of the SUs in the node which are now marked as 
     * instantiable. SUs in the instantiable list will get instantiated only 
     * if needed as per the rules of the SG.
     */

    AMS_CALL ( clAmsPeNodeEvaluateWork(node) );

    /*
     * Finally, inform the dependents of this node that it is now instantiated.
     */

    AMS_CALL ( clAmsEntityListWalkGetEntity(
                    &node->config.nodeDependentsList, 
                    (ClAmsEntityCallbackT) clAmsPeNodeInstantiate) );
 
    return CL_OK;
}

/*
 * clAmsPeNodeInstantiateCallback
 * ------------------------------
 * This fn is called whenever a SU instantiate callback takes place
 * or when a SU instantiate timeout takes place.  
 *
 * Note that SUs on a node will only be instantiated if they are
 * required by their respective SGs, so this call could come back
 * at any time, not necessarily when node is instantiated.
 */
 
ClRcT
clAmsPeNodeInstantiateCallback(
        CL_IN       ClAmsNodeT        *node,
        CL_IN       ClRcT             error)
{
    AMS_CHECK_NODE ( node );

    AMS_FUNC_ENTER ( ("Node [%s]\n",node->config.entity.name.value) );

    if ( error )
    {
        // nothing for now, fault escalation could be added in future

        return CL_OK;
    }

    AMS_ENTITY_LOG (node, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
        ("Node [%s] now has [%d] SUs instantiated\n",
        node->config.entity.name.value,
        node->status.numInstantiatedSUs));

    if ( node->config.adminState == CL_AMS_ADMIN_STATE_LOCKED_A )
    {
        AMS_CALL ( clAmsPeNodeLockAssignmentCallback(node, error) );
    }

    return CL_OK;
}

/*
 * clAmsPeNodeTerminate
 * --------------------
 * This fn is called to do the necessary cleanup when a node is terminated. This
 * terminates all SUs on the node and then removes them from the instantiable list 
 * of their respective SGs.
 *
 * Also note that NodeTerminate is a recursive function that indirectly calls 
 * itself, to terminate dependent nodes.
 */

ClRcT
clAmsPeNodeTerminate(
        CL_IN   ClAmsNodeT *node)
{
    ClAmsEntityRefT *entityRef;
    ClBoolT docleanup = CL_FALSE;
    ClUint32T numInstantiatedSUs = 0;

    AMS_CHECK_NODE ( node );

    AMS_FUNC_ENTER ( ("Node [%s]\n",node->config.entity.name.value) );

    if ( node->status.presenceState == CL_AMS_PRESENCE_STATE_UNINSTANTIATED )
    {
        return CL_OK;
    }

    /*
     * Set presence state of node. The oper state is left unchanged, if there
     * is an external cause for termination, it will set the oper state.
     */

    AMS_ENTITY_LOG (node, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE, 
            ("Terminating Node [%s], Presence State [%s]\n", 
             node->config.entity.name.value, 
             CL_AMS_STRING_P_STATE(node->status.presenceState)));

    if ( node->status.presenceState == CL_AMS_PRESENCE_STATE_TERMINATING )
    {
        AMS_ENTITY_LOG (node, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("Node [%s] is already in Presence State [%s]. Will cleanup..\n",
            node->config.entity.name.value,
            CL_AMS_STRING_P_STATE(node->status.presenceState)));
        docleanup = CL_TRUE;
    }

    if ( (node->status.recovery == CL_AMS_RECOVERY_NODE_FAILOVER) 
         ||
         (node->status.recovery == CL_AMS_RECOVERY_NODE_FAILFAST)
         ||
         (node->status.recovery == CL_AMS_RECOVERY_NODE_HALT))
    {
        AMS_ENTITY_LOG (node, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("Node [%s] has fault [%s]. Will cleanup..\n",
            node->config.entity.name.value,
            CL_AMS_STRING_RECOVERY(node->status.recovery)));

        docleanup = CL_TRUE;
    }

    CL_AMS_SET_P_STATE(node, CL_AMS_PRESENCE_STATE_TERMINATING);

    /*
     * First tell this node's dependents that it is terminating.
     */

    for ( entityRef = clAmsEntityListGetFirst(&node->config.nodeDependentsList);
          entityRef != (ClAmsEntityRefT *) NULL;
          entityRef = clAmsEntityListGetNext(&node->config.nodeDependentsList,
                                             entityRef) )
    {
        ClAmsNodeT *n = (ClAmsNodeT *) entityRef->ptr;

        // TODO If dependency is due to containment then should not try
        // to talk to SUs.

        AMS_CALL ( clAmsPeNodeTerminate(n) );
    }

    numInstantiatedSUs = node->status.numInstantiatedSUs;

    /*
     * If there were no instantiated SUs, check for instantiating SUs 
     * which are also supposed to be cleaned up.
     */
    if(!numInstantiatedSUs)
    {
        ClAmsEntityRefT *entityRef = NULL;

        for ( entityRef = clAmsEntityListGetFirst(&node->config.suList);
              entityRef != (ClAmsEntityRefT *) NULL;
              entityRef = clAmsEntityListGetNext(&node->config.suList,entityRef) )
        {
            ClAmsSUT *su = (ClAmsSUT *) entityRef->ptr;

            AMS_CHECK_SU ( su );
            
            if(su->status.presenceState == CL_AMS_PRESENCE_STATE_INSTANTIATING)
            {
                ++numInstantiatedSUs;
            }
        }
    }

    /*
     * Then terminate the SUs on this node.
     */

    if(numInstantiatedSUs)
    {
        if ( docleanup )
        {
            AMS_CALL ( clAmsEntityListWalkGetEntity(
                            &node->config.suList,
                            (ClAmsEntityCallbackT) clAmsPeSUCleanup) );
        }
        else
        {
            AMS_CALL ( clAmsEntityListWalkGetEntity(
                            &node->config.suList,
                            (ClAmsEntityCallbackT) clAmsPeSUTerminate) );
        }
    }
    else
    {
        AMS_CALL ( clAmsPeNodeTerminateCallback(node, CL_OK) );
    }

    return CL_OK;
}

/*
 * clAmsPeNodeTerminateCallback
 * ----------------------------
 * This fn is called whenever a SU terminate callback takes place
 * or when a SU instantiate timeout takes place.
 */
 
ClRcT
clAmsPeNodeTerminateCallback(
        CL_IN   ClAmsNodeT *node,
        CL_IN   ClRcT error)
{
    ClRcT nerror = CL_OK;

    AMS_CHECK_NODE ( node );

    AMS_FUNC_ENTER ( ("Node [%s]\n",node->config.entity.name.value) );

    /*
     * Prechecks
     */

    if ( node->status.presenceState == CL_AMS_PRESENCE_STATE_UNINSTANTIATED )
    {
        goto terminated;
    }

    // XXX We currently don't check the error code, thus, we do not tranfer
    // SU termination errors to node. This could be changed in the future
    // by marking the node as termination-failed if there is an error
    // and then changing the if condition below to allow for it, as well
    // as changing the if conditions in su terminate callback and su cleanup
    // callback to allow further calls to node terminate, even if node state
    // is termination failed. If the node is marked as termination-failed
    // then the node's oper state should be changed to disabled and an
    // explicit node repaired will be required to bring the node back into
    // service.

    if ( node->status.presenceState != CL_AMS_PRESENCE_STATE_TERMINATING )
    {
        AMS_ENTITY_LOG (node, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
            ("Node [%s] is in Presence State [%s]. Ignoring terminate callback..\n",
            node->config.entity.name.value,
            CL_AMS_STRING_P_STATE(node->status.presenceState)) );

        return CL_OK;
    }

    if ( node->status.numInstantiatedSUs )
    {
        AMS_ENTITY_LOG (node, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
            ("Node [%s] has [%d] instantiated SUs. Ignoring terminate callback..\n",
            node->config.entity.name.value,
            node->status.numInstantiatedSUs));

        return CL_OK;
    }

    if ( node->status.presenceState == CL_AMS_PRESENCE_STATE_TERMINATING )
    {
        CL_AMS_SET_P_STATE(node, CL_AMS_PRESENCE_STATE_UNINSTANTIATED);
    }

    if ( node->status.presenceState == CL_AMS_PRESENCE_STATE_TERMINATION_FAILED )
    {
        nerror = CL_AMS_RC(!CL_OK); // XXX for now, change to a valid error
    }

    /*
     * All SUs have terminated and should have been marked as uninstantiable
     * in clAmsPeSUTerminate. But we repeat the logic here for safety and do
     * so silently, so confusing errors are not printed when a SU is not
     * found in the SGs instantiable list.
     */

    AMS_CALL ( clAmsEntityListWalkGetEntity(
                    &node->config.suList,
                    (ClAmsEntityCallbackT)clAmsPeSUMarkUninstantiable) );

    AMS_ENTITY_LOG (node, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
        ("Node [%s] is now terminated\n",
        node->config.entity.name.value));

terminated:

    // WORKQUEUE

    if ( node->config.adminState == CL_AMS_ADMIN_STATE_LOCKED_I )
    {
        AMS_CALL ( clAmsPeNodeLockInstantiationCallback(node, nerror) );
    }

    if ( node->status.recovery )
    {
        AMS_CALL( clAmsPeNodeFaultCallback_Step2(node, nerror) );
    }
    else
    {
        if ( node->status.isClusterMember == CL_AMS_NODE_IS_NOT_CLUSTER_MEMBER )
        {
            AMS_CALL( clAmsPeNodeHasLeftClusterCallback_Step2(node, nerror) );
        }

        if ( node->status.isClusterMember == CL_AMS_NODE_IS_LEAVING_CLUSTER )
        {
            AMS_CALL( clAmsPeNodeIsLeavingClusterCallback_Step2(node, nerror) );
        }
    }

    return CL_OK;
}

/*
 * clAmsPeNodeEvaluateWork
 * -----------------------
 * Evaluate the work requirements for a node, which distills down to
 * evaluating the work requirements for the SUs on the node. And
 * evaluation of the work requirements of a SU is dependent on the
 * work needs of the SG to which it belongs.
 */

ClRcT
clAmsPeNodeEvaluateWork(
        CL_IN   ClAmsNodeT *node)
{
    ClAmsAdminStateT adminState;

    AMS_CHECK_NODE ( node );

    AMS_FUNC_ENTER ( ("Node [%s]\n",node->config.entity.name.value) );

    if ( !node->config.isASPAware )
    {
        AMS_ENTITY_LOG (node, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
                 ("Node [%s] is not ASP aware. Nothing to do.\n",
                  node->config.entity.name.value));
        
        return CL_OK;
    }

    AMS_CALL ( clAmsPeNodeComputeAdminState(node, &adminState) );

    if ( (adminState != CL_AMS_ADMIN_STATE_UNLOCKED) &&
         (adminState != CL_AMS_ADMIN_STATE_LOCKED_A) )
    {
        AMS_ENTITY_LOG(node, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
            ("Node [%s] has an admin state of [%d]. Ignoring evaluate work request..\n",
             node->config.entity.name.value,
             node->config.adminState));

        return CL_OK;
    }

    AMS_ENTITY_LOG (node, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
             ("Evaluating work for Node [%s]\n",
              node->config.entity.name.value));

    AMS_CALL ( clAmsEntityListWalkGetEntity(
                    &node->config.suList,
                    (ClAmsEntityCallbackT) clAmsPeSUEvaluateWork) );
    
    /*
     * Finally, evaluate the work for dependents of this node. This is needed as the
     * dependent nodes may support SGs not on the dependency and they would not get
     * evaluated by the above.
     */

    AMS_CALL ( clAmsEntityListWalkGetEntity(
                    &node->config.nodeDependentsList, 
                    (ClAmsEntityCallbackT) clAmsPeNodeEvaluateWork) );

    return CL_OK;
}

/*
 * clAmsPeNodeSwitchoverWork
 * -------------------------
 * This fn switches over the work assigned to a node, ie, the work assigned
 * to the SUs on the node. It may be called for multiple reasons; when a
 * node is lock assigned, when it is shutdown, when a node leaves the cluster
 * or when there is a fault on a node.
 *
 * Note, this fn recursively calls itself.
 */

ClRcT
clAmsPeNodeSwitchoverWork(
                          CL_IN   ClAmsNodeT *node,
                          CL_IN   ClUint32T switchoverMode)
{
    ClAmsEntityRefT *entityRef;
    CL_LIST_HEAD_DECLARE(suListHead);
    typedef struct ClAmsNodeSUList
    {
        ClAmsSUT *su;
        ClListHeadT list;
    }ClAmsNodeSUListT;
    ClAmsNodeSUListT *suList = NULL;
    ClUint32T numSUs = 0;
    ClAmsNotificationDescriptorT notification = {0};
    ClAmsNotificationTypeT notificationType = CL_AMS_NOTIFICATION_NODE_SWITCHOVER;
    ClRcT rc = CL_OK;

    AMS_CHECK_NODE ( node );

    AMS_FUNC_ENTER ( ("Node [%s]\n",node->config.entity.name.value) );

    AMS_ENTITY_LOG (node, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
                    ("Node [%s] Switchover Request...\n",
                     node->config.entity.name.value));

    /*
     * Stop any timers for this node
     */

    node->status.suFailoverCount = 0;
    AMS_CALL ( clAmsEntityTimerStop((ClAmsEntityT *) node,
                                    CL_AMS_NODE_TIMER_SUFAILOVER) );

    if(node->status.isClusterMember == CL_AMS_NODE_IS_NOT_CLUSTER_MEMBER
        ||
       node->status.operState == CL_AMS_OPER_STATE_DISABLED)
    {
        notificationType = CL_AMS_NOTIFICATION_NODE_FAILOVER;
    }

    if(clAmsGenericNotificationEventPayloadSet(notificationType, &node->config.entity,
                                               &notification) == CL_OK)
    {
        clAmsNotificationEventPublish(&notification);
    }

    /*
     * Tell this node's dependents that it is switching over. The switchover mode
     * will automatically be overridden at the component level if the node is
     * not a member of the cluster.
     */

    for ( entityRef = clAmsEntityListGetFirst(&node->config.nodeDependentsList);
          entityRef != (ClAmsEntityRefT *) NULL;
          entityRef = clAmsEntityListGetNext(&node->config.nodeDependentsList,
                                             entityRef) )
    {
        ClAmsNodeT *n = (ClAmsNodeT *) entityRef->ptr;

        // TODO If node dependency is due to containment, then change switchover
        // mode to fast.

        AMS_CALL ( clAmsPeNodeSwitchoverWork(n, switchoverMode) );
    }

    /*
     * Switchover the work assigned to SUs on this node. We do this in
     * two phases, so that SUs with proxied and non-proxied components
     * are switched over first and then the fully preinstantiable SUs.
     * This ensures that proxies are still around (for some of the tasks) 
     * to talk to the proxied components.
     *
     */
    
    suList = clHeapCalloc(node->config.suList.numEntities, sizeof(*suList));
    CL_ASSERT(suList != NULL);
    for ( entityRef = clAmsEntityListGetFirst(&node->config.suList);
          entityRef != (ClAmsEntityRefT *) NULL;
          entityRef = clAmsEntityListGetNext(&node->config.suList,entityRef) )
    {
        ClAmsSUT *su = (ClAmsSUT *) entityRef->ptr;
        if(!su) continue;

        clAmsPeEntityOpsReplay(&su->config.entity, &su->status.entity, CL_TRUE);

        clAmsPeSUSIReassignEntryDelete(su);

        if(!su->status.numActiveSIs
           &&
           !su->status.numStandbySIs
           &&
           !su->status.numQuiescedSIs)
            continue;

        suList[numSUs].su = su;
        if ( clAmsPeSUHasProxiedComponents(su) == CL_TRUE )
        {
            clListAdd(&suList[numSUs++].list, &suListHead);
        }
        else
        {
            /*
             * Check if the SUs have SIs that have no dependencies
             */
            ClAmsEntityRefT *siRef = NULL;
            ClBoolT dependency = CL_FALSE;
            for(siRef = clAmsEntityListGetFirst(&su->status.siList);
                siRef != NULL;
                siRef = clAmsEntityListGetNext(&su->status.siList, siRef))
            {
                ClAmsSIT *si = (ClAmsSIT*)siRef->ptr;
                if(!si->config.siDependenciesList.numEntities
                   &&
                   si->config.siDependentsList.numEntities)
                {
                    dependency = CL_TRUE;
                    break;
                }
                        
            }
            if(dependency)
            {
                clListAdd(&suList[numSUs++].list, &suListHead);
            }
            else
            {
                clListAddTail(&suList[numSUs++].list, &suListHead);
            }
        }
    }

    AMS_ENTITY_LOG (node, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
                    ("Switching over [%d] SUs on Node [%s]\n",
                     numSUs,
                     node->config.entity.name.value));

    if(numSUs)
    {
        while(!CL_LIST_HEAD_EMPTY(&suListHead))
        {
            ClListHeadT *head = suListHead.pNext;
            ClAmsNodeSUListT *entry = CL_LIST_ENTRY(head, ClAmsNodeSUListT, list);
            ClAmsSUT *su = entry->su;
            clListDel(head);
            AMS_CHECK_RC_ERROR ( clAmsPeSUSwitchoverWork(su, switchoverMode) );
        }
    }
    AMS_CHECK_RC_ERROR ( clAmsPeNodeSwitchoverCallback(node, CL_OK, switchoverMode) );

    exitfn:
    if(suList)
    {
        clHeapFree(suList);
    }
    return rc;
}

/*
 * clAmsPeNodeSwitchoverWorkCallback
 * ---------------------------------
 * Callback when a SU has switched over. Waits until all SUs have switched
 * over and returns control to the original operation that triggered the
 * switchover.
 *
 * Note, we may get multiple callbacks to this fn, once all SUs on the
 * node have quiesced and once after the quiesced SIs are removed.
 */

ClRcT
clAmsPeNodeSwitchoverCallback(
        CL_IN   ClAmsNodeT *node,
        CL_IN   ClUint32T error,
        CL_IN   ClUint32T switchoverMode)
{
    ClAmsEntityRefT *entityRef;
    ClUint32T nodeHasAssignments = 0;

    AMS_CHECK_NODE (node);

    AMS_FUNC_ENTER ( ("Node [%s]\n",node->config.entity.name.value) );

    for ( entityRef = clAmsEntityListGetFirst(&node->config.suList);
          entityRef != (ClAmsEntityRefT *) NULL;
          entityRef = clAmsEntityListGetNext(&node->config.suList,entityRef) )
    {
        ClAmsSUT *su = (ClAmsSUT *) entityRef->ptr;

        AMS_CHECK_SU ( su );

        if ( su->status.numActiveSIs || 
             su->status.numStandbySIs ||
             su->status.numQuiescedSIs )
        {
            nodeHasAssignments++;
        }
    }

    if ( nodeHasAssignments )
    {
        AMS_ENTITY_LOG(node, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
            ("Node [%s] has [%d] assigned SUs. Ignoring switchover callback..\n",
             node->config.entity.name.value,
             nodeHasAssignments));

        return CL_OK;
    }

    AMS_ENTITY_LOG(node, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
        ("Node [%s] switchover callback..\n",
         node->config.entity.name.value));

    // WORKQUEUE

    if ( node->config.adminState == CL_AMS_ADMIN_STATE_LOCKED_A )
    {
        AMS_CALL(clAmsPeNodeLockAssignmentCallback(node, error) );
    }

    if ( node->config.adminState == CL_AMS_ADMIN_STATE_SHUTTINGDOWN )
    {
        AMS_CALL(clAmsPeNodeShutdownCallback(node, error) );
    }

    if ( node->status.recovery )
    {
        AMS_CALL(clAmsPeNodeFaultCallback_Step1(node, error) );
    }
    else
    {
        if ( node->status.isClusterMember == CL_AMS_NODE_IS_NOT_CLUSTER_MEMBER )
        {
            AMS_CALL(clAmsPeNodeHasLeftClusterCallback_Step1(node, error) );
        }

        if ( node->status.isClusterMember == CL_AMS_NODE_IS_LEAVING_CLUSTER )
        {
            AMS_CALL(clAmsPeNodeIsLeavingClusterCallback_Step1(node, error) );
        }
    }

    return CL_OK;
}

/*
 * clAmsPeNodeRemoveWork
 * ---------------------
 * Remove work assigned to all SUs on node. This fn is only used when it is
 * clear that service recovery is not necessary.
 *
 * Note: This fn is not used for now, we make do with switchoverWork.
 */

ClRcT
clAmsPeNodeRemoveWork(
        CL_IN       ClAmsNodeT      *node)
{
    ClAmsEntityRefT *entityRef;

    AMS_CHECK_NODE ( node );

    AMS_FUNC_ENTER ( ("Node [%s]\n",node->config.entity.name.value) );

    for ( entityRef = clAmsEntityListGetFirst(&node->config.suList);
          entityRef != (ClAmsEntityRefT *) NULL;
          entityRef = clAmsEntityListGetNext(&node->config.suList, entityRef) )
    {
        ClAmsSUT * su;

        AMS_CHECK_SU ( su = (ClAmsSUT *) entityRef->ptr );

        AMS_CALL ( clAmsPeSUSwitchoverWork(su, CL_AMS_ENTITY_SWITCHOVER_FAST) );
    }

    return CL_OK;
}


/*
 * clAmsPeNodeSUFailoverTimeout
 * ----------------------------
 * AMS counts the number of SU failover's that take place within a node. If too
 * many of them take place within a timeout, the recovery is escalated to a 
 * node failure. If this timeout occurs and this fn is called, then is means that
 * the number of SU failovers is less than the configured limit. So we clear the 
 * su failover counter for the node and reset the timer.
 */

ClRcT
clAmsPeNodeSUFailoverTimeout( 
        CL_IN ClAmsEntityTimerT  *timer) 
{
    ClAmsNodeT *node;

    AMS_CHECKPTR ( !timer );

    node = (ClAmsNodeT *) timer->entity;

    AMS_CHECK_NODE (node);

    AMS_FUNC_ENTER ( ("Node [%s]\n",node->config.entity.name.value) );

    node->status.suFailoverCount = 0;

    AMS_CALL ( clAmsEntityTimerStop((ClAmsEntityT *) node,
                    CL_AMS_NODE_TIMER_SUFAILOVER) );

    return CL_OK;
}

/*-----------------------------------------------------------------------------
 * Useful utility functions for Node
 *---------------------------------------------------------------------------*/

/*
 * clAmsPeNodeIsInstantiable
 * -------------------------
 * A node is considered instantiable if its computed admin state is unlocked
 * or lock_a, it is operationally enabled, a member of the cluster and if its 
 * dependencies are also instantiable.
 *
 * Future: We may want to change the dependency check to instantiated.
 */

ClRcT
clAmsPeNodeIsInstantiable(
        CL_IN   ClAmsNodeT *node)
{
    ClAmsAdminStateT adminState;

    AMS_CHECK_NODE(node);

    AMS_FUNC_ENTER ( ("Node [%s]\n",node->config.entity.name.value) );

    if(!node->config.isASPAware)
    {
        return CL_OK;
    }

    AMS_CALL ( clAmsPeNodeComputeAdminState(node, &adminState) );

    if ( ( (adminState == CL_AMS_ADMIN_STATE_UNLOCKED) ||
           (adminState == CL_AMS_ADMIN_STATE_LOCKED_A) ) &&
         (node->status.operState == CL_AMS_OPER_STATE_ENABLED) &&
         (node->status.isClusterMember == CL_AMS_NODE_IS_CLUSTER_MEMBER) )
    {
        if ( clAmsEntityListWalkGetEntity(
                    &node->config.nodeDependenciesList, 
                    (ClAmsEntityCallbackT) clAmsPeNodeIsInstantiable2) == CL_OK )
        {
            return CL_OK;
        }
    }

    return CL_AMS_RC(CL_AMS_ERR_ENTITY_NOT_ENABLED);
}

ClRcT
clAmsPeNodeIsInstantiable2(
        CL_IN   ClAmsNodeT *node)
{
    AMS_CHECK_NODE(node);

    AMS_FUNC_ENTER ( ("Node [%s]\n",node->config.entity.name.value) );

    if ( (node->status.presenceState == CL_AMS_PRESENCE_STATE_INSTANTIATED) &&
         (node->status.operState == CL_AMS_OPER_STATE_ENABLED) &&
         (node->status.isClusterMember == CL_AMS_NODE_IS_CLUSTER_MEMBER) )
    {
        // Recursion is not necessary. If a node is instantiated, means all of its
        // dependencies are instantiated.

        return CL_OK;
    }
    
    return CL_AMS_RC(CL_AMS_ERR_ENTITY_NOT_ENABLED);
}

/*
 * clAmsPeNodeComputeAdminState
 * ----------------------------
 * The computed admin state for a Node is dependent on the node's own admin 
 * state and the admin state of the cluster of which it is a member. It is
 * also dependent on the admin states of the nodes on which it is dependent.
 */

static ClRcT
clAmsPeNodeComputeAdminStateExtended(
        CL_IN   ClAmsNodeT *node,
        CL_OUT  ClAmsAdminStateT *adminState,
        CL_IN   ClAmsAdminStateT nodeAdminState)
{
    ClAmsEntityRefT *eRef;

    AMS_CHECK_NODE (node);
    AMS_CHECKPTR (!adminState);

    AMS_FUNC_ENTER ( ("Node [%s]\n",node->config.entity.name.value) );

    // Future: Add support for cluster.

    if(!node->config.isASPAware)
    {
        *adminState = CL_AMS_ADMIN_STATE_UNLOCKED;
        return CL_OK;
    }

    *adminState = nodeAdminState;

    if ( *adminState == CL_AMS_ADMIN_STATE_LOCKED_I )
    {
        return CL_OK;
    }

    for ( eRef = clAmsEntityListGetFirst(&node->config.nodeDependenciesList);
          eRef != (ClAmsEntityRefT *) NULL;
          eRef = clAmsEntityListGetNext(&node->config.nodeDependenciesList, eRef) )
    {
        ClAmsNodeT *n = (ClAmsNodeT *) eRef->ptr;
        
        if (AMS_ENTITY_OK(n,CL_AMS_ENTITY_TYPE_NODE))
        {            
          if ( n->config.adminState == CL_AMS_ADMIN_STATE_LOCKED_I )
          {
            *adminState = CL_AMS_ADMIN_STATE_LOCKED_I;
            return CL_OK;  /* most restrictive state, so might as well return early */
          }

          if ( n->config.adminState == CL_AMS_ADMIN_STATE_LOCKED_A )
          {
            *adminState = CL_AMS_ADMIN_STATE_LOCKED_A;
          }

          if ( (n->config.adminState == CL_AMS_ADMIN_STATE_SHUTTINGDOWN) && (*adminState != CL_AMS_ADMIN_STATE_LOCKED_A) )
          {
            *adminState = CL_AMS_ADMIN_STATE_SHUTTINGDOWN;
          }
        }
        
    }

    return CL_OK;
}

ClRcT
clAmsPeNodeComputeAdminState(
        CL_IN   ClAmsNodeT *node,
        CL_OUT  ClAmsAdminStateT *adminState)
{
    if(!node) 
        return CL_AMS_RC(CL_ERR_NULL_POINTER);

    return clAmsPeNodeComputeAdminStateExtended(node, adminState, node->config.adminState);
}


/*
 * clAmsPeNodeUpdateReadinessState
 * -------------------------------
 * A node does not have readiness state, but its constituent SU and Components do.
 * This fn is called to update their readiness state as a result of a change in
 * the admin state or operational state of the node.
 */

ClRcT
clAmsPeNodeUpdateReadinessState(
        CL_IN   ClAmsNodeT *node)
{
    ClAmsEntityRefT *entityRef;

    AMS_CHECK_NODE (node);

    /*
     * Update state of its constituent components
     */

    for ( entityRef = clAmsEntityListGetFirst(&node->config.suList);
          entityRef != (ClAmsEntityRefT *) NULL;
          entityRef = clAmsEntityListGetNext(&node->config.suList, entityRef) )
    {
        ClAmsSUT *su = (ClAmsSUT *) entityRef->ptr;

        AMS_CHECK_SU ( su );

        AMS_CALL ( clAmsPeSUUpdateReadinessState(su) );
    }

    return CL_OK;
}

/*
 * clAmsPeNodeComputeRecoveryAction
 * --------------------------------
 * Given a node and a recommended recovery action, this fn computes
 * an appropriate recovery taking escalation rules into account. It does
 * not initiate the recovery action.
 */

ClRcT
clAmsPeNodeComputeRecoveryAction(
        CL_IN       ClAmsNodeT                  *node,
        CL_INOUT    ClAmsLocalRecoveryT         *recovery,
        CL_OUT      ClUint32T                   *escalation)
{
    ClBoolT match = CL_FALSE;

    AMS_CHECKPTR ( !recovery || !escalation );
    AMS_CHECK_NODE ( node );

    AMS_FUNC_ENTER ( ("Node [%s]\n", node->config.entity.name.value) );

    /*
     * Make sure the node is enabled and a member of the cluster.
     */

    if ( node->config.isASPAware &&
         node->status.operState == CL_AMS_OPER_STATE_DISABLED )
    {
        match = CL_TRUE;

        *recovery = CL_AMS_RECOVERY_NONE;
    }
    
    if ( node->config.isASPAware
         &&
         node->status.isClusterMember == CL_AMS_NODE_IS_NOT_CLUSTER_MEMBER )
    {
        match = CL_TRUE;

        *recovery = CL_AMS_RECOVERY_NONE;
    }


    if ( *recovery == CL_AMS_RECOVERY_NO_RECOMMENDATION )
    {
        match = CL_TRUE;

        if ( !node->config.isASPAware ||
             node->config.isRestartable )
        {
            *recovery = CL_AMS_RECOVERY_NODE_FAILOVER;
        }
        else
        {
            *recovery = CL_AMS_RECOVERY_CLUSTER_RESET;
            *escalation = CL_TRUE;
        }
    }

    if ( *escalation == CL_FALSE )
    {
        if ( clAmsPeEntityRecoveryScopeLarger(
                    node->status.recovery, *recovery) )
        {
            *recovery = CL_AMS_RECOVERY_NONE;
        }
    }

    if ( *recovery == CL_AMS_RECOVERY_NODE_SWITCHOVER )
    {
        match = CL_TRUE;
    }

    if ( *recovery == CL_AMS_RECOVERY_NODE_FAILOVER )
    {
        match = CL_TRUE;
    }

    if ( *recovery == CL_AMS_RECOVERY_NODE_FAILFAST )
    {
        match = CL_TRUE;
    }

    if ( *recovery == CL_AMS_RECOVERY_NODE_HALT )
    {
        match = CL_TRUE;
    }

    if ( *recovery == CL_AMS_RECOVERY_CLUSTER_RESET )
    {
        match = CL_TRUE;
    }

    if ( *recovery == node->status.recovery )
    {
        match = CL_FALSE;
    }

    if ( !match )
    {
        *recovery = CL_AMS_RECOVERY_NONE;
    }

    if ( *recovery != CL_AMS_RECOVERY_NONE )
    {
        node->status.recovery = *recovery;
    }

    return CL_OK;
}

/*
 * clAmsPeNodeReset
 * ----------------
 * Reset the status of a node and its constituent SUs and components. Only the
 * wasMemberBefore and isClusterMember fields in node status are left untouched
 * as they need to be persistant.
 */

ClRcT clAmsPeNodeReset(CL_IN ClAmsNodeT *node)
{
    ClAmsNodeClusterMemberT isClusterMember;
    ClBoolT wasMemberBefore;
    ClAmsEntityRefT *entityRef;

    AMS_CHECK_NODE ( node );

    AMS_FUNC_ENTER ( ("Node [%s]\n",node->config.entity.name.value) );

    wasMemberBefore = node->status.wasMemberBefore;
    isClusterMember = node->status.isClusterMember;

    clAmsEntityReset((ClAmsEntityT *) node);

    node->status.wasMemberBefore = wasMemberBefore;
    node->status.isClusterMember = isClusterMember;

    CL_AMS_SET_O_STATE(node, CL_AMS_OPER_STATE_ENABLED);

    for ( entityRef = clAmsEntityListGetFirst(&node->config.suList);
          entityRef != (ClAmsEntityRefT *) NULL;
          entityRef = clAmsEntityListGetNext(&node->config.suList,entityRef) )
    {
        ClAmsSUT *su = (ClAmsSUT *) entityRef->ptr;
        if (su) clAmsPeSUReset(su);
    }

    return CL_OK;
}

/******************************************************************************
 * SU Functions
 *****************************************************************************/

/*-----------------------------------------------------------------------------
 * Functions called from the Management API
 *---------------------------------------------------------------------------*/

/*
 * clAmsPeSUUnlock
 * ---------------
 * When an SU is unlocked, it becomes ready to be instantiated and/or assigned
 * work as per the needs of the SG.
 */

ClRcT
clAmsPeSUUnlock(
                CL_IN   ClAmsSUT    *su) 
{
    ClAmsAdminStateT adminState = 0;

    AMS_CHECK_SU ( su );

    AMS_FUNC_ENTER ( ("SU [%s]\n",su->config.entity.name.value) );

    AMS_ENTITY_LOG (su, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
                    ("Admin Operation [Unlock] on SU [%s]\n",
                     su->config.entity.name.value));

    if(clAmsInvocationsPendingForSU(su))
    {
        clLogInfo("SU", "UNLOCK", "SU [%s] has invocations pending. Deferring unlock",
                  su->config.entity.name.value);
        return CL_AMS_RC(CL_ERR_TRY_AGAIN);
    }

    clAmsPeSUComputeAdminStateExtended(su, &adminState, CL_AMS_ADMIN_STATE_UNLOCKED);

    if(adminState == CL_AMS_ADMIN_STATE_UNLOCKED)
    {
        if(su->status.presenceState == CL_AMS_PRESENCE_STATE_INSTANTIATING
           ||
           su->status.presenceState == CL_AMS_PRESENCE_STATE_RESTARTING
           ||
           su->status.presenceState == CL_AMS_PRESENCE_STATE_TERMINATING)
        {
            clLogInfo("SU", "UNLOCK", "SU [%s] has presence state [%s]. Deferring unlock",
                      su->config.entity.name.value, CL_AMS_STRING_P_STATE(su->status.presenceState));
            return CL_AMS_RC(CL_ERR_TRY_AGAIN);
        }
    }

    CL_AMS_SET_A_STATE(su, CL_AMS_ADMIN_STATE_UNLOCKED);

    if ( adminState == CL_AMS_ADMIN_STATE_UNLOCKED )
    {
        AMS_CALL ( clAmsPeSUEvaluateWork(su) );
    }

    return CL_OK;
}

ClRcT
clAmsPeSUUnlockCallback(
        CL_IN   ClAmsSUT *su,
        CL_IN   ClUint32T error)
{
    return CL_OK;
}

/*
 * clAmsPeSULockAssignment
 * -----------------------
 * The fn is called by the mgmt api to put a SU in the locked assignment state.
 * When a SU is in the locked assignment state it may be instantiated but cannot 
 * be assigned any work. Consequently, the action to take when a SU is changed
 * to this state depends on its current state. If the current state is locked 
 * instantiation, the SU is now free to be instantiated. If the current state 
 * is unlocked, the SU must actually give up any work assignments that may have 
 * been assigned to it previously.
 */

ClRcT
clAmsPeSULockAssignment(
        CL_IN       ClAmsSUT        *su)
{
    ClAmsAdminStateT adminState;
    ClAmsReadinessStateT readinessState;
    ClAmsSGT *sg = NULL;
    ClAmsNodeT *node = NULL;

    AMS_CHECK_SU ( su );
    AMS_CHECK_SG ( sg = (ClAmsSGT*)su->config.parentSG.ptr);
    AMS_CHECK_NODE( node = (ClAmsNodeT*)su->config.parentNode.ptr);
    AMS_FUNC_ENTER ( ("SU [%s]\n",su->config.entity.name.value) );

    AMS_ENTITY_LOG (su, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
            ("Admin Operation [Lock Assignment] on SU [%s]\n",
             su->config.entity.name.value));

    if(clAmsInvocationsPendingForSU(su))
    {
        clLogInfo("SU", "LOCK", "SU [%s] has invocations pending. Deferring lock",
                  su->config.entity.name.value);
        return CL_AMS_RC(CL_ERR_TRY_AGAIN);
    }

    readinessState = su->status.readinessState;

    clAmsPeSUComputeAdminState(su, &adminState);

    if(adminState == CL_AMS_ADMIN_STATE_UNLOCKED 
       &&
       clAmsInvocationsPendingForSG(sg))
    {
        clLogInfo("SU", "LOCK", 
                  "SG [%s] containing SU [%s] has invocations pending. Deferring lock",
                  sg->config.entity.name.value, su->config.entity.name.value);
        return CL_AMS_RC(CL_ERR_TRY_AGAIN);
    }
    
    if(adminState == CL_AMS_ADMIN_STATE_LOCKED_I
       &&
       node->config.isASPAware 
       && 
       su->status.presenceState != CL_AMS_PRESENCE_STATE_UNINSTANTIATED
       &&
       su->status.presenceState != CL_AMS_PRESENCE_STATE_RESTARTING)
    {
        clLogInfo("SU", "LOCK", 
                  "SU [%s] is in presence state [%s]. Deferring lock",
                  su->config.entity.name.value, CL_AMS_STRING_P_STATE(su->status.presenceState));
        return CL_AMS_RC(CL_ERR_TRY_AGAIN);
    }

    CL_AMS_SET_A_STATE(su, CL_AMS_ADMIN_STATE_LOCKED_A);

    if ( adminState == CL_AMS_ADMIN_STATE_LOCKED_I )
    {
        AMS_CALL ( clAmsPeSUMarkInstantiable(su) );
        AMS_CALL ( clAmsPeSUEvaluateWork(su) );
    }
    else if ( adminState == CL_AMS_ADMIN_STATE_UNLOCKED )
    {
        if ( readinessState != CL_AMS_READINESS_STATE_OUTOFSERVICE )
        {
            AMS_CALL ( clAmsPeSUSwitchoverWork(
                            su,
                            CL_AMS_ENTITY_SWITCHOVER_IMMEDIATE) );
        }
        else
        {
            AMS_CALL ( clAmsPeSULockAssignmentCallback(su, CL_OK) );
        }
    }

    return CL_OK;
}

/*
 * Force a recovery specific soft lock/unlock assignment. on the SU which would be succeeded
 * by a lock instantiation. This is to enable back-doors to disable an assignment component restart loop where
 * the escalation policy is purposely disabled and operator maintenance is required to lock instantiate 
 * the service unit
 */
ClRcT
clAmsPeSUForceLockOperation(
                            CL_IN       ClAmsSUT *su,
                            CL_IN       ClBoolT lock)
{
    ClAmsAdminStateT adminState = CL_AMS_ADMIN_STATE_NONE;
    ClAmsSGT *sg = NULL;
    ClAmsNodeT *node = NULL;

    AMS_CHECK_SU ( su );
    AMS_CHECK_SG ( sg = (ClAmsSGT*)su->config.parentSG.ptr);
    AMS_CHECK_NODE( node = (ClAmsNodeT*)su->config.parentNode.ptr);
    AMS_FUNC_ENTER ( ("SU [%s]\n",su->config.entity.name.value) );

    clLogNotice("SU", "LOCK-FORCE", "Admin Operation [Forced Soft %s Assignment] on SU [%s]",
                lock ? "Lock" : "Unlock", su->config.entity.name.value);

    if(lock)
    {
        clAmsPeSUComputeAdminState(su, &adminState);

        if(adminState != CL_AMS_ADMIN_STATE_UNLOCKED)
        {
            clLogError("SU", "LOCK-FORCE", "Admin state of the SU has to be unlocked for force lock to work. "
                       "Current admin state [%s]", CL_AMS_STRING_A_STATE(adminState));
            return CL_AMS_RC(CL_ERR_INVALID_STATE);
        }

        /*
         * Schedule a soft admin state. transition so that the next recovery restart loop fault transitions to a 
         * component failover
         */
        su->config.adminState = CL_AMS_ADMIN_STATE_LOCKED_A;
    }
    else
    {
        if(su->config.adminState != CL_AMS_ADMIN_STATE_LOCKED_A)
        {
            clLogError("SU", "LOCK-FORCE", "Admin state of the SU has to be locked for force unlock to work. "
                       "Current admin state [%s]", CL_AMS_STRING_A_STATE(su->config.adminState));
            return CL_AMS_RC(CL_ERR_INVALID_STATE);
        }

        /*
         * Schedule a soft admin state. transition so that the next recovery restart loop fault transitions to a 
         * component failover
         */
        su->config.adminState = CL_AMS_ADMIN_STATE_UNLOCKED;
    }

    return CL_OK;
}

ClRcT
clAmsPeSUForceLockInstantiationOperation(
                                         CL_IN       ClAmsSUT *su)
{
    ClAmsAdminStateT adminState = CL_AMS_ADMIN_STATE_NONE;
    ClAmsSGT *sg = NULL;
    ClAmsNodeT *node = NULL;
    ClAmsRecoveryT recovery = CL_AMS_RECOVERY_INTERNALLY_RECOVERED;
    ClUint32T escalation = (ClUint32T)CL_FALSE;

    AMS_CHECK_SU ( su );
    AMS_CHECK_SG ( sg = (ClAmsSGT*)su->config.parentSG.ptr);
    AMS_CHECK_NODE( node = (ClAmsNodeT*)su->config.parentNode.ptr);
    AMS_FUNC_ENTER ( ("SU [%s]\n",su->config.entity.name.value) );

    clLogNotice("SU", "LOCK-FORCE", "Admin Operation [Forced lock instantiation trigger] on SU [%s]",
                su->config.entity.name.value);

    if(su->status.recovery != CL_AMS_RECOVERY_NONE)
    {
        clLogNotice("SU", "LOCKI", "SU [%s] already has a [%s] recovery pending."
                    "Skipping forced lock instantiation",
                    su->config.entity.name.value,
                    CL_AMS_STRING_RECOVERY(su->status.recovery));
        return CL_AMS_RC(CL_ERR_NO_OP);
    }

#if 0
    if(clAmsInvocationsPendingForSU(su))
    {
        clLogInfo("SU", "LOCKI", "SU [%s] has invocations pending. Deferring force lock instantiation",
                  su->config.entity.name.value);
        return CL_AMS_RC(CL_ERR_TRY_AGAIN);
    }

    if(clAmsInvocationsPendingForSG(sg))
    {
        clLogInfo("SU", "LOCKI", 
                  "SG [%s] containing SU [%s] has pending invocations. Deferring force lock instantiation",
                  sg->config.entity.name.value, su->config.entity.name.value);
        return CL_AMS_RC(CL_ERR_TRY_AGAIN);
    }
#endif

    clAmsPeSUComputeAdminState(su, &adminState);

    if(adminState == CL_AMS_ADMIN_STATE_LOCKED_I)
    {
        return CL_AMS_RC(CL_ERR_NO_OP);
    }

    su->config.adminState = CL_AMS_ADMIN_STATE_LOCKED_I;

    if(adminState == CL_AMS_ADMIN_STATE_UNLOCKED)
    {
        /*
         * Schedule a soft admin state. transition so that the next recovery restart loop fault transitions to a 
         * component failover
         */
        clAmsPeSUFaultReport(su, NULL, &recovery, &escalation);
    }
    else if(adminState == CL_AMS_ADMIN_STATE_LOCKED_A)
    {
        /*
         * Directly force cleanup the SU
         */
        clAmsPeSUCleanup(su);
    }
    return CL_OK;
}

/*
 * clAmsPeSULockAassignmentCallback
 * --------------------------------
 * This fn is called when a Lock A operation has completed
 */

ClRcT
clAmsPeSULockAssignmentCallback(
        CL_IN       ClAmsSUT            *su,
        CL_IN       ClUint32T           result)
{
    ClAmsAdminStateT adminState;
    ClAmsSGT *sg;

    AMS_CHECK_SU ( su );
    AMS_CHECK_SG ( sg = (ClAmsSGT *) su->config.parentSG.ptr );

    AMS_FUNC_ENTER ( ("SU [%s]\n",su->config.entity.name.value) );

    clAmsPeSUComputeAdminState(su, &adminState);

    if ( adminState != CL_AMS_ADMIN_STATE_LOCKED_A )
    {
        AMS_ENTITY_LOG(su, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
            ("Lock Assignment callback of SU [%s] in computed admin state [%s] has no effect. Continuing..\n",
             su->config.entity.name.value,
             CL_AMS_STRING_A_STATE(adminState)));

        return CL_OK;
    }

    return CL_OK;
}

/*
 * clAmsPeSULockInstantiation
 * --------------------------
 * Lock instantiation is the lowest energy state for a SU. It is not 
 * instantiated and has no work assignments. The SU however, remains 
 * on the instantiable list of the SG, so it can be instantiated again 
 * by a lock assignment and an unlock.
 */

ClRcT
clAmsPeSULockInstantiation(
        CL_IN       ClAmsSUT        *su)
{
    ClAmsAdminStateT adminState;
    ClAmsSGT *sg = NULL;

    AMS_CHECK_SU ( su );
    AMS_CHECK_SG ( sg = (ClAmsSGT*)su->config.parentSG.ptr );

    AMS_FUNC_ENTER ( ("SU [%s]\n",su->config.entity.name.value) );

    AMS_ENTITY_LOG (su, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
            ("Admin Operation [Lock Instantiation] on SU [%s]\n",
             su->config.entity.name.value));

    if(clAmsInvocationsPendingForSU(su))
    {
        clLogInfo("SU", "LOCKI", "SU [%s] has invocations pending. Deferring lock instantiation",
                  su->config.entity.name.value);
        return CL_AMS_RC(CL_ERR_TRY_AGAIN);
    }

#if 0
    if(clAmsInvocationsPendingForSG(sg))
    {
        clLogInfo("SU", "LOCKI", 
                  "SG [%s] containing SU [%s] has pending invocations. Deferring lock instantiation",
                  sg->config.entity.name.value, su->config.entity.name.value);
        return CL_AMS_RC(CL_ERR_TRY_AGAIN);
    }
#endif

    clAmsPeSUComputeAdminState(su, &adminState);

    CL_AMS_SET_A_STATE(su, CL_AMS_ADMIN_STATE_LOCKED_I);

    if ( adminState == CL_AMS_ADMIN_STATE_LOCKED_A )
    {
        if ( su->status.recovery )
        {
            AMS_CALL ( clAmsPeSUCleanup(su) );
        }
        else
        {
            AMS_CALL ( clAmsPeSUTerminate(su) );
        }
    }

    return CL_OK;
}

ClRcT
clAmsPeSULockInstantiationCallback(
        CL_IN   ClAmsSUT *su,
        CL_IN   ClUint32T error)
{
    AMS_CHECK_SU (su);

    AMS_FUNC_ENTER ( ("SU [%s]\n",su->config.entity.name.value) );

    AMS_CALL ( clAmsPeSUMarkUninstantiable(su) );

    if ( su->status.recovery )
    {
        CL_AMS_SET_O_STATE(su, CL_AMS_OPER_STATE_DISABLED);
    }

    return CL_OK;
}

/*
 * clAmsPeSUShutdown
 * -----------------
 * A shutdown informs the AMS that all SIs assigned to this SU should
 * be switched over gracefully.
 */

ClRcT
clAmsPeSUShutdown(
        CL_IN       ClAmsSUT        *su)
{
    ClAmsAdminStateT oldAdminState;
    ClAmsReadinessStateT readinessState;
    ClAmsSGT *sg = NULL;

    AMS_CHECK_SU ( su );
    AMS_CHECK_SG ( sg = (ClAmsSGT*)su->config.parentSG.ptr);

    AMS_FUNC_ENTER ( ("SU [%s]\n",su->config.entity.name.value) );

    AMS_ENTITY_LOG (su, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
            ("Admin Operation [Shutdown] on SU [%s]\n",
             su->config.entity.name.value));

    if(clAmsInvocationsPendingForSU(su))
    {
        clLogInfo("SU", "SHUTDOWN", 
                  "SU [%s] has invocations pending. Deferring shutdown",
                  su->config.entity.name.value);
        return CL_AMS_RC(CL_ERR_TRY_AGAIN);
    }

    if(clAmsInvocationsPendingForSG(sg))
    {
        clLogInfo("SU", "SHUTDOWN", 
                  "SG [%s] containing SU [%s] has invocations pending. Deferring shutdown",
                  sg->config.entity.name.value, su->config.entity.name.value);
        return CL_AMS_RC(CL_ERR_TRY_AGAIN);
    }

    readinessState = su->status.readinessState;

    clAmsPeSUComputeAdminState(su, &oldAdminState);

    CL_AMS_SET_A_STATE(su, CL_AMS_ADMIN_STATE_SHUTTINGDOWN);

    if ( oldAdminState == CL_AMS_ADMIN_STATE_UNLOCKED )
    {
        if ( readinessState != CL_AMS_READINESS_STATE_OUTOFSERVICE )
        {
            AMS_CALL ( clAmsPeSUSwitchoverWork(su, CL_AMS_ENTITY_SWITCHOVER_GRACEFUL) );
        }
        else
        {
            AMS_CALL ( clAmsPeSUShutdownCallback(su, CL_OK) );
        }
    }

    return CL_OK;
}

/*
 * clAmsPeSUShutdownCallback
 * -------------------------
 * This callback is invoked when a shutdown operation is completed.
 */

ClRcT
clAmsPeSUShutdownCallback(
        CL_IN       ClAmsSUT        *su,
        CL_IN       ClUint32T       error)
{
    ClAmsSGT *sg;
    ClAmsNodeT *node;

    AMS_CHECK_SU ( su );
    AMS_CHECK_SG ( sg = (ClAmsSGT *) su->config.parentSG.ptr );
    AMS_CHECK_NODE ( node = (ClAmsNodeT *) su->config.parentNode.ptr );

    AMS_FUNC_ENTER ( ("SU [%s]\n",su->config.entity.name.value) );

    if ( error )
    {
        /*
         * Any error in shutting down a SU should be resolved in the
         * switchover logic, so do nothing for now.
         */
    }

    if ( su->config.adminState == CL_AMS_ADMIN_STATE_SHUTTINGDOWN )
    {
        CL_AMS_SET_A_STATE(su, CL_AMS_ADMIN_STATE_LOCKED_A);
    }

    return CL_OK;
}

/*
 * clAmsPeSUAdminRestart
 * ---------------------
 * This informs AMS that the SU, if instantiated, should be terminated and
 * restarted. If the SU is restartable, any assignments to the SU are kept
 * as-is, otherwise, they are switched over if possible before the SU is
 * terminated and restarted. In this case, the SU may not be assigned the
 * same SIs or any SIs after restarting.
 */

ClRcT
clAmsPeSUAdminRestart(
        CL_IN   ClAmsSUT *su,
        CL_IN   ClUint32T switchoverMode)
{
    ClAmsSGT *sg = NULL;

    AMS_CHECK_SU ( su );

    AMS_CHECK_SG ( sg = (ClAmsSGT*)su->config.parentSG.ptr);

    AMS_FUNC_ENTER ( ("SU [%s]\n", su->config.entity.name.value) );

    AMS_ENTITY_LOG (su, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
            ("Admin Operation [Restart] on SU [%s]\n",
             su->config.entity.name.value));

    if(clAmsInvocationsPendingForSU(su))
    {
        clLogInfo("SU", "RESTART", 
                  "SU [%s] has invocations pending. Deferring restart",
                  su->config.entity.name.value);
        return CL_AMS_RC(CL_ERR_TRY_AGAIN);
    }

    if(clAmsInvocationsPendingForSG(sg))
    {
        clLogInfo("SU", "RESTART", 
                  "SG [%s] containing SU [%s] has invocations pending. Deferring restart",
                  sg->config.entity.name.value, su->config.entity.name.value);
        return CL_AMS_RC(CL_ERR_TRY_AGAIN);
    }

    if ( (su->status.presenceState != CL_AMS_PRESENCE_STATE_INSTANTIATED) ||
         (clAmsPeSUIsInstantiable(su) != CL_OK) )
    {
        AMS_ENTITY_LOG(su, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("SU [%s] is not instantiable or instantiated. Cannot Restart..\n",
             su->config.entity.name.value));

        return CL_AMS_RC(CL_ERR_NO_OP);
    }

    if ( !su->config.isRestartable )
    {
        AMS_ENTITY_LOG(su, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("SU [%s] is not restartable. Ignoring Restart..\n",
             su->config.entity.name.value));
        
        return CL_AMS_RC(CL_ERR_NO_OP);
    }

    return clAmsPeSURestart(su, switchoverMode);
}

/*
 * clAmsPeSURestart
 * ----------------
 * The real SU restart function called internally. Note, this fn and
 * its related callbacks assume that the SU is restartable.
 */

ClRcT
clAmsPeSURestart(
        CL_IN   ClAmsSUT *su,
        CL_IN   ClUint32T switchoverMode)
{
    ClBoolT cleanup = CL_FALSE;

    AMS_CHECK_SU ( su );

    AMS_FUNC_ENTER ( ("SU [%s]\n", su->config.entity.name.value) );

    if ( clAmsPeSUIsInstantiable(su) != CL_OK )
    {
        AMS_ENTITY_LOG(su, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("SU [%s] is not instantiable. Stopping in Restart/Termination\n",
             su->config.entity.name.value));

        return CL_OK;
    }

    AMS_ENTITY_LOG (su, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
            ("Restarting SU [%s]\n",
             su->config.entity.name.value));

    /*
     * Do cleanup/terminate, instantiate, reassign.
     */

    CL_AMS_SET_P_STATE(su, CL_AMS_PRESENCE_STATE_RESTARTING);

    if ( (switchoverMode & CL_AMS_ENTITY_SWITCHOVER_FAST) ||
         (su->status.operState == CL_AMS_OPER_STATE_DISABLED) )
    {
        cleanup = CL_TRUE;
    }

    if ( cleanup )
    {
        AMS_CALL ( clAmsPeSUCleanup(su) );
    }
    else
    {
        AMS_CALL ( clAmsPeSUTerminate(su) );
    }

    return CL_OK;
}

ClRcT
clAmsPeSURestartCallback_Step1(
        CL_IN   ClAmsSUT *su,
        CL_IN   ClUint32T error)
{
    ClAmsSGT *sg = NULL;

    AMS_CHECK_SU ( su );

    AMS_CHECK_SG ( sg = (ClAmsSGT*)su->config.parentSG.ptr);

    AMS_FUNC_ENTER ( ("SU [%s]\n", su->config.entity.name.value) );

    if ( error )
    {
        return clAmsPeSURestartCallback_Step2(su, error);
    }

    if ( clAmsPeSUIsInstantiable(su) != CL_OK )
    {
        AMS_ENTITY_LOG(su, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("SU [%s] is not instantiable. Stopping in Restart/Instantiation\n",
             su->config.entity.name.value));

        return CL_OK;
    }

    /*
     * Start the probation after the restart repair
     */
    if(sg->config.autoAdjust && su->status.recovery == CL_AMS_RECOVERY_SU_RESTART)
    {
        clLogInfo("SG", "ADJUST", "Starting SU [%s] adjustment probation timer after SU restart",
                  su->config.entity.name.value);
        AMS_CALL ( clAmsEntityTimerStart((ClAmsEntityT*)su, CL_AMS_SU_TIMER_PROBATION));
    }

    AMS_CALL ( clAmsPeSUInstantiate(su) );

    return CL_OK;
}

ClRcT
clAmsPeSURestartCallback_Step2(
        CL_IN   ClAmsSUT *su,
        CL_IN   ClUint32T error)
{

    AMS_CHECK_SU ( su );

    AMS_FUNC_ENTER ( ("SU [%s]\n", su->config.entity.name.value) );

    /*
     * If the restart operation fails and a pending fault exists, then
     * indicate to the fault handler that the operation has failed, else
     * report a new fault.
     */

    if ( error )
    {
        ClAmsRecoveryT recovery;
        ClUint32T escalation;

        if ( su->status.recovery == CL_AMS_RECOVERY_SU_RESTART )
        {
            AMS_CALL ( clAmsPeSUFaultCallback_Step2(su, error) );
        }

        escalation = clAmsPeEntityComputeFaultEscalation((ClAmsEntityT*) su);
        recovery = CL_AMS_RECOVERY_COMP_FAILOVER;
        return clAmsPeSUFaultReport(su, NULL, &recovery, &escalation);
    }

    if ( su->status.readinessState != CL_AMS_READINESS_STATE_INSERVICE )
    {
        AMS_ENTITY_LOG(su, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("SU [%s] is not inservice. Stopping in Restart/Reassignment\n",
             su->config.entity.name.value));

        return CL_OK;
    }

    if ( su->config.isPreinstantiable )
    {
        AMS_CALL ( clAmsPeSUAssignWorkAgain(su) );
    }

    return CL_OK;
}

//deprecated
ClRcT
clAmsPeSURestartCallback_Step3(
        CL_IN   ClAmsSUT *su,
        CL_IN   ClUint32T error)
{
    AMS_CHECK_SU ( su );

    AMS_FUNC_ENTER ( ("SU [%s]\n", su->config.entity.name.value) );

    if ( su->status.recovery == CL_AMS_RECOVERY_SU_RESTART )
    {
        AMS_CALL ( clAmsPeSUFaultCallback_Step2(su, error) );
    }

    return CL_OK;
}

/*
 * clAmsPeSUFaultReport
 * --------------------
 * This informs AMS that a fault is being reported against a SU. This fn
 * is also internally called to escalate a component fault to a SU.
 */

ClRcT
clAmsPeSUFaultReport(
                     CL_IN       ClAmsSUT                    *su,
                     CL_IN       ClAmsCompT                  *faultyComp,
                     CL_INOUT    ClAmsLocalRecoveryT         *recovery,
                     CL_INOUT    ClUint32T                   *escalation)
{
    ClAmsSGT *sg;
    ClAmsNodeT *node;
    ClAmsEntityRefT *entityRef;
    ClAmsLocalRecoveryT recommendedRecovery;
    
    AMS_CHECKPTR ( !recovery || !escalation );

    if(*escalation != CL_TRUE && *escalation != CL_FALSE)
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);

    AMS_CHECK_SU ( su );
    AMS_CHECK_SG ( sg = (ClAmsSGT *) su->config.parentSG.ptr );
    AMS_CHECK_NODE ( node = (ClAmsNodeT *) su->config.parentNode.ptr );

    AMS_FUNC_ENTER ( ("SU [%s]\n", su->config.entity.name.value) );

    /*
     * Replay all pending operations against this faulty SU
     */
    clAmsPeEntityOpsReplay(&su->config.entity, &su->status.entity, CL_TRUE);

    /*
     * A fault cannot be reported on a SU that has not been started.
     */

    if ( su->status.presenceState == CL_AMS_PRESENCE_STATE_UNINSTANTIATED )
    {
        clLogDebug(CL_LOG_AREA_AMS, CL_LOG_CONTEXT_AMS_FAULT_SU,
                   "Fault on SU [%s]: Presence State [Uninstantiated]."\
                   "Ignoring fault..",
                   su->config.entity.name.value);

        return CL_OK;
    }

    /*
     * Find the component on which the fault was reported.
     */
    if(!faultyComp)
    {

        for ( entityRef = clAmsEntityListGetFirst(&su->config.compList);
              entityRef != (ClAmsEntityRefT *) NULL;
              entityRef = clAmsEntityListGetNext(&su->config.compList, entityRef) )
        {
            ClAmsCompT *comp = (ClAmsCompT *) entityRef->ptr;
            
            if ( comp->status.recovery )
            {
                faultyComp = comp;
                break;
            }
        }

        if (!faultyComp )
        {
            clLogInfo(CL_LOG_AREA_AMS, CL_LOG_CONTEXT_AMS_FAULT_SU,
                      "Fault on SU [%s]/Component [Unknown]."\
                      "Setting Escalation = [False]",
                      su->config.entity.name.value);

            *escalation = CL_FALSE;

            /*
             * This is true when a fault is reported for a SU. The following is a
             * hack. Since FM does not understand SUs, we will select the first 
             * component in the SU as the supposed cause of the problem.
             */

            entityRef = clAmsEntityListGetFirst(&su->config.compList);

            if ( !entityRef )
            {
                clLogError(CL_LOG_AREA_AMS, CL_LOG_CONTEXT_AMS_FAULT_SU,
                           "Fault on SU [%s] but SU has no components?"\
                           " Ignoring Fault..",
                           su->config.entity.name.value);

                return CL_OK;
            }

            AMS_CHECK_COMP ( faultyComp = (ClAmsCompT *) entityRef->ptr );
        }
    }

    /*
     * Compute recovery action, taking into account SU level escalation rules.
     * Update the status of recovery for the faulty component.
     */

    recommendedRecovery = *recovery;

    AMS_CALL ( clAmsPeSUComputeRecoveryAction(su, recovery, escalation) );

    if ( *recovery == CL_AMS_RECOVERY_NONE )
    {
        clLogDebug(CL_LOG_AREA_AMS, CL_LOG_CONTEXT_AMS_FAULT_SU,
                   "Fault on SU [%s]: Recommended recovery = [%s],"\
                   " Computed recovery = [%s]. Ignoring fault",
                   su->config.entity.name.value,
                   CL_AMS_STRING_RECOVERY(recommendedRecovery),
                   CL_AMS_STRING_RECOVERY(*recovery));

        return CL_OK;
    }

    faultyComp->status.recovery = *recovery;

    clLogDebug(CL_LOG_AREA_AMS, CL_LOG_CONTEXT_AMS_FAULT_SU,
               "Fault on SU [%s]/Component [%s]:"\
               " Recommended recovery = [%s], Computed recovery = [%s],"\
               " Escalation = [%s]",
               su->config.entity.name.value,
               faultyComp->config.entity.name.value,
               CL_AMS_STRING_RECOVERY(recommendedRecovery),
               CL_AMS_STRING_RECOVERY(*recovery),
               *escalation ? "True" : "False");

    /*
     * Stop all possible timers that could be running for the SU
     * and clear the invocation list.
     */

    AMS_CALL ( clAmsEntityClearOps((ClAmsEntityT *)su) );

    /*
     * Delete pending reassign ops against this SU.
     */
    clAmsPeSUSIReassignEntryDelete(su);
    /*
     * Clear the entity stack operations.
     */
    clAmsEntityOpsClear((ClAmsEntityT*)su, &su->status.entity);

    if(*recovery != CL_AMS_RECOVERY_INTERNALLY_RECOVERED)
    {
        CL_AMS_SET_O_STATE(su, CL_AMS_OPER_STATE_DISABLED);
    }

    /*
     * Undertake recovery actions
     */

    switch ( *recovery )
    {
    case CL_AMS_RECOVERY_SU_RESTART:
        {
            if ( !su->status.suRestartTimer.count )
            {
                AMS_CALL ( clAmsEntityTimerStart((ClAmsEntityT *) su,
                                                 CL_AMS_SU_TIMER_SURESTART) );
            }

            CL_AMS_SET_O_STATE(faultyComp, CL_AMS_OPER_STATE_ENABLED);
            CL_AMS_SET_O_STATE(su, CL_AMS_OPER_STATE_ENABLED);

            AMS_CALL ( clAmsPeSURestart(su, CL_AMS_ENTITY_SWITCHOVER_FAST) );

            break;
        }

    case CL_AMS_RECOVERY_INTERNALLY_RECOVERED:
        {
            su->status.operState = CL_AMS_OPER_STATE_DISABLED;
            for ( entityRef = clAmsEntityListGetFirst(&su->config.compList);
                  entityRef != (ClAmsEntityRefT *) NULL;
                  entityRef = clAmsEntityListGetNext(&su->config.compList, entityRef) )
            {
                ClAmsCompT *comp = (ClAmsCompT*)entityRef->ptr;
                comp->status.operState = CL_AMS_OPER_STATE_DISABLED;
            }

            AMS_CALL ( clAmsPeSUSwitchoverWork(su, CL_AMS_ENTITY_SWITCHOVER_FAST) );
            /*
             * A component failover implies a SU failover+switchover. Switch
             * over the CSIs that are assigned to working components, and fail
             * over the ones assigned to the failed component. Then terminate
             * and/or cleanup the SU.
             */
            break;
        }

    case CL_AMS_RECOVERY_COMP_FAILOVER:
        {
            if ( !node->status.suFailoverTimer.count )
            {
                AMS_CALL ( clAmsEntityTimerStart((ClAmsEntityT *) node,
                                                 CL_AMS_NODE_TIMER_SUFAILOVER) );
            }
            AMS_CALL ( clAmsPeSUSwitchoverWork(su, CL_AMS_ENTITY_SWITCHOVER_IMMEDIATE) );

            break;
        }

    case CL_AMS_RECOVERY_NODE_SWITCHOVER:
    case CL_AMS_RECOVERY_NODE_FAILOVER:
    case CL_AMS_RECOVERY_NODE_FAILFAST:
        {
            AMS_CALL ( clAmsPeNodeFaultReport(node, faultyComp, recovery, escalation) );

            break;
        }

    case CL_AMS_RECOVERY_CLUSTER_RESET:
        {
            AMS_CALL ( clAmsPeClusterFaultReport(&gAms, recovery, escalation) );

            break;
        }

    default:
        {
            return CL_AMS_RC(CL_ERR_NOT_IMPLEMENTED);
        }
    }

    return CL_OK;
}

/*
 * clAmsPeSUFaultCallback_Step1
 * ----------------------------
 * This fn is internally invoked by AMS once a comp failover has been
 * processed.
 */

ClRcT
clAmsPeSUFaultCallback_Step1(
        CL_IN       ClAmsSUT            *su,
        CL_IN       ClUint32T           error)
{
    ClAmsSGT *sg;

    AMS_CHECK_SU ( su );
    AMS_CHECK_SG ( sg = (ClAmsSGT *) su->config.parentSG.ptr );

    AMS_FUNC_ENTER ( ("SU [%s]\n", su->config.entity.name.value) );

    clLogInfo(CL_LOG_AREA_AMS, CL_LOG_CONTEXT_AMS_FAULT_SU, "Fault on SU [%s] : Recovery [%s] :Step 1 Cleaning up",
                    su->config.entity.name.value, CL_AMS_STRING_RECOVERY(su->status.recovery));

    AMS_CALL ( clAmsPeSUCleanup(su) );

    // XXX probably redundant, may want to remove in future

    // for fully saf aware SUs, cleanup is sync, so this will be executed after 
    // the step2 of fault callback but if there is any async cleanup, then this 
    // will be executed before step2. So autorepair could happen before or after 
    // this evaluate and that will dictate which su is brought into service if
    // there is also a spare SU.

    AMS_CALL ( clAmsPeSGEvaluateWork(sg) );

    return CL_OK;
}

/*
 * clAmsPeSUFaultCallback_Step2
 * ----------------------------
 * This fn is internally invoked by AMS once a fault on a SU has been
 * processed. Faults handled by this fn are: SU restart, Comp Failover.
 */

ClRcT
clAmsPeSUFaultCallback_Step2(
                             CL_IN       ClAmsSUT            *su,
                             CL_IN       ClUint32T           error)
{
    ClAmsSGT *sg;
    ClAmsEntityRefT *entityRef;
    ClBoolT repairNecessary;
    ClAmsRecoveryT recovery;
    ClAmsCompT *faultyComp = NULL;
    ClBoolT faultReport = CL_TRUE;

    AMS_CHECK_SU ( su );
    AMS_CHECK_SG ( sg = (ClAmsSGT *) su->config.parentSG.ptr );

    AMS_FUNC_ENTER ( ("SU [%s]\n", su->config.entity.name.value) );

    /*
     * Start with the assumption that repair is necessary. Save a copy of
     * the recovery action taken as well as the component on which the
     * original fault was generated.
     */

    repairNecessary = CL_TRUE;
    recovery = su->status.recovery;

    for ( entityRef = clAmsEntityListGetFirst(&su->config.compList);
          entityRef != (ClAmsEntityRefT *) NULL;
          entityRef = clAmsEntityListGetNext(&su->config.compList, entityRef) )
    {
        ClAmsCompT *comp = (ClAmsCompT *) entityRef->ptr;

        if ( comp->status.recovery )
        {
            faultyComp = comp;
            break;
        }
    }

    if ( !faultyComp )
    {
        clLogError(CL_LOG_AREA_AMS, CL_LOG_CONTEXT_AMS_FAULT_SU,
                   "Fault raised on SU [%s] but faulty component not found. "\
                   "Ignoring Fault..",
                   su->config.entity.name.value);

        return CL_OK;
    }

    /*
     * If there was no error in the recovery, reset the state of the SU and its
     * components and undertake auto repair if necessary. If auto repair is
     * successful, then no repair action from FM is needed but we still need to
     * report the fault.
     */

    if ( error )
    {
        clLogError(CL_LOG_AREA_AMS, CL_LOG_CONTEXT_AMS_FAULT_SU,
                   "Fault on SU [%s]/Component [%s]: Recovery [%s] Failed",
                   su->config.entity.name.value,
                   faultyComp->config.entity.name.value,
                   CL_AMS_STRING_RECOVERY(su->status.recovery));

        /* If SU restart failed, we need to failover.  Reset this SU so it can become standby */
        if (su->status.recovery == CL_AMS_RECOVERY_SU_RESTART)
        {           
          AMS_CALL ( clAmsPeSUReset(su) );
          AMS_CALL ( clAmsPeSUMarkInstantiable(su) );
        }        

        CL_AMS_SET_O_STATE(su, CL_AMS_OPER_STATE_DISABLED);
    }
    else
    {
        clLogDebug(CL_LOG_AREA_AMS, CL_LOG_CONTEXT_AMS_FAULT_SU,
                   "Fault on SU [%s]/Component [%s]: Recovery [%s] Successful",
                   su->config.entity.name.value,
                   faultyComp->config.entity.name.value,
                   CL_AMS_STRING_RECOVERY(su->status.recovery));

        if ( su->status.recovery == CL_AMS_RECOVERY_SU_RESTART )
        {
            repairNecessary = CL_FALSE;

            su->status.recovery = CL_AMS_RECOVERY_NONE;
            
            for ( entityRef = clAmsEntityListGetFirst(&su->config.compList);
                  entityRef != (ClAmsEntityRefT *) NULL;
                  entityRef = clAmsEntityListGetNext(&su->config.compList, entityRef) )
            {
                ClAmsCompT *comp = (ClAmsCompT *) entityRef->ptr;

                comp->status.recovery = CL_AMS_RECOVERY_NONE;
            }
        }

        if ( su->status.recovery == CL_AMS_RECOVERY_COMP_FAILOVER )
        {
            if ( sg->config.autoRepair )
            {
                ClRcT rc = CL_OK;
                clLogDebug(CL_LOG_AREA_AMS, CL_LOG_CONTEXT_AMS_FAULT_SU,
                           "Fault on SU [%s]/Component [%s]: "\
                           "Starting Auto Repair",
                           su->config.entity.name.value,
                           faultyComp->config.entity.name.value);

                AMS_CALL ( clAmsPeSUReset(su) );
                AMS_CALL ( clAmsPeSUMarkInstantiable(su) );
                /*
                 * Start the auto adjust probation timer for this SU
                 */
                if(sg->config.autoAdjust
                   &&
                   (su->config.rank ||
                    sg->config.redundancyModel == CL_AMS_SG_REDUNDANCY_MODEL_CUSTOM))
                {
                    clLogInfo("SG", "ADJUST", "Starting SU [%s] adjustment probation timer after SU fault repair",
                              su->config.entity.name.value);

                    AMS_CALL ( clAmsEntityTimerStart((ClAmsEntityT*)su,
                                                     CL_AMS_SU_TIMER_PROBATION) );
                }
                if ( (rc = clAmsPeSUEvaluateWork(su)) == CL_OK )
                {
                    repairNecessary = CL_FALSE;
                }
            }
        }

        if ( su->status.recovery == CL_AMS_RECOVERY_INTERNALLY_RECOVERED )
        {
            clLogDebug(CL_LOG_AREA_AMS, CL_LOG_CONTEXT_AMS_FAULT_SU,
                       "SU [%s] underwent internal fast recovery: ",
                       su->config.entity.name.value);
            AMS_CALL ( clAmsPeSUReset(su) );
            AMS_CALL ( clAmsPeSUMarkInstantiable(su) );
            faultReport = CL_FALSE;
        }

    }

#ifdef AMS_CPM_INTEGRATION

    if(faultReport)
    {
        ClRcT rc = CL_OK;

        clLogDebug(CL_LOG_AREA_AMS, CL_LOG_CONTEXT_AMS_FAULT_SU,
                   "Fault on SU [%s]/Component [%s]: Reporting fault to FM. "\
                   "Recovery [%s], Repair Necessary [%s]",
                   su->config.entity.name.value,
                   faultyComp->config.entity.name.value,
                   CL_AMS_STRING_RECOVERY(recovery),
                   CL_AMS_STRING_BOOLEAN(repairNecessary));

        if ( (rc = _clAmsSAFaultReportCallback(
                                               (ClAmsEntityT *)faultyComp,
                                               recovery,
                                               repairNecessary)) != CL_OK )
        { 
            clLogWarning(CL_LOG_AREA_AMS, CL_LOG_CONTEXT_AMS_FAULT_SU,
                         "Fault on SU [%s]/Component [%s]: Error [0x%x] "   \
                         "when reporting fault to FM. Continuing..",
                         su->config.entity.name.value,
                         faultyComp->config.entity.name.value,
                         rc);
        }
    }

#endif

    return CL_OK;
}

/*
 * clAmsPeSUComputeRecoveryAction
 * ------------------------------
 * Given a su and a recommended recovery action, this fn computes
 * an appropriate recovery taking escalation rules into account. It 
 * does not initiate the recovery action. 
 */

ClRcT
clAmsPeSUComputeRecoveryAction(
        CL_IN       ClAmsSUT                    *su,
        CL_INOUT    ClAmsLocalRecoveryT         *recovery,
        CL_OUT      ClUint32T                   *escalation)
{
    ClAmsSGT *sg;
    ClAmsNodeT *node;
    ClBoolT match = CL_FALSE;

    AMS_CHECKPTR ( !recovery || !escalation );
    AMS_CHECK_SU ( su );
    AMS_CHECK_SG ( sg = (ClAmsSGT *) su->config.parentSG.ptr );
    AMS_CHECK_NODE ( node = (ClAmsNodeT *) su->config.parentNode.ptr );

    AMS_FUNC_ENTER ( ("SU [%s]\n", su->config.entity.name.value) );

    if ( (*recovery == CL_AMS_RECOVERY_NO_RECOMMENDATION) ||
         (*recovery == CL_AMS_RECOVERY_COMP_RESTART) )
    {
        match = CL_TRUE;

        *recovery = su->config.isRestartable ?  
                        CL_AMS_RECOVERY_SU_RESTART : 
                        CL_AMS_RECOVERY_COMP_FAILOVER;
    }

    if ( *escalation == CL_FALSE )
    {
        if ( clAmsPeEntityRecoveryScopeLarger(
                    su->status.recovery, *recovery) || 
             clAmsPeEntityRecoveryScopeLarger(
                    node->status.recovery, *recovery) )
        {
            *recovery = CL_AMS_RECOVERY_NONE;
        }
    }

    if ( *recovery == CL_AMS_RECOVERY_SU_RESTART )
    {
        match = CL_TRUE;

        if ( su->config.isRestartable )
        {
            su->status.suRestartCount ++;

            if ( su->status.suRestartCount > sg->config.suRestartCountMax )
            {
                *recovery = CL_AMS_RECOVERY_COMP_FAILOVER;
                *escalation = CL_TRUE;
            }
        }
        else
        {
            *recovery = CL_AMS_RECOVERY_COMP_FAILOVER;
            *escalation = CL_TRUE;
        }
    }

    if ( *recovery == CL_AMS_RECOVERY_COMP_FAILOVER )
    {
        match = CL_TRUE;

        node->status.suFailoverCount++;

        if ( node->status.suFailoverCount > node->config.suFailoverCountMax )
        {
            *recovery = CL_AMS_RECOVERY_NODE_FAILOVER;
            *escalation = CL_TRUE;
        }
    }

    if( *recovery == CL_AMS_RECOVERY_INTERNALLY_RECOVERED )
    {
        match = CL_TRUE;
    }

    if ( *recovery == CL_AMS_RECOVERY_NODE_SWITCHOVER )
    {
        match = CL_TRUE;
    }

    if ( *recovery == CL_AMS_RECOVERY_NODE_FAILOVER )
    {
        match = CL_TRUE;
    }

    if ( *recovery == CL_AMS_RECOVERY_NODE_FAILFAST )
    {
        match = CL_TRUE;
    }

    if ( *recovery == CL_AMS_RECOVERY_APP_RESTART )
    {
        // no escalation rules for now
    }

    if ( *recovery == CL_AMS_RECOVERY_CLUSTER_RESET )
    {
        match = CL_TRUE;
    }

    if ( !*escalation && *recovery == su->status.recovery )
    {
        match = CL_FALSE;
    }

    if ( !match )
    {
        *recovery = CL_AMS_RECOVERY_NONE;
    }

    if ( *recovery != CL_AMS_RECOVERY_NONE )
    {
        su->status.recovery = *recovery;
    }

    return CL_OK;
}

/*
 * clAmsPeSUCompRestartTimeout
 * ---------------------------
 * This fn is called when a SU's component restart probation timer pops. If 
 * within this timeout, more than a certain number of component restarts 
 * take place, the AMS escalates the recovery action and cancels the timer. 
 * Since this fn is only called when the timer pops, the fn resets the restart 
 * counter.
 */

ClRcT
clAmsPeSUCompRestartTimeout( 
        CL_IN ClAmsEntityTimerT  *timer) 
{
    ClAmsSUT *su;

    AMS_CHECKPTR ( !timer );
    AMS_CHECK_SU ( su = (ClAmsSUT *) timer->entity );

    AMS_FUNC_ENTER ( ("SU [%s]\n",su->config.entity.name.value) );

    su->status.compRestartCount = 0;

    AMS_CALL ( clAmsEntityTimerStop((ClAmsEntityT *) su,
                    CL_AMS_SU_TIMER_COMPRESTART) );

    return CL_OK;
}

/*
 * clAmsPeSUSURestartTimeout
 * -------------------------
 * This fn is called when a SU's SU restart probation timer pops. If within
 * this timeout, more than a certain number of SU restarts take place, the 
 * AMS escalates the recovery action and cancels the timer. Since this fn 
 * is only called when the timer pops, the fn resets the restart counter.
 */

ClRcT
clAmsPeSUSURestartTimeout(
        CL_IN ClAmsEntityTimerT  *timer) 
{
    ClAmsSUT *su;

    AMS_CHECKPTR ( !timer );
    AMS_CHECK_SU ( su = (ClAmsSUT *) timer->entity );

    AMS_FUNC_ENTER ( ("SU [%s]\n",su->config.entity.name.value) );

    su->status.suRestartCount = 0;
    AMS_CALL ( clAmsEntityTimerStop((ClAmsEntityT *) su,
                    CL_AMS_SU_TIMER_SURESTART) );

    return CL_OK;
}

/*
 * clAmsPeSURepaired
 * -----------------
 * This informs AMS that a faulty SU has been externally repaired and can 
 * now be put back into service.
 */

ClRcT
clAmsPeSURepaired(
        CL_IN   ClAmsSUT *su)
{
    ClAmsSGT *sg = NULL;

    AMS_FUNC_ENTER ( ("SU [%s]\n", su->config.entity.name.value) );

    AMS_CHECK_SG ( sg = (ClAmsSGT*)su->config.parentSG.ptr);

    if ( (su->status.recovery == CL_AMS_RECOVERY_NONE) &&
         (su->status.operState == CL_AMS_OPER_STATE_ENABLED) )
    {
        AMS_ENTITY_LOG (su, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("Admin Operation [Repaired] on SU [%s] in Oper State [%s], Recovery [%s]. Ignoring...\n",
             su->config.entity.name.value,
             CL_AMS_STRING_O_STATE(su->status.operState),
             CL_AMS_STRING_RECOVERY(su->status.recovery)));

        return CL_AMS_RC(CL_ERR_NO_OP);
    }

    AMS_ENTITY_LOG (su, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
            ("Admin Operation [Repaired] on SU [%s]\n",
             su->config.entity.name.value));

    clAmsPeSUReset(su);

    AMS_CALL ( clAmsPeSUMarkInstantiable(su) );

    /*
     * Start the auto adjust probation for this SU after the repair.
     */
    if(sg->config.autoAdjust && 
       (su->config.rank || sg->config.redundancyModel == CL_AMS_SG_REDUNDANCY_MODEL_CUSTOM))
    {
        clLogInfo("SG", "ADJUST", "Starting SU [%s] adjustment probation timer after SU repair",
                  su->config.entity.name.value);

        AMS_CALL(clAmsEntityTimerStart((ClAmsEntityT*)su, CL_AMS_SU_TIMER_PROBATION));
    }

    AMS_CALL ( clAmsPeSUEvaluateWork(su) );

    return CL_OK;
}

/*-----------------------------------------------------------------------------
 * SU life cycle management functions
 *---------------------------------------------------------------------------*/

ClRcT
clAmsPeSUInstantiate2(ClAmsSUT *su, ClUint32T *pNumComponents)
{
    ClAmsEntityRefT *entityRef = NULL;
    ClUint32T limit = su->status.instantiateLevel + 1;
    ClUint32T lastInstantiateLevel = 0;
    ClUint32T numComponents = 0;
    for ( entityRef = clAmsEntityListGetFirst(&su->config.compList);
          entityRef != (ClAmsEntityRefT *) NULL;
          entityRef = clAmsEntityListGetNext(&su->config.compList, entityRef) )
    {
        ClAmsCompT *comp = (ClAmsCompT *) entityRef->ptr;

        if (!comp)
        {
            clDbgCodeError(CL_ERR_NULL_POINTER,("NULL component ref on the su comp list"));
            continue;  /* A NULL comp ref might indicate an entity that was freed while still on the list, but attempt to ignore the entity and continue with the next */
        }
        
        if ( (comp->config.property == CL_AMS_COMP_PROPERTY_SA_AWARE) ||
             (comp->config.property == CL_AMS_COMP_PROPERTY_PROXIED_PREINSTANTIABLE) )
        {
            ClRcT rc;
            ClUint32T instantiateLevel = comp->config.instantiateLevel;
            ClUint32T curInstantiateCount = comp->status.instantiateCount;

            if(instantiateLevel < limit) continue;

            if(!lastInstantiateLevel)
            {
                lastInstantiateLevel = instantiateLevel;
            }
            else if(lastInstantiateLevel != instantiateLevel)
            {
                break;
            }
            rc = clAmsPeCompInstantiate(comp);
            if (rc != CL_OK)
            {
                /* Lower level has logged the root cause issue so just continue to the next comp */
                continue;
            }
            
            if((comp->status.instantiateCount != curInstantiateCount) ||
               clAmsEntityTimerIsRunning((ClAmsEntityT*)comp, CL_AMS_COMP_TIMER_INSTANTIATEDELAY))
            {
                su->status.numPIComp++;
                ++numComponents;
            }
        }
    }

    if(lastInstantiateLevel) 
        su->status.instantiateLevel = lastInstantiateLevel;

    if(pNumComponents)
        *pNumComponents = numComponents;

    clLogInfo("SU", "INST", "SU [%s] instantiated [%d] components at level [%d]", su->config.entity.name.value, numComponents, su->status.instantiateLevel);

    return CL_OK;
}

/*
 * clAmsPeSUInstantiate
 * --------------------
 * Instantiate a SU. For preinstantiable SUs, only preinstantiable components 
 * are started, the other components will be started when work needs to be
 * assigned to this SU. For non-preinstantiable SUs, the SU is marked as
 * ready and then work is reevaluated for the SU. This results in the
 * components in the SU being assigned CSIs, which results in them being
 * instantiated.
 */

ClRcT clAmsPeSUInstantiate(CL_IN       ClAmsSUT        *su)
{
    ClAmsNodeT *node;

    AMS_CHECK_SU ( su );
    AMS_CHECK_NODE ( node = (ClAmsNodeT *) su->config.parentNode.ptr );

    AMS_FUNC_ENTER ( ("SU [%s]\n",su->config.entity.name.value) );

    if ( node->config.isASPAware
         &&
        (su->status.presenceState != CL_AMS_PRESENCE_STATE_UNINSTANTIATED) &&
         (su->status.presenceState != CL_AMS_PRESENCE_STATE_RESTARTING) )
    {
        AMS_ENTITY_LOG(su, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("SU [%s] is in Presence State [%s]. Ignoring instantiate request..\n",
            su->config.entity.name.value,
            CL_AMS_STRING_P_STATE(su->status.presenceState)));

        return CL_OK;
    }

    if( clAmsPeSUIsInstantiable(su) != CL_OK )
    {
        AMS_ENTITY_LOG(su, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("SU [%s] is not instantiable. Ignoring instantiate request..\n",
             su->config.entity.name.value));

        return CL_OK;
    }

    if(!node->config.isASPAware)
    {
        clAmsPeSUMarkInstantiable(su);
    }

    /*
     * Update SU state
     */

    AMS_ENTITY_LOG(su, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
        ("Instantiating SU [%s] in Presence State [%s]\n",
         su->config.entity.name.value,
         CL_AMS_STRING_P_STATE(su->status.presenceState)) ); 

    su->status.numInstantiatedComp = 0;

    if ( su->status.presenceState != CL_AMS_PRESENCE_STATE_RESTARTING )
    {
        CL_AMS_SET_P_STATE(su, CL_AMS_PRESENCE_STATE_INSTANTIATING);
    }

    AMS_CALL ( clAmsPeSUMarkInstantiated(su) );

    CL_AMS_SET_EPOCH(su);

    /*
     * If the SU is preinstantiable, then instantiate the SA Aware and Proxied
     * preinstantiable components. Other types of components will be started
     * when work needs to be assigned.
     */

    if ( su->config.isPreinstantiable )
    {
        /*
         * This is a preinstantiable SU; has at least 1 preinstantiable component.
         */

        su->status.numPIComp = 0;
        su->status.instantiateLevel = 0;
        AMS_CALL( clAmsPeSUInstantiate2(su, NULL) );
        /*
         * If SU is non-preinstantiable or is preinstantiable but has no 
         * preinstantiable components (which is a modeling error), then
         * go to the next step of instantiation.
         */

        if ( !su->status.numPIComp )
        {
            AMS_ENTITY_LOG(su, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_ERROR,
                ("Error: Preinstantiable SU [%s] has 0 PI components. Exiting..\n",
                 su->config.entity.name.value));

            return CL_AMS_RC(CL_AMS_ERR_BAD_CONFIG);
        }
    }
    else
    {
        /*
         * This is a non-preinstantiable SU; has zero preinstantiable components.
         */
        CL_AMS_SET_P_STATE(su, CL_AMS_PRESENCE_STATE_INSTANTIATED);
        AMS_CALL ( clAmsPeSUMarkReady(su) );
        AMS_CALL ( clAmsPeSUEvaluateWork(su) );
    }

    return CL_OK;
}


/*
 * clAmsPeSUInstantiateCallback
 * ----------------------------
 * This fn is called whenever a constituent component receives an instantiate
 * callback or timeout indicating the instantiation has succeeded or failed. 
 */

ClRcT
clAmsPeSUInstantiateCallback(
        CL_IN   ClAmsSUT    *su,
        CL_IN   ClRcT       error) 
{
    ClAmsNodeT  *node;
    ClAmsPresenceStateT pstate;

    AMS_CHECK_SU ( su );
    AMS_CHECK_NODE ( (node = (ClAmsNodeT *) su->config.parentNode.ptr) );

    AMS_FUNC_ENTER ( ("SU [%s]\n",su->config.entity.name.value) );

    /*
     * Ensure that the SU is in the correct state. A SU can be in the 
     * instantiation-failed state if one of the components failed
     * instantiation prior to getting this callback for another component.
     * A SU can be in UNINSTANTIATED state for NON-PROXIED components.
     * We let this condition through so the component count is properly
     * updated.
     */

    if ( (su->status.presenceState != CL_AMS_PRESENCE_STATE_INSTANTIATING) &&
         (su->status.presenceState != CL_AMS_PRESENCE_STATE_RESTARTING) &&
         (su->status.presenceState != CL_AMS_PRESENCE_STATE_INSTANTIATION_FAILED) &&
         (su->status.presenceState != CL_AMS_PRESENCE_STATE_UNINSTANTIATED) )
    {
        AMS_ENTITY_LOG(su, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
            ("SU [%s] is [%s]. Ignoring instantiate callback..\n",
            su->config.entity.name.value,
            CL_AMS_STRING_P_STATE(su->status.presenceState)));

        return CL_OK;
    }

    if ( error )
    {
        return clAmsPeSUInstantiateError(su, error);
    }

    /*
     * A new component has instantiated. The SU is considered instantiated
     * for a preinstantiable SU, if all preinstantiable components have
     * instantiated. Otherwise all components must instantiate.
     */

    if ( su->config.isPreinstantiable )
    {
        ClUint32T numComponents = 0;
        if ( su->status.numInstantiatedComp != su->status.numPIComp )
        {
            return CL_OK;
        }
        /*
         * Instantiate in the next level.
         */
        AMS_CALL(clAmsPeSUInstantiate2(su, &numComponents));
        if(numComponents)
        {
            return CL_OK;
        }
    }
    else
    {
        if ( su->status.numInstantiatedComp < su->config.numComponents )
        {
            return CL_OK;
        }
    }

    AMS_ENTITY_LOG(su, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
        ("SU [%s] is now instantiated\n",
         su->config.entity.name.value));

    if ( su->status.presenceState == CL_AMS_PRESENCE_STATE_INSTANTIATING )
    {
        node->status.numInstantiatedSUs ++;
    }

    AMS_ENTITY_LOG(node, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
        ("Node [%s] now has [%d] instantiated + restarting SUs\n",
         node->config.entity.name.value,
         node->status.numInstantiatedSUs));

    // WORKQUEUE 

    pstate = su->status.presenceState;

    CL_AMS_SET_P_STATE(su, CL_AMS_PRESENCE_STATE_INSTANTIATED);

    if ( su->config.isPreinstantiable )
    {
        AMS_CALL ( clAmsPeSUMarkReady(su) );

        if ( pstate == CL_AMS_PRESENCE_STATE_RESTARTING )
        {
            AMS_CALL ( clAmsPeSURestartCallback_Step2(su, CL_OK) );
        }
        else
        {
            AMS_CALL ( clAmsPeNodeInstantiateCallback(node, CL_OK) );
            AMS_CALL ( clAmsPeSUEvaluateWork(su) );
        }
    }
    else
    {
        if ( pstate == CL_AMS_PRESENCE_STATE_RESTARTING )
        {
            AMS_CALL ( clAmsPeSURestartCallback_Step2(su, CL_OK) );
        }
        else
        {
            AMS_CALL ( clAmsPeNodeInstantiateCallback(node, CL_OK) );
        }
    }

    return CL_OK;
}

/*
 * clAmsPeSUInstantiateError
 * -------------------------
 * This fn is called whenever a constituent component receives an instantiate
 * error or timeout indicating the instantiation has failed. 
 */

ClRcT
clAmsPeSUInstantiateError(
        CL_IN   ClAmsSUT    *su,
        CL_IN   ClRcT       error) 
{
    ClAmsNodeT  *node;
    ClAmsPresenceStateT pstate;

    AMS_CHECK_SU ( su );
    AMS_CHECK_NODE ( (node = (ClAmsNodeT *) su->config.parentNode.ptr) );

    AMS_FUNC_ENTER ( ("SU [%s]\n",su->config.entity.name.value) );

    /*
     * Ensure that the SU is in the correct state. A SU can be in the 
     * instantiation-failed state if one of the components failed
     * instantiation prior to getting this callback for another component.
     * We let this condition through so the component count is properly
     * updated.
     */

    if ( (su->status.presenceState != CL_AMS_PRESENCE_STATE_INSTANTIATING) &&
         (su->status.presenceState != CL_AMS_PRESENCE_STATE_RESTARTING) &&
         (su->status.presenceState != CL_AMS_PRESENCE_STATE_INSTANTIATION_FAILED) )
    {
        AMS_ENTITY_LOG(su, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
            ("SU [%s] is [%s]. Ignoring instantiate callback..\n",
            su->config.entity.name.value,
            CL_AMS_STRING_P_STATE(su->status.presenceState)));

        return CL_OK;
    }

    pstate = su->status.presenceState;

    AMS_ENTITY_LOG(su, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_ERROR,
        ("Error [0x%x] in instantiating SU [%s]. Marking [instantiation-failed]\n",
         error,
         su->config.entity.name.value));

    CL_AMS_SET_P_STATE(su, CL_AMS_PRESENCE_STATE_INSTANTIATION_FAILED);
    CL_AMS_SET_O_STATE(su, CL_AMS_OPER_STATE_DISABLED);
    
    AMS_CALL ( clAmsPeSUMarkUninstantiated(su) );
    AMS_CALL ( clAmsPeSUMarkUninstantiable(su) );

    if ( pstate == CL_AMS_PRESENCE_STATE_RESTARTING )
    {
        if ( node->status.numInstantiatedSUs )
        {
            node->status.numInstantiatedSUs --;
        }

        AMS_CALL ( clAmsPeSURestartCallback_Step2(su, error) );
    }
    else
    {


        /*
         * Send notification for SU instantiation failure
         */

        ClAmsNotificationDescriptorT notification = {0};

        notification.type = CL_AMS_NOTIFICATION_SU_INSTANTIATION_FAILURE;
        notification.entityType = CL_AMS_ENTITY_TYPE_SU;
        memcpy ( &notification.entityName,&su->config.entity.name,
                sizeof(ClNameT) );
        notification.entityName.length = strlen(su->config.entity.name.value);
        notification.recoveryActionTaken = CL_AMS_RECOVERY_NONE;
        notification.repairNecessary = CL_TRUE;

        AMS_CALL_PUBLISH_NTF ( clAmsNotificationEventPublish(&notification ) );

        AMS_CALL ( clAmsPeNodeInstantiateCallback(node, error) );
        AMS_CALL ( clAmsPeSUCleanup(su));

    }

    return CL_OK;
}

ClRcT clAmsPeSUTerminate2(ClAmsSUT *su)
{
    ClAmsEntityRefT *entityRef = NULL;
    ClUint32T lastInstantiateLevel = 0;
    ClUint32T curInstantiateLevel = 0;
    ClBoolT responsePending = CL_FALSE;
    ClBoolT reset = CL_TRUE;
    ClRcT rc = CL_OK;
    static ClBoolT reentrant = CL_FALSE;
    
    if(reentrant == CL_TRUE)
        return CL_OK;

    reentrant = CL_TRUE;

    clLogInfo("SU", "TERMINATE", "SU [%s] terminate for level [%d]",
              su->config.entity.name.value, su->status.instantiateLevel);

    for ( entityRef = clAmsEntityListGetLast(&su->config.compList);
          entityRef != (ClAmsEntityRefT *) NULL;
          entityRef = clAmsEntityListGetPrevious(&su->config.compList, entityRef) )
    {
        ClAmsCompT *sucomp = (ClAmsCompT *) entityRef->ptr;
        ClUint32T instantiateLevel = 0;

        if(!sucomp || sucomp->config.entity.type != CL_AMS_ENTITY_TYPE_COMP)
        {
            rc = CL_AMS_RC(CL_AMS_ERR_INVALID_ENTITY);
            goto exitfn;
        }

        instantiateLevel = sucomp->config.instantiateLevel;
        if(su->status.instantiateLevel != instantiateLevel) 
        {
            if(!lastInstantiateLevel) continue;
            su->status.instantiateLevel = instantiateLevel;
            if(responsePending)
            {
                goto exitfn;
            }
        }
        if(!lastInstantiateLevel) 
            lastInstantiateLevel = instantiateLevel;

        curInstantiateLevel = su->status.instantiateLevel;
        AMS_CHECK_RC_ERROR ( clAmsPeCompShutdown(sucomp, &responsePending) );
        /*
         * Instantiate level has changed which mostly implies that the SU was instantiated
         * or restarted. Avoid resetting the instantiate level. 
         * If instantiate level flipped back and forth, then its a restart for the SU which is taken
         * care by a suinstantiate2 in instantiatecallback
         */
        if(curInstantiateLevel != su->status.instantiateLevel)
            reset = CL_FALSE;
    }
    if(reset)
        su->status.instantiateLevel = 0;

    exitfn:
    reentrant = CL_FALSE;
    return rc;
}

/*
 * clAmsPeSUTerminate
 * ------------------
 * Terminate a SU and all components within it.
 */

ClRcT
clAmsPeSUTerminate(
        CL_IN       ClAmsSUT        *su)
{
    ClAmsNodeT *node;

    AMS_CHECK_SU ( su );
    AMS_CHECK_NODE ( node = (ClAmsNodeT *) su->config.parentNode.ptr );

    AMS_FUNC_ENTER ( ("SU [%s]\n",su->config.entity.name.value) );

    /*
     * Precheck: Is SU in valid state to be terminated?
     */

    if ( (su->status.presenceState != CL_AMS_PRESENCE_STATE_INSTANTIATED)  &&
         (su->status.presenceState != CL_AMS_PRESENCE_STATE_INSTANTIATING) &&
         (su->status.presenceState != CL_AMS_PRESENCE_STATE_RESTARTING) )
    {
        AMS_ENTITY_LOG(su, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
            ("SU [%s] is in Presence State [%s]. Ignoring terminate request..\n",
            su->config.entity.name.value,
            CL_AMS_STRING_P_STATE(su->status.presenceState)));

        return CL_OK;
    }

    AMS_ENTITY_LOG(su, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
            ("Terminating SU [%s]. Presence State [%s]\n",
             su->config.entity.name.value,
             CL_AMS_STRING_P_STATE(su->status.presenceState)) ); 

    CL_AMS_RESET_EPOCH(su);

    if ( su->status.presenceState == CL_AMS_PRESENCE_STATE_INSTANTIATING )
    {
        AMS_CALL ( clAmsPeSUMarkUninstantiated(su) );

        /*
         * We increment instantiated SU count as it will get decremented
         * in the SU terminate callback, but at that time we won't know
         * that the SU did a instantiating -> terminating transition and
         * wasn't really instantiated.
         */

        node->status.numInstantiatedSUs++;
    }

    if ( su->status.presenceState == CL_AMS_PRESENCE_STATE_RESTARTING )
    {
        AMS_CALL ( clAmsPeSUMarkUnassigned(su) );
        AMS_CALL ( clAmsPeSUMarkTerminated(su) );
    }

    if ( su->status.presenceState == CL_AMS_PRESENCE_STATE_INSTANTIATED )
    {
        AMS_CALL ( clAmsPeSUMarkUnassigned(su) );
        AMS_CALL ( clAmsPeSUMarkTerminated(su) );
    }

    if ( clAmsPeSUIsInstantiable(su) != CL_OK )
    {
        AMS_CALL ( clAmsPeSUMarkUninstantiable(su) );
    }

    if ( su->status.presenceState != CL_AMS_PRESENCE_STATE_RESTARTING )
    {
        CL_AMS_SET_P_STATE(su, CL_AMS_PRESENCE_STATE_TERMINATING);
    }

    /*
     * Terminate all components in reverse order of instantiation.
     */
    clAmsPeSUTerminate2(su);

    return CL_OK;
}

/*
 * clAmsPeSUTerminateCallback
 * --------------------------
 * This fn is called whenever a constituent component receives a terminate
 * callback indicating the termination has succeeded or failed. 
 */

ClRcT
clAmsPeSUTerminateCallback(
        CL_IN       ClAmsSUT        *su,
        CL_IN       ClRcT           error)
{
    ClAmsNodeT  *node;
    ClAmsSGT *sg;

    AMS_CHECK_SU ( su );
    AMS_CHECK_NODE ( (node = (ClAmsNodeT *) su->config.parentNode.ptr) );
    AMS_CHECK_SG ( (sg = (ClAmsSGT *) su->config.parentSG.ptr) );

    AMS_FUNC_ENTER ( ("SU [%s]\n",su->config.entity.name.value) );

    /*
     * Precheck: Is terminate callback valid?
     */

    if ( (su->status.presenceState != CL_AMS_PRESENCE_STATE_TERMINATING) &&
         (su->status.presenceState != CL_AMS_PRESENCE_STATE_RESTARTING) )
    {
        AMS_ENTITY_LOG(su, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("SU [%s] is in Presence State [%s]. Ignoring terminate callback..\n",
             su->config.entity.name.value,
             CL_AMS_STRING_P_STATE(su->status.presenceState)) ); 

        return CL_OK;
    }

    /*
     * It is unlikely that a comp terminate will return an error. If there is
     * an error, it will result in a cleanup. If cleanup fails, then there
     * will be a fault and flow wont come here. If cleanup succeeds, its the
     * same as terminate succeeding, hence no error.
     *
     * However, we will still keep this clause here to catch future changes
     * that may not follow the above logic.
     */

    if ( error )
    {
        AMS_ENTITY_LOG(su, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_ERROR,
            ("Error: SU [%s] terminate callback indicates Error [0x%x]. Cleaning up..\n",
            su->config.entity.name.value,
            error));

        return clAmsPeSUCleanup(su);
    }

    if ( su->status.numInstantiatedComp > 0 )
    {
        if((su->status.numInstantiatedComp != su->status.numPIComp))
        {
            return CL_OK;
        }
        /*
         * Cleanup the components in higher levels.
         */
        AMS_CALL(clAmsPeSUTerminate2(su));

        if(su->status.numInstantiatedComp) return CL_OK;
    }

    /*
     * All components in SU have terminated successfully.
     */
    su->status.numInstantiatedComp = 0;

    if ( su->status.presenceState == CL_AMS_PRESENCE_STATE_RESTARTING )
    {
         if ( clAmsPeSUIsInstantiable(su) != CL_OK )
         {
            CL_AMS_SET_P_STATE(su, CL_AMS_PRESENCE_STATE_TERMINATING);
         }
    }

    if ( su->status.presenceState == CL_AMS_PRESENCE_STATE_TERMINATING )
    {
        if ( node->status.numInstantiatedSUs )
        {
            node->status.numInstantiatedSUs --;
        }
    }

    AMS_ENTITY_LOG(su, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
        ("SU [%s] is now terminated\n",
         su->config.entity.name.value));

    AMS_ENTITY_LOG(node, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
        ("Node [%s] now has [%d] instantiated + restarting SUs\n",
         node->config.entity.name.value,
         node->status.numInstantiatedSUs));

    // WORKQUEUE

    if ( su->status.presenceState == CL_AMS_PRESENCE_STATE_RESTARTING )
    {
        AMS_CALL ( clAmsPeSURestartCallback_Step1(su, CL_OK) );
    }
    else
    {
        CL_AMS_SET_P_STATE(su, CL_AMS_PRESENCE_STATE_UNINSTANTIATED);

        /*
         * If the component was terminated without removing SIs assigned to it,
         * and this happens when there are overlapping operations such as node
         * terminate happening before node switchover is complete, then remove
         * the SIs assigned to this SU.
         */

        if ( su->status.siList.numEntities )
        {
            AMS_CALL ( clAmsPeSURemoveWork(su, CL_AMS_ENTITY_SWITCHOVER_FAST) );
        }

        if ( su->config.adminState == CL_AMS_ADMIN_STATE_LOCKED_I )
        {
            AMS_CALL ( clAmsPeSULockInstantiationCallback(su, CL_OK) );
        }

        if ( sg->status.isStarted == CL_FALSE )
        {
            AMS_CALL ( clAmsPeSGTerminateCallback(sg, CL_OK) );
        }

        if ( node->status.presenceState == CL_AMS_PRESENCE_STATE_TERMINATING )
        {
            AMS_CALL ( clAmsPeNodeTerminateCallback(node, CL_OK) );
        }
    }

    /*
     * Make a gratuitous call to SGEvaluateWork to ensure that the SG rules
     * are satisfied.
     */
 
    AMS_CALL ( clAmsPeSGEvaluateWork((ClAmsSGT *)su->config.parentSG.ptr) );

    return CL_OK;
}

/*
 * clAmsPeSUCleanup
 * ----------------
 * This fn is called to cleanup the components associated with a SU. The logic
 * for the fn is the same as terminate followed by terminate callback.
 *
 * Note: We may get multiple calls to SU cleanup if a SU restart fails. The
 * code below should not have any side effects if this happens.
 */

ClRcT clAmsPeSUCleanup(CL_IN   ClAmsSUT    *su) 
{
    ClAmsEntityRefT *entityRef;
    ClAmsNodeT *node;
    ClRcT rc;

    AMS_CHECK_SU ( su );
    AMS_CHECK_NODE ( node = (ClAmsNodeT *) su->config.parentNode.ptr );

    AMS_FUNC_ENTER ( ("SU [%s]\n",su->config.entity.name.value) );

    if ( su->status.presenceState == CL_AMS_PRESENCE_STATE_UNINSTANTIATED )
    {
        return clAmsPeSUCleanupCallback(su, CL_OK);
    }

    AMS_ENTITY_LOG(su, CL_AMS_MGMT_SUB_AREA_MSG, CL_LOG_SEV_DEBUG,
        ("Cleaning up SU [%s] in Presence State [%s]\n",
         su->config.entity.name.value,
         CL_AMS_STRING_P_STATE(su->status.presenceState)) ); 

    CL_AMS_RESET_EPOCH(su);

    /*
     * A SU cleanup may be called at any state of the SUs life cycle. The
     * SU must be marked appropriately in the SGs list. Note, we do not
     * handle the case when Presence State is terminating because this
     * logic has already been executed in SU terminate.
     */

    if ( su->status.presenceState == CL_AMS_PRESENCE_STATE_INSTANTIATING )
    {
        AMS_CALL ( clAmsPeSUMarkUninstantiated(su) );

        /*
         * We increment instantiated SU count as it will get decremented
         * in the SU terminate callback, but at that time we won't know
         * that the SU did a instantiating -> terminating transition and
         * wasn't really instantiated.
         */

        node->status.numInstantiatedSUs++;
    }

    if ( su->status.presenceState == CL_AMS_PRESENCE_STATE_RESTARTING )
    {
        AMS_CALL ( clAmsPeSUMarkUninstantiated(su) );
        AMS_CALL ( clAmsPeSUMarkUnassigned(su) );
        AMS_CALL ( clAmsPeSUMarkTerminated(su) );
    }

    if ( su->status.presenceState == CL_AMS_PRESENCE_STATE_INSTANTIATED )
    {
        // XXX: Need check to ensure this is not called multiple times,
        // when restart operation fails and su cleanup is repeated.
        // Currently both functions below are changed to not complain if 
        // delete su from sg list fails (which may be a valid soln too, 
        // but it may hide real errors so should be avoided).

        AMS_CALL ( clAmsPeSUMarkUnassigned(su) );
        AMS_CALL ( clAmsPeSUMarkTerminated(su) );
    }

    if ( clAmsPeSUIsInstantiable(su) != CL_OK )
    {
        AMS_CALL ( clAmsPeSUMarkUninstantiable(su) );
    }

    if ( (su->status.presenceState != CL_AMS_PRESENCE_STATE_RESTARTING) &&
         (su->status.presenceState != CL_AMS_PRESENCE_STATE_TERMINATION_FAILED) &&
         (su->status.presenceState != CL_AMS_PRESENCE_STATE_INSTANTIATION_FAILED) )
    {
        CL_AMS_SET_P_STATE(su, CL_AMS_PRESENCE_STATE_TERMINATING);
    }

    /*
     * Cleanup all components that have not already failed instantiation or
     * termination.
     */

    ClUint32T count = 0;

    for ( entityRef = clAmsEntityListGetLast(&su->config.compList);
          entityRef != (ClAmsEntityRefT *) NULL;
          entityRef = clAmsEntityListGetPrevious(&su->config.compList, entityRef) )
    {
        ClAmsCompT *sucomp = (ClAmsCompT *) entityRef->ptr;

        AMS_CHECK_COMP ( sucomp );

        if ( sucomp->status.presenceState == CL_AMS_PRESENCE_STATE_TERMINATION_FAILED )
            continue;

        if ( sucomp->status.presenceState == CL_AMS_PRESENCE_STATE_INSTANTIATION_FAILED )
            continue;

        rc = clAmsPeCompCleanup(sucomp);
        if (rc != CL_OK)
        {
          AMS_ENTITY_LOG(sucomp, CL_AMS_MGMT_SUB_AREA_MSG,CL_LOG_SEV_WARNING, ("Component [%s] cleanup failed with error code [%d]", sucomp->config.entity.name.value));
        }
        
        count++;

        if ( sucomp->status.presenceState == CL_AMS_PRESENCE_STATE_TERMINATION_FAILED )
        {
            /*
             * If a component cleanup fails, it will result in a fault
             * report for the component and that will result in further
             * cleanup and recovery actions. So, this cleanup is aborted.
             */

            AMS_ENTITY_LOG(sucomp, CL_AMS_MGMT_SUB_AREA_MSG,CL_LOG_SEV_WARNING,("Component [%s] cleanup failed. Marking SU termination-failed and exiting SU cleanup..", sucomp->config.entity.name.value));

            CL_AMS_SET_P_STATE(su, CL_AMS_PRESENCE_STATE_TERMINATION_FAILED);

            return CL_OK;
        }
    }

    /*
     * Be careful of adding code here, it may get executed after su cleanup
     * callback as comp cleanup calls comp cleanup callback, which calls
     * su cleanup callback for saf aware components.
     */

    if ( count == 0 )
    {
        return clAmsPeSUCleanupCallback(su, CL_OK);
    }

    return CL_OK;
}

static ClUint32T clAmsSURestartingCompCount(CL_IN       ClAmsSUT        *su)
{
    ClAmsEntityRefT *entityRef;
    ClUint32T restartCompCount = 0;

    if (!su)
        return 0;

    for ( entityRef = clAmsEntityListGetLast(&su->config.compList);
          entityRef != (ClAmsEntityRefT *) NULL;
          entityRef = clAmsEntityListGetPrevious(&su->config.compList, entityRef) )
    {
        ClAmsCompT *sucomp = (ClAmsCompT *) entityRef->ptr;

        if (sucomp)
        {
            if ( sucomp->status.presenceState == CL_AMS_PRESENCE_STATE_RESTARTING )
                restartCompCount++;
        }
    }

    return restartCompCount;
}

/*
 * clAmsPeSUCleanupCallback
 * ------------------------
 * This fn is called whenever a constituent component receives a cleanup
 * callback and at the end of SU cleanup. So it can be called synchronously
 * (for saf aware and npnp components)and asynchronously (for proxied 
 * components).
 */

ClRcT
clAmsPeSUCleanupCallback(
        CL_IN       ClAmsSUT        *su,
        CL_IN       ClRcT           error)
{
    ClAmsNodeT *node;
    ClAmsSGT *sg;

    AMS_CHECK_SU ( su );
    AMS_CHECK_NODE ( node = (ClAmsNodeT *) su->config.parentNode.ptr );
    AMS_CHECK_SG ( (sg = (ClAmsSGT *) su->config.parentSG.ptr) );

    AMS_FUNC_ENTER ( ("SU [%s]\n",su->config.entity.name.value) );

    if ( error )
    {
        return clAmsPeSUCleanupError(su, error);
    }

    if ( su->status.numInstantiatedComp > clAmsSURestartingCompCount(su) )
    {
        return CL_OK;
    }

    AMS_ENTITY_LOG(su, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
        ("SU [%s] is now terminated by cleanup action\n",
         su->config.entity.name.value));

    if ( su->status.presenceState == CL_AMS_PRESENCE_STATE_RESTARTING )
    {
         if ( clAmsPeSUIsInstantiable(su) != CL_OK )
         {
            CL_AMS_SET_P_STATE(su, CL_AMS_PRESENCE_STATE_TERMINATING);
         }
    }

    /*
     * The count is incremented when the su is first instantiated, left
     * unchanged through restart cycles and then finally decremented when we
     * know for sure that the su is being terminated. The instantiated count
     * is already decremented for TERMINATION_FAILED.
     */

    if ( su->status.presenceState == CL_AMS_PRESENCE_STATE_TERMINATING )
    {
        if ( node->status.numInstantiatedSUs )
        {
            node->status.numInstantiatedSUs-- ;
        }
    }

    AMS_ENTITY_LOG(node, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
        ("Node [%s] now has [%d] instantiated + restarting SUs\n",
         node->config.entity.name.value,
         node->status.numInstantiatedSUs));

    // WORKQUEUE

    if ( su->status.presenceState == CL_AMS_PRESENCE_STATE_RESTARTING )
    {
        AMS_CALL ( clAmsPeSURestartCallback_Step1(su, CL_OK) );
    }
    else
    {
        /*
         * If the component was terminated without removing SIs assigned to it,
         * and this happens when there are overlapping operations such as node
         * terminate happening before node switchover is complete, then remove
         * the SIs assigned to this SU with mode fast.
         *
         * This should be an sync action due to mode fast.
         */

        if ( su->status.siList.numEntities )
        {
            AMS_CALL ( clAmsPeSURemoveWork(su, CL_AMS_ENTITY_SWITCHOVER_FAST) );
        }

        if ( (su->status.presenceState != CL_AMS_PRESENCE_STATE_TERMINATION_FAILED) &&
             (su->status.presenceState != CL_AMS_PRESENCE_STATE_INSTANTIATION_FAILED) )
        {
            CL_AMS_SET_P_STATE(su, CL_AMS_PRESENCE_STATE_UNINSTANTIATED);
        }

        if ( su->status.recovery == CL_AMS_RECOVERY_COMP_FAILOVER 
             ||
             su->status.recovery == CL_AMS_RECOVERY_INTERNALLY_RECOVERED)
        {
            AMS_CALL ( clAmsPeSUFaultCallback_Step2(su, CL_OK) );
        }

        if ( su->config.adminState == CL_AMS_ADMIN_STATE_LOCKED_I )
        {
            AMS_CALL ( clAmsPeSULockInstantiationCallback(su, CL_OK) );
        }

        if ( sg->status.isStarted == CL_FALSE )
        {
            AMS_CALL ( clAmsPeSGTerminateCallback(sg, CL_OK) );
        }

        if ( node->status.presenceState == CL_AMS_PRESENCE_STATE_TERMINATING )
        {
            AMS_CALL ( clAmsPeNodeTerminateCallback(node, CL_OK) );
        }
    }

    /*
     * Make a gratuitous call to SGEvaluateWork to ensure that the SG rules
     * are satisfied.
     */
 
    AMS_CALL ( clAmsPeSGEvaluateWork((ClAmsSGT *)su->config.parentSG.ptr) );

    return CL_OK;
}

/*
 * clAmsPeSUCleanupError
 * ---------------------
 * This fn is called whenever a constituent component receives a cleanup
 * error.
 */

ClRcT
clAmsPeSUCleanupError(
        CL_IN       ClAmsSUT        *su,
        CL_IN       ClRcT           error)
{
    ClAmsNodeT *node;
    ClAmsSGT *sg;
    ClAmsPresenceStateT pstate;

    AMS_CHECK_SU ( su );
    AMS_CHECK_NODE ( node = (ClAmsNodeT *) su->config.parentNode.ptr );
    AMS_CHECK_SG ( (sg = (ClAmsSGT *) su->config.parentSG.ptr) );

    AMS_FUNC_ENTER ( ("SU [%s]\n",su->config.entity.name.value) );

    AMS_ENTITY_LOG(su, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_ERROR,
        ("Error [0x%x] in cleaning up SU [%s]. Marking [termination-failed]\n",
         error,
         su->config.entity.name.value));

    pstate = su->status.presenceState;

    if ( (su->status.presenceState == CL_AMS_PRESENCE_STATE_TERMINATING) ||
         (su->status.presenceState == CL_AMS_PRESENCE_STATE_RESTARTING)  ||
         (su->status.presenceState == CL_AMS_PRESENCE_STATE_INSTANTIATED) )
    {
        if ( node->status.numInstantiatedSUs )
        {
            node->status.numInstantiatedSUs-- ;
        }
        if( su->status.presenceState == CL_AMS_PRESENCE_STATE_INSTANTIATED)
        {
            AMS_CALL ( clAmsPeSUMarkUnassigned(su) );
            AMS_CALL ( clAmsPeSUMarkNotReady(su) );
            AMS_CALL ( clAmsPeSUMarkUninstantiated(su) );
        }
        AMS_CALL ( clAmsPeSUMarkUninstantiable(su) );
    }

    /*
     * If SU is in presence state instantiating, then mark it uninstantiable
     */
    if(su->status.presenceState == CL_AMS_PRESENCE_STATE_INSTANTIATING)
    {
        AMS_CALL ( clAmsPeSUMarkUninstantiated(su) );
        AMS_CALL ( clAmsPeSUMarkUninstantiable(su) );
    }

    AMS_ENTITY_LOG(node, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
        ("Node [%s] now has [%d] instantiated + restarting SUs\n",
         node->config.entity.name.value,
         node->status.numInstantiatedSUs));

    CL_AMS_SET_P_STATE(su, CL_AMS_PRESENCE_STATE_TERMINATION_FAILED);
    CL_AMS_SET_O_STATE(su, CL_AMS_OPER_STATE_DISABLED);

    if ( pstate == CL_AMS_PRESENCE_STATE_RESTARTING )
    {
        AMS_CALL ( clAmsPeSURestartCallback_Step2(su, error) );
    }
    else
    {
        if ( su->status.recovery == CL_AMS_RECOVERY_COMP_FAILOVER 
             ||
             su->status.recovery == CL_AMS_RECOVERY_INTERNALLY_RECOVERED)
        {
            AMS_CALL ( clAmsPeSUFaultCallback_Step2(su, error) );
        }

        if ( su->config.adminState == CL_AMS_ADMIN_STATE_LOCKED_I )
        {
            AMS_CALL ( clAmsPeSULockInstantiationCallback(su, error) );
        }

        if ( sg->status.isStarted == CL_FALSE )
        {
            AMS_CALL ( clAmsPeSGTerminateCallback(sg, error) );
        }

        if ( node->status.presenceState == CL_AMS_PRESENCE_STATE_TERMINATING )
        {
            AMS_CALL ( clAmsPeNodeTerminateCallback(node, error) );
        }
    }

    return CL_OK;
}

/*-----------------------------------------------------------------------------
 * SU work assignment functions
 *---------------------------------------------------------------------------*/

/*
 * clAmsPeSUEvaluateWork
 * ---------------------
 * This fn is called to evaluate the work needs of an SU which can be either
 * or both of instantiation followed by assignment. The decision on whether 
 * to instantiate or assign work to a SU is dependent on the needs of the 
 * SG, so this request is redirected to the SU's SG.
 */

ClRcT
clAmsPeSUEvaluateWork(
                      CL_IN   ClAmsSUT    *su
                      )
{
    ClAmsSGT *sg;
    ClAmsNodeT *node;

    AMS_CHECK_SU ( su );

    AMS_FUNC_ENTER ( ("SU [%s]\n",su->config.entity.name.value) );

    sg = (ClAmsSGT *) su->config.parentSG.ptr ;
    if(!sg) return CL_OK;

    AMS_CHECK_SG( sg );
    AMS_CHECK_NODE (node = (ClAmsNodeT*)su->config.parentNode.ptr);

    if( clAmsPeSUIsInstantiable(su) != CL_OK )
    {
        AMS_ENTITY_LOG(su, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
            ("SU [%s] is not instantiable, ignoring evaluate work request..\n",
             su->config.entity.name.value));

        return CL_OK;
    }

    AMS_ENTITY_LOG (su, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE, 
            ("Evaluating work for SU [%s]\n",
             su->config.entity.name.value));

    AMS_CALL ( clAmsPeSGEvaluateWork(sg) );

    return CL_OK;
}


/*
 * clAmsPeSUAssignWorkAgain
 * ------------------------
 * Assigns the currently assigned work to a SU back again to the SU. This is 
 * used during a SU restart.
 */

ClRcT
clAmsPeSUAssignWorkAgain(
        CL_IN   ClAmsSUT *su)
{
    ClAmsEntityRefT *entityRef;

    AMS_CHECK_SU ( su );

    AMS_FUNC_ENTER ( ("SU [%s]\n",su->config.entity.name.value) );

    AMS_ENTITY_LOG (su, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
             ("Replaying [%d] SI assignments for SU [%s]\n",
              su->status.siList.numEntities,
              su->config.entity.name.value) );

    if ( su->status.numActiveSIs || su->status.numStandbySIs )
    {
        AMS_CALL ( clAmsPeSUMarkAssigned(su) );
    }

    for ( entityRef = clAmsEntityListGetFirst(&su->config.compList);
          entityRef != (ClAmsEntityRefT *) NULL;
          entityRef = clAmsEntityListGetNext(&su->config.compList, entityRef) )
    {
        ClAmsCompT *comp = (ClAmsCompT *) entityRef->ptr;

        AMS_CALL ( clAmsPeCompAssignCSIAgain(comp) );
    }

    return CL_OK;
}

/*
 * clAmsPeSUSwitchoverWorkByComponent
 * ----------------------------------
 * Switchover the work assigned to a SU component by component.
 *
 * Note, SAF uses the term switchover to only mean switchover of active
 * SIs. In this code, switchover is used more generically for both active
 * and standby SIs. The priority is obviously to switch active SIs first
 * but then also reassign standby SIs if there is sufficient spare capacity
 * in the system.
 *
 * This fn can be called because of multiple reasons (locked, shutdown,
 * fault, adjust) at the su, node or cluster level. It thus does not make
 * any assumptions about the cause, but lets it's callback figure out what
 * actions to take once the switchover is complete.
 */

ClRcT
clAmsPeSUSwitchoverWorkByComponent(
        CL_IN ClAmsSUT *su,
        CL_IN ClUint32T switchoverMode)
{
    ClAmsEntityRefT *entityRef;

    AMS_CHECK_SU ( su );

    AMS_FUNC_ENTER ( ("SU [%s]\n",su->config.entity.name.value) );

    /*
     * Switchover/remove pending removed SIs/CSIs.
     */

    for ( entityRef = clAmsEntityListGetFirst(&su->config.compList);
          entityRef != (ClAmsEntityRefT *) NULL;
          entityRef = clAmsEntityListGetNext(&su->config.compList,entityRef) )
    {
        AMS_CALL ( clAmsPeCompSwitchoverWork(
                        (ClAmsCompT *) entityRef->ptr,
                        switchoverMode,
                        CL_AMS_HA_STATE_NONE) );
    }

    /*
     * Switchover/remove quiesced SIs/CSIs.
     */

    for ( entityRef = clAmsEntityListGetFirst(&su->config.compList);
          entityRef != (ClAmsEntityRefT *) NULL;
          entityRef = clAmsEntityListGetNext(&su->config.compList,entityRef) )
    {
        AMS_CALL ( clAmsPeCompSwitchoverWork(
                                             (ClAmsCompT *) entityRef->ptr,
                                             switchoverMode,
                                             CL_AMS_HA_STATE_QUIESCED) );
    }
        
    /*
     * Switchover/remove Quiescing SIs/CSIs
     */
    for ( entityRef = clAmsEntityListGetFirst(&su->config.compList);
          entityRef != (ClAmsEntityRefT *) NULL;
          entityRef = clAmsEntityListGetNext(&su->config.compList,entityRef) )
    {
        AMS_CALL ( clAmsPeCompSwitchoverWork(
                        (ClAmsCompT *) entityRef->ptr,
                        switchoverMode,
                        CL_AMS_HA_STATE_QUIESCING) );
    }

    /*
     * For 2N and M+N models, only one of the two if statements below will
     * be executed. For N-Way, both may be executed and for N-Way-Active
     * only the first will be executed.
     */

    /*
     * Switchover active SIs/CSIs first.
     */

    if ( su->status.numActiveSIs )
    {
        for ( entityRef = clAmsEntityListGetFirst(&su->config.compList);
              entityRef != (ClAmsEntityRefT *) NULL;
              entityRef = clAmsEntityListGetNext(&su->config.compList, entityRef) )
        {
            AMS_CALL ( clAmsPeCompSwitchoverWork(
                            (ClAmsCompT *) entityRef->ptr,
                            switchoverMode,
                            CL_AMS_HA_STATE_ACTIVE) );
        }
    }

    /*
     * Switchover standby SIs/CSIs.
     */

    if ( su->status.numStandbySIs )
    {
        for ( entityRef = clAmsEntityListGetFirst(&su->config.compList);
              entityRef != (ClAmsEntityRefT *) NULL;
              entityRef = clAmsEntityListGetNext(&su->config.compList,entityRef) )
        {
            AMS_CALL ( clAmsPeCompSwitchoverWork(
                            (ClAmsCompT *) entityRef->ptr,
                            switchoverMode,
                            CL_AMS_HA_STATE_STANDBY) );
        }
    }

    return CL_OK;
}

/*
 * clAmsPeSUSwitchoverWorkBySI
 * ---------------------------
 * This fn switches over the work assigned to a SU by following the SI list
 * instead of the component list.
 *
 */

ClRcT
clAmsPeSUSwitchoverWorkBySI(
        CL_IN ClAmsSUT *su,
        CL_IN ClUint32T switchoverMode)
{
    ClAmsEntityRefT *entityRef;

    AMS_CHECK_SU ( su );

    AMS_FUNC_ENTER ( ("SU [%s]\n",su->config.entity.name.value) );

    /*
     * Switchover/remove quiesced/pending remove SIs/CSIs.
     */

    {
        entityRef = clAmsEntityListGetFirst(&su->status.siList);

        while ( entityRef != (ClAmsEntityRefT *) NULL )
        {
            ClAmsSUSIRefT *siRef = (ClAmsSUSIRefT *) entityRef;
            ClAmsSIT *si = (ClAmsSIT *) entityRef->ptr;

            /*
             * Get the next SI upfront as RemoveSI will destroy the list
             * of SIs.
             */

            entityRef = clAmsEntityListGetNext(&su->status.siList, entityRef);
            if ( (siRef->haState == CL_AMS_HA_STATE_QUIESCED)|| 
                 (siRef->haState == CL_AMS_HA_STATE_NONE) )
            {
                AMS_CALL ( clAmsPeSURemoveSI(su, si, switchoverMode) );
            }
        }
    }

    /*
     * For 2N and M+N models, only one of the two if statements below will
     * be executed. For N-Way, both may be executed and for N-Way-Active
     * only the first will be executed.
     */

    /*
     * Switchover active SIs/CSIs first.
     */

    if ( su->status.numActiveSIs )
    {
        entityRef = clAmsEntityListGetFirst(&su->status.siList);

        while ( entityRef != (ClAmsEntityRefT *) NULL )
        {
            ClAmsSUSIRefT *siRef = (ClAmsSUSIRefT *) entityRef;
            ClAmsSIT *si = (ClAmsSIT *) entityRef->ptr;

            /*
             * Get the next SI upfront as QuiesceSI may end up removing the
             * current SI from the list, if switchoverMode is FAST and the
             * call stack goes to removeSI.
             */

            entityRef = clAmsEntityListGetNext(&su->status.siList, entityRef);

            if ( siRef->haState == CL_AMS_HA_STATE_ACTIVE )
            {
                AMS_CALL ( clAmsPeSUQuiesceSI(su, si, switchoverMode) );
            }
        }
    }

    /*
     * Switchover standby SIs/CSIs.
     */

    if ( su->status.numStandbySIs )
    {
        entityRef = clAmsEntityListGetFirst(&su->status.siList);

        while ( entityRef != (ClAmsEntityRefT *) NULL )
        {
            ClAmsSUSIRefT *siRef = (ClAmsSUSIRefT *) entityRef;
            ClAmsSIT *si = (ClAmsSIT *) entityRef->ptr;

            /*
             * Get the next SI upfront as RemoveSI will destroy the list
             * of SIs.
             */

            entityRef = clAmsEntityListGetNext(&su->status.siList, entityRef);

            if ( siRef->haState == CL_AMS_HA_STATE_STANDBY )
            {
                AMS_CALL ( clAmsPeSURemoveSI(su, si, switchoverMode) );
            }
        }
    }

    return CL_OK;
}

/*
 * Removes the work assignments for both active and its standbys
 */
ClRcT
clAmsPeSUSwitchoverWorkActiveAndStandby(ClAmsSUT *su, ClListHeadT *standbyList, ClUint32T mode)
{
    ClAmsEntityRefT *eRef = NULL;
    ClAmsEntityRefT *nextRef = NULL;
    for(eRef = clAmsEntityListGetFirst(&su->status.siList);
        eRef != NULL;
        eRef = nextRef)
    {
        ClAmsSIT *si = (ClAmsSIT*)eRef->ptr;
        ClAmsEntityRefT *eRef2 = NULL;
        ClAmsEntityRefT *nextRef2 = NULL;
        nextRef = clAmsEntityListGetNext(&su->status.siList, eRef);
        /*
         * Check if the standby SU needs to be switched over
         or is already switched over.
        */
        for(eRef2 = clAmsEntityListGetFirst(&si->status.suList);
            eRef2 != NULL;
            eRef2 = nextRef2)
        {
            ClAmsSUT *tmpSU = (ClAmsSUT*)eRef2->ptr;
            nextRef2 = clAmsEntityListGetNext(&si->status.suList, eRef2);
            if(tmpSU == su) continue;
            if(standbyList)
            {
                ClListHeadT *iter;
                CL_LIST_FOR_EACH(iter, standbyList)
                {
                    ClAmsSUAdjustListT *adjustEntry = CL_LIST_ENTRY(iter, ClAmsSUAdjustListT, list);
                    if(adjustEntry->su == tmpSU) goto skip_standby;
                }
            }
            
            /*
             * Remove the standby assignments. from this SU
             */
            if(tmpSU->status.numStandbySIs || tmpSU->status.numQuiescedSIs)
            {
                clLogInfo("AMS", "ADJUST", "Removing standby assignment of SI [%.*s] from SU [%.*s]",
                          si->config.entity.name.length-1, si->config.entity.name.value,
                          tmpSU->config.entity.name.length-1, tmpSU->config.entity.name.value);
                clAmsPeSURemoveSI(tmpSU, si, mode);
            }
            skip_standby: ;
        }
        /*
         * Now quiesce the active SIs for this SU.
         */
        clLogInfo("AMS", "ADJUST", "Quiescing active SI [%.*s] for SU [%.*s]",
                  si->config.entity.name.length-1, si->config.entity.name.value,
                  su->config.entity.name.length-1, su->config.entity.name.value);
        clAmsPeSUQuiesceSI(su, si, mode);
    }
    return CL_OK;
}


/*
 * clAmsPeSUSwitchoverWork
 * -----------------------
 * Switch over the work assigned to a SU, ie, switchover the work assigned to
 * its components.
 */

ClRcT
clAmsPeSUSwitchoverWork(
        CL_IN ClAmsSUT *su,
        CL_IN ClUint32T switchoverMode)
{
    AMS_CHECK_SU ( su );

    su->status.compRestartCount = 0;
    AMS_CALL ( clAmsEntityTimerStop((ClAmsEntityT *) su, CL_AMS_SU_TIMER_COMPRESTART) );
    su->status.suRestartCount = 0;
    AMS_CALL ( clAmsEntityTimerStop((ClAmsEntityT *) su, CL_AMS_SU_TIMER_SURESTART) );

    clAmsEntityTimerStop((ClAmsEntityT *)su, CL_AMS_SU_TIMER_PROBATION);

    clAmsEntityTimerStop ((ClAmsEntityT*)su, CL_AMS_SU_TIMER_ASSIGNMENT);

    AMS_ENTITY_LOG (su, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
             ("SU [%s] Switchover Request...\n",
              su->config.entity.name.value) );

    if ( su->status.siList.numEntities )
    {
        AMS_CALL ( clAmsPeSUSwitchoverWorkByComponent(su, switchoverMode) );
    }
    else
    {
        AMS_CALL ( clAmsPeSUSwitchoverCallback(su, CL_OK, switchoverMode) );
    }

    return CL_OK;
}

static ClRcT clAmsPeSUSwitchoverPrologue(ClAmsSUT *su, ClUint32T error, ClUint32T switchoverMode)
{
    ClAmsSGT *sg;
    ClAmsNodeT *node;

    AMS_CHECK_SU ( su );
    AMS_CHECK_SG ( sg = (ClAmsSGT *) su->config.parentSG.ptr );
    AMS_CHECK_NODE ( node = (ClAmsNodeT *) su->config.parentNode.ptr );

    /*
     * Remove the assignments to the constituent components of this SU.
     */
    if (! su->status.numQuiescedSIs )
    {
        /*
         * Now figure out who needs to get a callback that the SU has switched over.
         */

        // SU callbacks

        if ( su->config.adminState == CL_AMS_ADMIN_STATE_LOCKED_A )
        {
            AMS_CALL ( clAmsPeSULockAssignmentCallback(su, error) );
        }

        if ( su->config.adminState == CL_AMS_ADMIN_STATE_SHUTTINGDOWN )
        {
            AMS_CALL ( clAmsPeSUShutdownCallback(su, error) );
        }

        if ( su->status.recovery == CL_AMS_RECOVERY_COMP_FAILOVER 
             ||
             su->status.recovery == CL_AMS_RECOVERY_INTERNALLY_RECOVERED)
        {
            AMS_CALL ( clAmsPeSUFaultCallback_Step1(su, error) );
        }

        // SG callbacks

        if ( sg->config.adminState == CL_AMS_ADMIN_STATE_LOCKED_A )
        {
            AMS_CALL ( clAmsPeSGLockAssignmentCallback(sg, error) );
        }

        if ( sg->config.adminState == CL_AMS_ADMIN_STATE_SHUTTINGDOWN )
        {
            AMS_CALL ( clAmsPeSGShutdownCallback(sg, error) );
        }

        // Node callbacks

        if ( (node->status.isClusterMember == CL_AMS_NODE_IS_LEAVING_CLUSTER)    ||
             (node->status.isClusterMember == CL_AMS_NODE_IS_NOT_CLUSTER_MEMBER) ||
             (node->config.adminState      == CL_AMS_ADMIN_STATE_LOCKED_A)       ||
             (node->config.adminState      == CL_AMS_ADMIN_STATE_SHUTTINGDOWN) )
        {
            AMS_CALL ( clAmsPeNodeSwitchoverCallback(node, error, switchoverMode | CL_AMS_ENTITY_SWITCHOVER_SU) );
        }
    }

    /*
     * Make a gratuitous call to SGEvaluateWork to ensure that the SG rules
     * are satisfied.
     */
 
    AMS_CALL ( clAmsPeSGEvaluateWork((ClAmsSGT *)su->config.parentSG.ptr) );
    return CL_OK;
}

ClRcT clAmsPeSUSwitchoverRemoveReplay(ClAmsSUT *su, ClUint32T error, ClUint32T switchoverMode)
{
    AMS_CHECKPTR(!su);
    if(su->status.numQuiescedSIs)
    {
        ClAmsEntityRefT *entityRef = NULL;
        for ( entityRef = clAmsEntityListGetFirst(&su->config.compList);
              entityRef != (ClAmsEntityRefT *) NULL;
              entityRef = clAmsEntityListGetNext(&su->config.compList,entityRef) )
        {
            AMS_CALL ( clAmsPeCompRemoveWork( (ClAmsCompT*)entityRef->ptr,
                                              switchoverMode | CL_AMS_ENTITY_SWITCHOVER_SU));
        }
    }
    else
    {
        return clAmsPeSUSwitchoverPrologue(su, error, switchoverMode);
    }
    return CL_OK;
}

/*
 * If we are in node switchover phase and are in a group SU failover,
 * then check if the dependencies pending reassignments have SUs mapped
 * to the same node.
 */
ClBoolT clAmsPeCheckDependencySIInNode(ClAmsSIT *si, ClAmsSUT *su, ClUint32T level)
{
    ClAmsEntityRefT *ref = NULL;
    ClAmsNodeT *node = NULL;

    if(!si || !su) return CL_FALSE;

    node = (ClAmsNodeT*)su->config.parentNode.ptr;
    if(!node) return CL_FALSE;

    for(ref = clAmsEntityListGetFirst(&si->config.siDependenciesList);
        ref != NULL;
        ref = clAmsEntityListGetNext(&si->config.siDependenciesList, ref))
    {
        ClAmsSIT *targetSI = (ClAmsSIT*)ref->ptr;
        ClAmsEntityRefT *ref2 = NULL;
        ClBoolT reassignPending = clAmsEntityOpPending(&targetSI->config.entity,
                                                       &targetSI->status.entity,
                                                       CL_AMS_ENTITY_OP_SI_REASSIGN_MPLUSN);
        for(ref2 = clAmsEntityListGetFirst(&targetSI->status.suList);
            ref2 != NULL;
            ref2 = clAmsEntityListGetNext(&targetSI->status.suList, ref2))
        {
            ClAmsSUT *targetSU = (ClAmsSUT*)ref2->ptr;
            if(targetSU->config.adminState != CL_AMS_ADMIN_STATE_UNLOCKED)
                continue;
            if(reassignPending
               ||
               clAmsInvocationsSetPendingForSI(targetSI, targetSU))
            {
                return CL_TRUE;
            }
            /*
             * Check for multiple dependent SIs within the same SUs.
             */
            if(targetSU == su 
               && 
               targetSI->status.numStandbyAssignments
                && 
               node->status.operState == CL_AMS_OPER_STATE_DISABLED)
                return CL_TRUE;
        }
        /*
         * If the top level dependency SI has no standby, there is nothing to reassign
         */
        if(!level && !targetSI->status.numStandbyAssignments)
            continue;
        /*
         * Check if any of the dependencies of the target SI have pending invocations   
         */
        if(targetSI->config.siDependenciesList.numEntities > 0 
           &&
           clAmsPeCheckDependencySIInNode(targetSI, su, level+1))
            return CL_TRUE;
    }
    return CL_FALSE;
}

ClBoolT clAmsPeCheckDependencySIInSU(ClAmsSIT *si, ClAmsSUT *su)
{
    ClAmsEntityRefT *ref = NULL;
    for(ref = clAmsEntityListGetFirst(&si->config.siDependenciesList);
        ref != NULL;
        ref = clAmsEntityListGetNext(&si->config.siDependenciesList, ref))
    {
        ClAmsSIT *dependencySI = (ClAmsSIT*)ref->ptr;
        ClAmsEntityRefT *siRef = NULL;
        for(siRef = clAmsEntityListGetFirst(&su->status.siList);
            siRef != NULL;
            siRef = clAmsEntityListGetNext(&su->status.siList, siRef))
        {
            ClAmsSIT *targetSI = (ClAmsSIT*)siRef->ptr;
            if(targetSI == dependencySI)
            {
                if(clAmsInvocationsSetPendingForSI(targetSI, su)
                   ||
                   clAmsEntityOpPending(&targetSI->config.entity, 
                                        &targetSI->status.entity, 
                                        CL_AMS_ENTITY_OP_SI_REASSIGN_MPLUSN))
                    return CL_TRUE;
            }
        }
    }
    return CL_FALSE;
}

/*
 * Replay the SU switchover once the active SU has had its other standby
 * assignments removed.
 */

ClRcT
clAmsPeSUSwitchoverReplay(ClAmsSUT *su, ClAmsSUT *activeSU, ClUint32T error, ClUint32T switchoverMode)
{
    ClAmsSGT *sg = NULL;
    ClAmsNodeT *node = NULL;
    ClAmsEntityRefT *entityRef = NULL;
    ClBoolT allCSIsReassigned = CL_TRUE;
    ClBoolT *pAllCSIsReassigned = &allCSIsReassigned;
    CL_LIST_HEAD_DECLARE(dependentSIList);
    ClRcT rc = CL_OK;

    AMS_CHECKPTR(!activeSU);
    AMS_CHECK_SU ( su );
    AMS_CHECK_SU (activeSU);
    AMS_CHECK_SG ( sg = (ClAmsSGT *) su->config.parentSG.ptr );
    AMS_CHECK_NODE ( node = (ClAmsNodeT *) su->config.parentNode.ptr );

    clAmsPeSUSIDependentsListMPlusN(su, &activeSU, &dependentSIList);

    for ( entityRef = clAmsEntityListGetFirst(&su->config.compList);
          entityRef != (ClAmsEntityRefT *) NULL;
          entityRef = clAmsEntityListGetNext(&su->config.compList,entityRef) )
    {
        ClAmsCompT *comp = (ClAmsCompT *) entityRef->ptr;
        
        AMS_CHECK_COMP ( comp );

        rc = clAmsPeCompReassignWork(comp, &activeSU, &dependentSIList, pAllCSIsReassigned);
        
        if(rc != CL_OK)
        {
            clLogError("SU", "SWITCHOVER-REPLAY", "Comp [%s] reassign work returned [%#x]",
                       comp->config.entity.name.value, rc);
            clAmsPeSIReassignEntryDelete(&dependentSIList);
            return rc;
        }

        if(pAllCSIsReassigned && (!*pAllCSIsReassigned) )
            pAllCSIsReassigned = NULL;
    }

    clAmsPeSIReassignEntryListDelete(&dependentSIList);

    if(!allCSIsReassigned && su->status.siList.numEntities)
    {
        clLogDebug("AMS", "REPLAY", "Deferring SU [%s] remove after "
                   "reassignment of SU [%s] to active", su->config.entity.name.value,
                   activeSU->config.entity.name.value);
        ClAmsEntityRemoveOpT activeRemoveOp = {{0}};
        memcpy(&activeRemoveOp.entity, 
               &su->config.entity, 
               sizeof(activeRemoveOp.entity));
        activeRemoveOp.sisRemoved = su->status.siList.numEntities;
        activeRemoveOp.switchoverMode = switchoverMode;
        activeRemoveOp.error = error;
        clAmsEntityOpPush(&activeSU->config.entity, 
                          &activeSU->status.entity,
                          (void*)&activeRemoveOp,
                          (ClUint32T)sizeof(activeRemoveOp),
                          CL_AMS_ENTITY_OP_ACTIVE_REMOVE_MPLUSN);
        /*
         * Store a back ref.
         */
        memcpy(&activeRemoveOp.entity, &activeSU->config.entity, sizeof(activeRemoveOp.entity));
        clAmsEntityOpPush(&su->config.entity, &su->status.entity, 
                          (void*)&activeRemoveOp, sizeof(activeRemoveOp),
                          CL_AMS_ENTITY_OP_ACTIVE_REMOVE_REF_MPLUSN);
        return CL_OK;
    }

    clLogDebug("AMS", "REPLAY", "Replaying switchover remove for SU [%s] in mode [%d]",
               su->config.entity.name.value, switchoverMode);

    return clAmsPeSUSwitchoverRemoveReplay(su, error, switchoverMode);
}

ClRcT clAmsPeSISUReassignEntryDelete(ClAmsSIT *si)
{
    ClAmsEntityRefT *suRef = NULL;
    for(suRef = clAmsEntityListGetFirst(&si->status.suList);
        suRef != NULL;
        suRef = clAmsEntityListGetNext(&si->status.suList, suRef))
    {
        ClAmsSUT *targetSU = (ClAmsSUT*)suRef->ptr;
        clAmsPeDeleteSUReassignOp(targetSU);
    }
    return CL_OK;
}

ClRcT clAmsPeSIReassignEntryDelete(ClListHeadT *siList)
{
    ClAmsSIReassignEntryT *reassignEntry = NULL;
    if(!siList) return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    while(!CL_LIST_HEAD_EMPTY(siList))
    {
        ClAmsSIT *si = NULL;
        ClListHeadT *pNext = siList->pNext;
        void *reassignData = NULL;
        reassignEntry = CL_LIST_ENTRY(pNext, ClAmsSIReassignEntryT, list);
        clListDel(pNext);
        si = reassignEntry->si;
        if(si)
        {
            if(clAmsEntityOpClear(&si->config.entity,
                                  &si->status.entity,
                                  CL_AMS_ENTITY_OP_SI_REASSIGN_MPLUSN,
                                  (void**)&reassignData,
                                  NULL) == CL_OK)
            {
                if(reassignData) clHeapFree(reassignData);
            }
            clAmsPeSISUReassignEntryDelete(si);
        }
        clHeapFree(reassignEntry);
    }
    return CL_OK;
}

ClRcT clAmsPeSUSIReassignEntryDelete(ClAmsSUT *su)
{
    ClRcT rc = CL_OK;
    ClAmsSUReassignOpT *reassignEntry = NULL;
    if(!su) return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    rc = clAmsEntityOpClear(&su->config.entity, &su->status.entity,
                            CL_AMS_ENTITY_OP_SI_REASSIGN_MPLUSN,
                            (void**)&reassignEntry, NULL);
    if(rc == CL_OK && reassignEntry)
    {
        ClInt32T i;
        for(i  = 0; i < reassignEntry->numSIs; ++i)
        {
            ClAmsEntityT *siRef = reassignEntry->sis + i;
            ClAmsEntityRefT entityRef = {{0}};
            memcpy(&entityRef.entity, siRef, sizeof(entityRef.entity));
            rc = clAmsEntityDbFindEntity(&gAms.db.entityDb[CL_AMS_ENTITY_TYPE_SI],
                                         &entityRef);
            if(rc == CL_OK)
            {
                ClAmsSIT *si = (ClAmsSIT*)entityRef.ptr;
                if(si)
                {
                    clAmsEntityOpsClear(&si->config.entity, &si->status.entity);
                }
            }
        }
        if(reassignEntry->sis) clHeapFree(reassignEntry->sis);
        clHeapFree(reassignEntry);
    }
    return rc;
}

ClRcT clAmsPeDeleteSUReassignOp(ClAmsSUT *su)
{
    ClRcT rc = CL_OK;
    ClAmsSUReassignOpT *reassignEntry = NULL;
    if(!su) return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);
    rc = clAmsEntityOpClear(&su->config.entity, &su->status.entity,
                            CL_AMS_ENTITY_OP_SI_REASSIGN_MPLUSN,
                            (void**)&reassignEntry, NULL);
    if(rc == CL_OK)
    {
        if(reassignEntry->sis) clHeapFree(reassignEntry->sis);
        clHeapFree(reassignEntry);
    }
    return rc;
}

ClBoolT clAmsPeCheckSUReassignOp(ClAmsSUT *su, ClAmsSIT *si, ClBoolT deleteEntry)
{
    ClRcT rc = CL_OK;
    ClAmsSUReassignOpT *reassignEntry = NULL;
    ClInt32T i;
    if(!su || !si) return CL_FALSE;
    rc = clAmsEntityOpGet(&su->config.entity, &su->status.entity,
                          CL_AMS_ENTITY_OP_SI_REASSIGN_MPLUSN,
                          (void**)&reassignEntry, NULL);
    if(rc != CL_OK || !reassignEntry)
        return CL_FALSE;
    for(i = 0 ; i < reassignEntry->numSIs; ++i)
    {
        ClAmsEntityT *targetSI = reassignEntry->sis + i;
        if((targetSI->name.length ==
            si->config.entity.name.length) &&
           !strncmp(targetSI->name.value,
                    si->config.entity.name.value,
                    targetSI->name.length))
        {
            ClAmsEntityRefT *ref = NULL;
            /*
             * If the dependencies have invocations pending, then skip assignment
             */
            for(ref = clAmsEntityListGetFirst(&si->config.siDependenciesList);
                ref != NULL;
                ref = clAmsEntityListGetNext(&si->config.siDependenciesList, ref))
            {
                ClAmsSIT *dependencySI = (ClAmsSIT*)ref->ptr;
                ClAmsEntityRefT *suRef = NULL;
                ClBoolT reassignPending = clAmsEntityOpPending(&dependencySI->config.entity,
                                                               &dependencySI->status.entity,
                                                               CL_AMS_ENTITY_OP_SI_REASSIGN_MPLUSN);
                for(suRef = clAmsEntityListGetFirst(&dependencySI->status.suList);
                    suRef != NULL;
                    suRef = clAmsEntityListGetNext(&dependencySI->status.suList, suRef))
                {
                    ClAmsSUT *dependencySU = (ClAmsSUT*)suRef->ptr;
                    ClAmsNodeT *dependencyNode = (ClAmsNodeT*)dependencySU->config.parentNode.ptr;
                    if(!dependencyNode) continue;
                    if(dependencyNode->status.isClusterMember != CL_AMS_NODE_IS_CLUSTER_MEMBER)
                        continue;
                    if(reassignPending
                       ||
                       clAmsInvocationsSetPendingForSI(dependencySI, dependencySU))
                    {
                        if(reassignPending)
                        {
                            clLogTrace("CHECK", "REASSIGN", "SI [%s] has reassign op pending",
                                       dependencySI->config.entity.name.value);
                        }
                        else
                        {
                            clLogTrace("CHECK", "REASSIGN", "SU [%s] has invocation pending against SI [%s]",
                                       dependencySU->config.entity.name.value,
                                       dependencySI->config.entity.name.value);
                        }
                        return CL_FALSE;
                    }
                }
            }

            if(deleteEntry)
            {
                ClUint32T j;
                ClUint32T suSICount = 0;
                targetSI->name.length = 0;
                for(j = 0; j < reassignEntry->numSIs; ++j)
                    if(reassignEntry->sis[j].name.length) ++suSICount;

                if(!suSICount)
                {
                    clAmsPeDeleteSUReassignOp(su);
                }
                    
            }
            return CL_TRUE;
        }
    }
    return CL_FALSE;
}

ClRcT clAmsPeAddReassignOp(ClAmsSIT *targetSI, ClAmsSUT *targetSU) 
{
    ClAmsSIReassignOpT reassignSIOp = {{0}};
    ClAmsSUReassignOpT reassignSUOp = {0};
    ClAmsSUReassignOpT *reassignSUEntry = NULL;
    ClInt32T i;
    ClBoolT newEntry =  CL_FALSE;
    ClRcT rc = CL_OK;

    if(!targetSI || !targetSU) return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);

    memcpy(&reassignSIOp.su, &targetSU->config.entity, sizeof(targetSU->config.entity));

    clLogInfo("SI", "REASSIGN", "Delaying SI [%s] active reassignment till its dependencies are "
              "enabled", targetSI->config.entity.name.value);

    if(!clAmsEntityOpPending(&targetSI->config.entity, &targetSI->status.entity,
                             CL_AMS_ENTITY_OP_SI_REASSIGN_MPLUSN))
    {
        clAmsEntityOpPush(&targetSI->config.entity, &targetSI->status.entity,
                          (void*)&reassignSIOp, (ClUint32T)sizeof(reassignSIOp),
                          CL_AMS_ENTITY_OP_SI_REASSIGN_MPLUSN);
    }
    /*
     * Add an indirect one against this SI to the target SU.
     */
    rc = clAmsEntityOpGet(&targetSU->config.entity, &targetSU->status.entity,
                          CL_AMS_ENTITY_OP_SI_REASSIGN_MPLUSN, 
                          (void**)&reassignSUEntry, NULL);
    if(rc != CL_OK)
    {
        reassignSUEntry = &reassignSUOp;
        newEntry = CL_TRUE;
    }
    for(i = 0; i < reassignSUEntry->numSIs; ++i)
    {
        ClAmsEntityT *si = reassignSUEntry->sis + i;
        if(si->name.length == targetSI->config.entity.name.length
           &&
           !strncmp(si->name.value, targetSI->config.entity.name.value,
                    si->name.length))
            break;
    }

    if(i != reassignSUEntry->numSIs) return CL_OK;

    reassignSUEntry->sis = clHeapRealloc(reassignSUEntry->sis,
                                         (ClUint32T)sizeof(*reassignSUEntry->sis)
                                         * (reassignSUEntry->numSIs+1));
    CL_ASSERT(reassignSUEntry->sis != NULL);
    memcpy(reassignSUEntry->sis + reassignSUEntry->numSIs,
           &targetSI->config.entity, sizeof(*reassignSUEntry->sis));
    ++reassignSUEntry->numSIs;
    if(newEntry)
    {
        clAmsEntityOpPush(&targetSU->config.entity,
                          &targetSU->status.entity,
                          (void*)reassignSUEntry,
                          (ClUint32T)sizeof(*reassignSUEntry),
                          CL_AMS_ENTITY_OP_SI_REASSIGN_MPLUSN);
    }

    return CL_OK;
}

/*
 * clAmsPeSUSwitchoverCallback
 * ---------------------------
 * This fn is called when an SI switchover is completed. It waits till all SIs
 * have switched over. Since switchover can be invoked because of multiple causes
 * (locked, shutdown, fault, adjust) on multiple entities (su, node, cluster), 
 * this fn is a demuxer that figures out the cause and directs the callback
 * appropriately to the appropriate SU level cause handler. It is then the SU
 * level callback handler's job to propagate this callback up the entity chain
 * if necessary.
 *
 * Note that we may get multiple calls to this function, once when the SIs on
 * a SU are quiesced and once after they are removed.
 *
 * A switchover of a SU may leave its SG needs unsatisfied (SU is no longer in
 * service) and thus a gratiuitous call is made to SGEvaluateWork to take care
 * of any needs.
 */

ClRcT
clAmsPeSUSwitchoverCallback(
                            CL_IN       ClAmsSUT        *su,
                            CL_IN       ClUint32T       error,
                            CL_IN       ClUint32T       switchoverMode)
{
    ClAmsEntityRefT *entityRef;
    ClAmsSGT *sg;
    ClAmsNodeT *node;
    ClAmsAdminStateT sgAdminState;
    ClRcT rc = CL_OK;

    AMS_CHECK_SU ( su );
    AMS_CHECK_SG ( sg = (ClAmsSGT *) su->config.parentSG.ptr );
    AMS_CHECK_NODE ( node = (ClAmsNodeT *) su->config.parentNode.ptr );

    AMS_FUNC_ENTER ( ("SU [%s]\n",su->config.entity.name.value) );

    /*
     * Ensure that the SU has no active or standby assignments or any
     * of its components have any pending operations.
     */

    if ( su->status.numActiveSIs || su->status.numStandbySIs )
    {
        AMS_ENTITY_LOG(su, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
                       ("SU [%s] has [%d] assignments. Ignoring switchover callback..\n",
                        su->config.entity.name.value,
                        su->status.numActiveSIs + su->status.numStandbySIs));

        return CL_OK;
    }

    for ( entityRef = clAmsEntityListGetFirst(&su->config.compList);
          entityRef != (ClAmsEntityRefT *) NULL;
          entityRef = clAmsEntityListGetNext(&su->config.compList,entityRef) )
    {
        ClAmsCompT *comp = (ClAmsCompT *) entityRef->ptr;
        ClUint32T invocationsPendingForComp;

        AMS_CHECK_COMP ( comp );

        invocationsPendingForComp = clAmsInvocationsPendingForComp(comp);

        if ( comp->status.numActiveCSIs || 
             comp->status.numStandbyCSIs ||
             invocationsPendingForComp )
        {
            AMS_ENTITY_LOG(su, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
                           ("SU [%s] has components with assignments. Ignoring switchover callback..\n",
                            su->config.entity.name.value));

            return CL_OK;
        }
    }

    /*
     * At this point, any active SIs assigned to the SU are now quiesced. SIs
     * in any other state would have been removed.
     */

    AMS_ENTITY_LOG(su, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
                   ("SU [%s] switchover callback..\n",
                    su->config.entity.name.value));

    /*
     * If the SG for this component is shutting down, skip CSI reassignment.
     */

    sgAdminState = clAmsPeSGComputeAdminState(sg);
    
    if ( (sgAdminState == CL_AMS_ADMIN_STATE_SHUTTINGDOWN) ||
         (sgAdminState == CL_AMS_ADMIN_STATE_LOCKED_A) )
        goto SKIP_REASSIGNMENT;

    /*
     * If there are no assignments to this SU (this happens when SUswitchoverwork
     * is called for an SU with no assignments), skip reassignment.
     */

    if ( !su->status.siList.numEntities )
        goto SKIP_REASSIGNMENT;

    /*
     * fall thru to here means the CSIs assigned to components in this SU
     * must be reassigned. 
     */

#if NOTNEEDEDFORNOW

    /*
     * The for loop below only verifies that an inservice standby is
     * available for each active SI assigned to this SU.
     */

    for ( entityRef = clAmsEntityListGetFirst(&su->status.siList);
          entityRef != (ClAmsEntityRefT *) NULL;
          entityRef = clAmsEntityListGetNext(&su->status.siList, entityRef) )
    {
        ClAmsSIT *si = (ClAmsSIT *) entityRef->ptr;
        ClBoolT foundStandby = CL_FALSE;
        ClAmsSUT *standbySU;
        ClAmsEntityRefT *entityRef2;
        ClAmsAdminStateT adminState;

        AMS_CHECK_SI ( si );

        AMS_CALL ( clAmsPeSIComputeAdminState(si, &adminState) );

        if ( adminState != CL_AMS_ADMIN_STATE_UNLOCKED )
            continue;

        for ( entityRef2 = clAmsEntityListGetFirst(&si->status.suList);
              entityRef2 != (ClAmsEntityRefT *) NULL;
              entityRef2 = clAmsEntityListGetNext(&si->status.suList, entityRef2) )
        {
            ClAmsSISURefT *suRef = (ClAmsSISURefT *) entityRef2;
            ClAmsReadinessStateT rstate;

            AMS_CHECK_SU ( standbySU = (ClAmsSUT *) entityRef2->ptr );

            clAmsPeSUComputeReadinessState(standbySU, &rstate);
            CL_AMS_SET_R_STATE (standbySU, rstate);

            if ( suRef->haState == CL_AMS_HA_STATE_STANDBY )
            {
                if ( standbySU->status.readinessState ==
                     CL_AMS_READINESS_STATE_INSERVICE )
                {
                    foundStandby = CL_TRUE;
                    break;
                }
            }
        }

        if ( !foundStandby )
        {
            AMS_ENTITY_LOG(su, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
                           ("SI [%s] assigned to SU [%s] has no in-service standby SUs. Work reassignment not possible..\n", 
                            si->config.entity.name.value,
                            su->config.entity.name.value));

            CL_AMS_SET_O_STATE(si, CL_AMS_OPER_STATE_DISABLED);

            continue;
        }
    }
#endif // NOTNEEDEDFORNOW

    /*
     * If the SU has quiesced SIs, this is the first callback into this fn.
     * The quiesced SIs need to be reassigned and SG specific actions taken.
     */

    if ( su->status.numQuiescedSIs )
    {
        ClAmsSUT *activeSU =  NULL;
        ClBoolT reassignWork = CL_FALSE;
        /*
         * This is the main reassignment step and it necessary for all redundancy
         * models. We do this after removing other standbys if applicable (M+N)
         */
        clAmsPeSURemoveStandbyMPlusN(sg, su, switchoverMode | CL_AMS_ENTITY_SWITCHOVER_SU,
                                     error,
                                     &activeSU, &reassignWork);
        
        if(reassignWork == CL_TRUE)
        {
            ClBoolT allCSIsReassigned = CL_TRUE;
            ClBoolT *pAllCSIsReassigned = &allCSIsReassigned;
            CL_LIST_HEAD_DECLARE(dependentSIList);

            clAmsPeSUSIDependentsListMPlusN(su, &activeSU, &dependentSIList);

            for ( entityRef = clAmsEntityListGetFirst(&su->config.compList);
                  entityRef != (ClAmsEntityRefT *) NULL;
                  entityRef = clAmsEntityListGetNext(&su->config.compList,entityRef) )
            {
                ClAmsCompT *comp = (ClAmsCompT *) entityRef->ptr;
                
                AMS_CHECK_COMP ( comp );

                rc = clAmsPeCompReassignWork(comp, &activeSU, &dependentSIList, pAllCSIsReassigned);

                if(rc != CL_OK)
                {
                    clAmsPeSIReassignEntryDelete(&dependentSIList);
                    clLogError("SU", "SWITCHOVER", "Comp reassign work for SU [%s] returned [%#x]",
                               su->config.entity.name.value, rc);
                    return rc;
                }

                if(pAllCSIsReassigned && (!*pAllCSIsReassigned))
                    pAllCSIsReassigned = NULL;
            }

            clAmsPeSIReassignEntryListDelete(&dependentSIList);
            /*
             * Now if all CSIs haven't been reassigned, push the failover SU context
             * for delaying removes after all standbys are reassigned.
             */
            if(!allCSIsReassigned && 
               activeSU && 
               su->status.siList.numEntities)
            {
                ClAmsEntityRemoveOpT activeRemoveOp = {{0}};
                memcpy(&activeRemoveOp.entity, &su->config.entity, sizeof(activeRemoveOp.entity));
                activeRemoveOp.sisRemoved = su->status.siList.numEntities;
                activeRemoveOp.switchoverMode = switchoverMode;
                activeRemoveOp.error = error;
                clAmsEntityOpPush(&activeSU->config.entity, 
                                  &activeSU->status.entity,
                                  (void*)&activeRemoveOp, 
                                  (ClUint32T)sizeof(activeRemoveOp),
                                  CL_AMS_ENTITY_OP_ACTIVE_REMOVE_MPLUSN);
                memcpy(&activeRemoveOp.entity, &activeSU->config.entity, sizeof(activeRemoveOp.entity));
                /*
                 * Store a back reference.
                 */
                clAmsEntityOpPush(&su->config.entity, &su->status.entity,
                                  (void*)&activeRemoveOp, (ClUint32T)sizeof(activeRemoveOp),
                                  CL_AMS_ENTITY_OP_ACTIVE_REMOVE_REF_MPLUSN);
                clLogInfo("SU", "SWITCHOVER", "Deferring SU [%s] CSI removes after "
                          "reassignment to SU [%s] as active", 
                          su->config.entity.name.value,
                          activeSU->config.entity.name.value);
                return CL_OK;
            }
        }
        else
        {
            /*
             * if reassign work is false, we have an entity op pending.
             */
            if(activeSU)
            {
                clLogInfo("SU", "SWITCHOVER", "Deferring SU switchover till standby assignment "
                          "is removed from SU [%s]", activeSU->config.entity.name.value);
                return CL_OK;
            }
        }

#if 0
            /*
             * Optional steps for other redundancy models
             */

            switch ( sg->config.redundancyModel )
            {
                /*
                 * The M + N model has a requirement that the a SU should only have
                 * active or standby assignments. So, if there are any SUs after the
                 * above reassignment step that have old standby assignments, then
                 * they should be removed.
                 */

            case CL_AMS_SG_REDUNDANCY_MODEL_M_PLUS_N:
                {
                    ClAmsEntityRefT *eRef;
                    ClAmsEntityRefT *eRef2;

                    /*
                     * Instead of trying to figure out which SUs were affected by
                     * the reassignment step, we go through all of the SUs in the
                     * list. The affected SUs are the ones with both active and
                     * standby assignments. 
                     */

                    eRef = clAmsEntityListGetFirst(&sg->status.assignedSUList);

                    while ( eRef != (ClAmsEntityRefT *) NULL )
                    {
                        ClAmsSUT *tmpSU = (ClAmsSUT *) eRef->ptr;

                        eRef = clAmsEntityListGetNext(&sg->status.assignedSUList, eRef);

                        if ( !tmpSU->status.numActiveSIs || !tmpSU->status.numStandbySIs )
                        {
                            continue;
                        }

                        eRef2 = clAmsEntityListGetFirst(&tmpSU->status.siList);

                        while ( eRef2 != (ClAmsEntityRefT *) NULL )
                        {
                            ClAmsSUSIRefT *siRef = (ClAmsSUSIRefT *) eRef2;
                            ClAmsSIT *tmpSI = (ClAmsSIT *) eRef2->ptr;

                            eRef2 = clAmsEntityListGetNext(&tmpSU->status.siList, eRef2);

                            if ( siRef->haState == CL_AMS_HA_STATE_STANDBY )
                            {
                                AMS_CALL ( clAmsPeSURemoveSI(tmpSU, tmpSI, switchoverMode | 
                                                             CL_AMS_ENTITY_SWITCHOVER_SU) );
                            }
                        }
                    }

                    break;
                }

            default:
                {
                    // just to stop the compiler from complaining
                }
            }
#endif
    }

    /*
     * Rank adjustment step
     *
     * In the case of the n-way model, the switchover of a SU with active SI
     * assignments results in the standby with rank = 1 becoming active and
     * the ranks of other standbys must be adjusted. Similarly, the switchover
     * of a sU with standby SI assignments results in the loss of a standby
     * with a certain rank and all other SUs with higher ranks for the same
     * SI must be adjusted. 
     *
     * Note that the above two cases are not disjoint since a SU can have
     * both active and standby assignments.
     *
     * We take a brute force approach as the first part of switchover results
     * in the removal of standby SIs and they are no longer part of the SUs
     * SI list.
     */

    if ( sg->config.redundancyModel == CL_AMS_SG_REDUNDANCY_MODEL_N_WAY )
    {
        ClAmsEntityRefT *eRef1, *eRef2;

        for ( eRef1 = clAmsEntityListGetFirst(&sg->config.siList);
              eRef1 != (ClAmsEntityRefT *) NULL;
              eRef1 = clAmsEntityListGetNext(&sg->config.siList, eRef1) )
        {
            ClAmsSIT *sSI = (ClAmsSIT *)eRef1->ptr;
            
            AMS_CHECK_SI ( sSI );

            for ( eRef2 = clAmsEntityListGetFirst(&sSI->status.suList);
                  eRef2 != (ClAmsEntityRefT *) NULL;
                  eRef2 = clAmsEntityListGetNext(&sSI->status.suList, eRef2) )
            {
                ClAmsSISURefT *suRef = (ClAmsSISURefT *) eRef2;
                ClAmsSUT *sSU = (ClAmsSUT *) eRef2->ptr;

                if ( suRef->haState == CL_AMS_HA_STATE_STANDBY )
                {
                    AMS_CALL ( clAmsPeSUAssignSI(sSU, sSI, CL_AMS_HA_STATE_STANDBY) );
                }
            }
        }
    }

    SKIP_REASSIGNMENT:

    if ( su->status.numQuiescedSIs )
    {
        for ( entityRef = clAmsEntityListGetFirst(&su->config.compList);
              entityRef != (ClAmsEntityRefT *) NULL;
              entityRef = clAmsEntityListGetNext(&su->config.compList,entityRef) )
        {
            AMS_CALL ( clAmsPeCompRemoveWork( (ClAmsCompT*)entityRef->ptr,
                                              switchoverMode | CL_AMS_ENTITY_SWITCHOVER_SU));
        }
    }
    else
    {
        clAmsPeSUSwitchoverPrologue(su, error, switchoverMode | CL_AMS_ENTITY_SWITCHOVER_SU);
    }
    return CL_OK;
}


/*
 * clAmsPeSURemoveWork
 * -------------------
 * Walk the list of SIs assigned to this SU and remove them.
 */

ClRcT
clAmsPeSURemoveWork(
        CL_IN ClAmsSUT *su,
        CL_IN ClUint32T switchoverMode)
{
    ClAmsEntityRefT *entityRef;

    AMS_CHECK_SU(su);

    AMS_FUNC_ENTER ( ("SU [%s]\n",su->config.entity.name.value) );

    if ( !su->status.numActiveSIs && 
         !su->status.numStandbySIs &&
         !su->status.numQuiescedSIs )
    {
        AMS_ENTITY_LOG(su, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
                ("SU [%s] has no assignments. Ignoring remove work request..\n",
                 su->config.entity.name.value));

        return clAmsPeSURemoveCallback(su, CL_OK, switchoverMode);
    }
 
    entityRef = clAmsEntityListGetFirst(&su->status.siList);

    while ( entityRef != (ClAmsEntityRefT *) NULL )
    {
        ClAmsSIT *si;

        si = (ClAmsSIT *) entityRef->ptr;
        entityRef = clAmsEntityListGetNext(&su->status.siList, entityRef);

        AMS_CALL ( clAmsPeSURemoveSI(su, si, switchoverMode) );
    }

    return CL_OK;
}

ClRcT
clAmsPeSURemoveCallback(
        CL_IN ClAmsSUT *su,
        CL_IN ClUint32T error,
        CL_IN ClUint32T switchoverMode)
{
    
    return CL_OK;
}

/*-----------------------------------------------------------------------------
 * SU SI management functions
 *---------------------------------------------------------------------------*/

static ClBoolT clAmsPeCheckCSIDependentsAssigned(ClAmsSUT *su, ClAmsCSIT *csi)
{
    ClAmsEntityRefT *entityRef = NULL;
    for(entityRef = clAmsEntityListGetFirst(&csi->config.csiDependentsList);
        entityRef != NULL;
        entityRef = clAmsEntityListGetNext(&csi->config.csiDependentsList, entityRef))
    {
        ClAmsCSIT *dependentCSI = (ClAmsCSIT*)entityRef->ptr;
        ClAmsEntityRefT *compRef = NULL;
        /*
         * Check for components assigned CSI for the given SU
         */
        for(compRef = clAmsEntityListGetFirst(&dependentCSI->status.pgList);
            compRef != NULL;
            compRef = clAmsEntityListGetNext(&dependentCSI->status.pgList, compRef))
        {
            ClAmsCompT *comp = (ClAmsCompT*)compRef->ptr;
            ClAmsSUT *parentSU = (ClAmsSUT*)comp->config.parentSU.ptr;
            if(parentSU == su) return CL_TRUE;
        }
    }
    return CL_FALSE;
}

static __inline__ ClRcT
clAmsPeGetPendingOp(ClAmsHAStateT haState)
{
    switch(haState)
    {
    case CL_AMS_HA_STATE_ACTIVE:
    case CL_AMS_HA_STATE_STANDBY:
    case CL_AMS_HA_STATE_QUIESCED:
        return CL_AMS_CSI_SET_CALLBACK;

    case CL_AMS_HA_STATE_QUIESCING:
        return CL_AMS_CSI_QUIESCING_CALLBACK;

    default: break;
    }
    return CL_AMS_CSI_RMV_CALLBACK;
}

ClRcT
clAmsPeSUAssignSIDependencyCallback(
                                    CL_IN ClAmsSUT *su,
                                    CL_IN ClAmsCSIT *csi,
                                    CL_IN ClAmsHAStateT haState,
                                    ClUint32T switchoverMode,
                                    ClBoolT   reassignCSI)
{
    ClAmsEntityRefT *entityRef = NULL;
    ClRcT rc = CL_OK;
    ClAmsSIT *si = NULL;
    ClAmsSUSIRefT *siRef = NULL;
    ClAmsSISURefT *suRef = NULL;
    ClAmsSUT *activeSU = NULL;
    ClUint32T standbyRank = 0;
    ClAmsEntityListTypeT listType = 0;
    ClAmsEntityListT *list = NULL;
    ClUint32T standbyAssignmentOrder = 0;
    ClAmsInvocationCmdT pendingOp = 0;

    AMS_CHECK_SU (su);

    AMS_FUNC_ENTER ( ("SU [%s]\n",su->config.entity.name.value) );

    if(!csi) return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);

    AMS_CHECK_SI ( si = (ClAmsSIT*)csi->config.parentSI.ptr);

    standbyAssignmentOrder = si->config.standbyAssignmentOrder;

    if(haState == CL_AMS_HA_STATE_ACTIVE)
    {
        if(!csi->config.csiDependentsList.numEntities)
            return CL_OK;
    }
    else
    {
        if(haState == CL_AMS_HA_STATE_STANDBY)
        {
            if(standbyAssignmentOrder)
            {
                if(!csi->config.csiDependenciesList.numEntities)
                    return CL_OK;
            }
            else
            {
                if(!csi->config.csiDependentsList.numEntities)
                    return CL_OK;
            }
        }
        else
        {
            if(!csi->config.csiDependenciesList.numEntities)
                return CL_OK;
        }
    }

    rc = clAmsEntityListFindEntityRef2(
                                       &su->status.siList, 
                                       &si->config.entity,
                                       0, 
                                       (ClAmsEntityRefT **)&siRef);

    if(rc != CL_OK)
        return rc;

    rc = clAmsEntityListFindEntityRef2(
                                       &si->status.suList, 
                                       &su->config.entity,
                                       0, 
                                       (ClAmsEntityRefT **)&suRef);
    if(rc != CL_OK)
        return rc;

    pendingOp = clAmsPeGetPendingOp(haState);

    if ( haState == CL_AMS_HA_STATE_ACTIVE )
    {
        activeSU = su;
        standbyRank = 0;
        listType = CL_AMS_CSI_CONFIG_CSI_DEPENDENTS_LIST;
        list = &csi->config.csiDependentsList;
    }
    else 
    {
        listType = CL_AMS_CSI_CONFIG_CSI_DEPENDENCIES_LIST;
        list = &csi->config.csiDependenciesList;
        if ( haState == CL_AMS_HA_STATE_STANDBY )
        {
            if(!standbyAssignmentOrder)
            {
                /*
                 * Follow the same as active.
                 */
                listType = CL_AMS_CSI_CONFIG_CSI_DEPENDENTS_LIST;
                list = &csi->config.csiDependentsList;
            }
            activeSU = NULL;
            standbyRank = 1;

            for ( entityRef = clAmsEntityListGetFirst(&si->status.suList);
                  entityRef != (ClAmsEntityRefT *) NULL;
                  entityRef = clAmsEntityListGetNext(&si->status.suList, entityRef) )
            {
                ClAmsSISURefT *sRef = (ClAmsSISURefT *) entityRef;

                if ( sRef->haState == CL_AMS_HA_STATE_ACTIVE 
                     ||
                     sRef->haState == CL_AMS_HA_STATE_QUIESCING)
                {
                    activeSU = (ClAmsSUT *) entityRef->ptr;
                }

                if ( sRef->haState == CL_AMS_HA_STATE_STANDBY )
                {
                    if ( sRef->rank < siRef->rank )
                    {
                        standbyRank++;
                    }
                }
            }

            if ( !activeSU )
            {
                AMS_ENTITY_LOG (su, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
                                ("Warning: Assigning SI [%s] with HA state [Standby] to SU [%s] but no active assignment found.\n",
                                 si->config.entity.name.value,
                                 su->config.entity.name.value));

                activeSU = su;
            }
        }
    }

    if(haState == CL_AMS_HA_STATE_ACTIVE 
       ||
       haState == CL_AMS_HA_STATE_STANDBY)
    {
        siRef->rank = standbyRank;
        suRef->rank = standbyRank;
    }

    for(entityRef = clAmsEntityListGetFirst(list);
        entityRef != NULL;
        entityRef = clAmsEntityListGetNext(list, entityRef))
    {
        ClAmsCSIT *csiRef = (ClAmsCSIT*)entityRef->ptr;
        ClAmsEntityRefT *entityRef2 = NULL;
        ClAmsEntityListT *list2 = NULL;
        ClAmsCompCSIRefT *foundCSIRef = NULL;
        ClAmsCompT *comp = NULL;

        if(listType == CL_AMS_CSI_CONFIG_CSI_DEPENDENTS_LIST)
        {
            list2 = &csiRef->config.csiDependenciesList;
        }
        else
        {
            list2 = &csiRef->config.csiDependentsList;
        }
        for(entityRef2 = clAmsEntityListGetFirst(list2);
            entityRef2 != NULL;
            entityRef2 = clAmsEntityListGetNext(list2, entityRef2))
        {
            ClAmsCSIT *csiRef2 = (ClAmsCSIT*)entityRef2->ptr;
            ClAmsEntityRefT *entityRef3 = NULL;
            if(csiRef2 == csi) continue;
            /*
             * Check if there a pending invocation for this. Skip assignment if any
             */
            if(clAmsInvocationPendingForCSI(su, NULL, csiRef2, pendingOp) == CL_TRUE)
                goto skip;

            for(entityRef3 = clAmsEntityListGetFirst(list);
                entityRef3 != NULL;
                entityRef3 = clAmsEntityListGetNext(list, entityRef3))
            {
                ClAmsCSIT *csiRef3 = (ClAmsCSIT*)entityRef3->ptr;
                if(csiRef2 == csiRef3) goto skip;
            }
        }
        /*
         * We are here when the dependency or dependent are not in each others list.
         */
        if(haState == CL_AMS_HA_STATE_ACTIVE ||
           haState == CL_AMS_HA_STATE_STANDBY)
        {
            if(clAmsPeSUFindCompAndCSIWithCSIAssignment(su, csiRef, &comp, (ClAmsEntityRefT**)&foundCSIRef) != CL_OK)
            {
                if(clAmsPeSUFindCompForCSIAssignment(su, csiRef, haState, &comp) != CL_OK)
                {
                    clLogWarning("SU", "DEP-ASSIGN-SI", "Didnt find component to assign CSI [%s] with haState [%s]",
                                 csiRef->config.entity.name.value,
                                 CL_AMS_STRING_H_STATE(haState));
                }
            }
        }
        else
        {
            if(clAmsPeSUFindCompAndCSIWithCSIAssignment(su, csiRef, &comp, (ClAmsEntityRefT**)&foundCSIRef) != CL_OK)
            {
                if(haState != CL_AMS_HA_STATE_NONE)
                {
                    clLogWarning("SU", "DEP-ASSIGN-SI", "Didnt find component to assign CSI [%s] with haState [%s]",
                                 csiRef->config.entity.name.value, CL_AMS_STRING_H_STATE(haState));
                }
                else
                {
                    clLogDebug("SU", "DEP-ASSIGN-SI", "Didnt find component to assign CSI [%s] with haState [%s]",
                               csiRef->config.entity.name.value, CL_AMS_STRING_H_STATE(haState));
                }
            }
        }

        if(comp)
        {
            switch(haState)
            {
            case CL_AMS_HA_STATE_ACTIVE:
            case CL_AMS_HA_STATE_STANDBY:
                {
                    clLogDebug("SU", "DEP-ASSIGN-SI", "Assigning CSI [%s] to component [%s] with ha state [%s]",
                               csiRef->config.entity.name.value, comp->config.entity.name.value, 
                               CL_AMS_STRING_H_STATE(haState));
                    clAmsPeCompAssignCSI(comp, csiRef, haState);
                }
                break;
            case CL_AMS_HA_STATE_QUIESCING:
                {
                    clLogDebug("SU", "DEP-ASSIGN-SI", "Assigning CSI [%s] to component [%s] with ha state [%s]",
                               csiRef->config.entity.name.value, comp->config.entity.name.value,
                               CL_AMS_STRING_H_STATE(haState));
                    switchoverMode = CL_AMS_ENTITY_SWITCHOVER_GRACEFUL;
                    clAmsPeCompComputeSwitchoverMode(comp, &switchoverMode);
                    clAmsPeCompQuiesceCSIGracefully(comp, csiRef, switchoverMode);
                }
                break;
            case CL_AMS_HA_STATE_QUIESCED:
                {
                    clLogDebug("SU", "DEP-ASSIGN-SI", "Assigning CSI [%s] to component [%s] with ha state [%s]",
                               csiRef->config.entity.name.value, comp->config.entity.name.value, 
                               CL_AMS_STRING_H_STATE(haState));
                    switchoverMode = CL_AMS_ENTITY_SWITCHOVER_IMMEDIATE;
                    clAmsPeCompComputeSwitchoverMode(comp, &switchoverMode);
                    clAmsPeCompQuiesceCSIImmediately(comp, csiRef, switchoverMode);
                }
                break;
            case CL_AMS_HA_STATE_NONE:
                {
                    clLogDebug("SU", "DEP-ASSIGN-SI", "Removing CSI [%s] from component [%s]",
                               csiRef->config.entity.name.value, comp->config.entity.name.value);
                    switchoverMode = CL_AMS_ENTITY_SWITCHOVER_IMMEDIATE;
                    clAmsPeCompRemoveCSI(comp, csiRef, switchoverMode);
                }
                break;
            default:
                {
                    clLogError("SU", "DEP-ASSIGN-SI", "SU assign SI dependency callback invoked with "
                               "invalid ha state [%d]", haState);
                }
                break;
            }
        }
        skip: ;
    }
    return CL_OK;
}

/*
 * clAmsPeSUAssignSI
 * -----------------
 * Given a SI to assign active or standby to a SU, this function breaks down 
 * the SI into its constituent CSIs, finds appropriate components to assign 
 * them and makes the assignment.
 */

ClRcT
clAmsPeSUAssignSI(
                  CL_IN ClAmsSUT *su,
                  CL_IN ClAmsSIT *si,
                  CL_IN ClAmsHAStateT haState)
{
    ClAmsEntityRefT *entityRef;
    ClRcT rc;
    ClAmsSUSIRefT *siRef;
    ClAmsSISURefT *suRef;
    ClAmsSUT *activeSU;
    ClUint32T standbyRank;
    ClUint32T standbyAssignmentOrder = 0;

    AMS_CHECK_SU (su);
    AMS_CHECK_SI (si);

    AMS_FUNC_ENTER ( ("SU [%s]\n",su->config.entity.name.value) );

    /*
     * Step 1: Verify if this SI is already assigned to this SU.  If not, 
     * then create an appropriate siRef. If yes, just reuse the existing 
     * siRef.  The second case happens when a SI is reassigned to a SU 
     * during restart or after a switchover with a haState change or new
     * rank.
     */

    standbyAssignmentOrder = si->config.standbyAssignmentOrder;

    rc = clAmsEntityListFindEntityRef2(
                                       &su->status.siList, 
                                       &si->config.entity,
                                       0, 
                                       (ClAmsEntityRefT **)&siRef);

    if ( CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST )
    {
        /*
         * This is a new assignment. 
         *
         * If this is an active assignment, then the rank of this assignment is 
         * 0 and SU is the active SU. 
         *
         * If this is a standby assignment, find the SU with active SI assignment. 
         * If no such SU is found, print out a warning. Also, compute the standby 
         * rank. It is assumed that the ranks are always contiguous and start at 1.
         */

        AMS_ENTITY_LOG (su, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
                        ("Assigning SI [%s] to SU [%s] with HA State [%s]\n",
                         si->config.entity.name.value,
                         su->config.entity.name.value,
                         CL_AMS_STRING_H_STATE(haState)));

        if ( haState == CL_AMS_HA_STATE_ACTIVE )
        {
            activeSU = su;
            standbyRank = 0;
        }
        else if ( haState == CL_AMS_HA_STATE_STANDBY )
        {
            activeSU = NULL;
            standbyRank = 1;

            for ( entityRef = clAmsEntityListGetFirst(&si->status.suList);
                  entityRef != (ClAmsEntityRefT *) NULL;
                  entityRef = clAmsEntityListGetNext(&si->status.suList, entityRef) )
            {
                ClAmsSISURefT *sRef = (ClAmsSISURefT *) entityRef;

                if ( sRef->haState == CL_AMS_HA_STATE_ACTIVE 
                     ||
                     sRef->haState == CL_AMS_HA_STATE_QUIESCING)
                {
                    activeSU = (ClAmsSUT *) entityRef->ptr;
                }

                if ( sRef->haState == CL_AMS_HA_STATE_STANDBY )
                {
                    standbyRank++;
                }
            }

            if ( !activeSU )
            {
                AMS_ENTITY_LOG (su, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
                                ("Warning: Assigning SI [%s] with HA state [Standby] to SU [%s] but no active assignment found.\n",
                                 si->config.entity.name.value,
                                 su->config.entity.name.value));

                activeSU = su;
            }
        }
        else
        {
            return CL_AMS_RC(CL_ERR_INVALID_STATE);
        }

        /*
         * Add a new siRef to this SU and then set up the siRef appropriately. The
         * state of the assignments is updated once the CSIs are assigned.
         */

        AMS_CALL (clAmsSUAddSIRefToSIList(
                                          &su->status.siList,
                                          si,
                                          CL_AMS_HA_STATE_NONE));

        AMS_CALL (clAmsSIAddSURefToSUList(
                                          &si->status.suList,
                                          su,
                                          CL_AMS_HA_STATE_NONE));

        AMS_CALL ( clAmsEntityListFindEntityRef2(
                                                 &su->status.siList, 
                                                 &si->config.entity,
                                                 0, 
                                                 (ClAmsEntityRefT **)&siRef) );

        siRef->rank = standbyRank;

        AMS_CALL ( clAmsEntityListFindEntityRef2(
                                                 &si->status.suList, 
                                                 &su->config.entity,
                                                 0, 
                                                 (ClAmsEntityRefT **)&suRef) );

        suRef->rank = standbyRank;

        /*
         * Walk the list of CSIs in the SI (this list is ordered by CSI rank)
         * and find a compatible component with the same CSI type and with
         * capacity to host the new CSI.
         *
         * Note, any error below will leave the SI counts updated and SI on
         * the assigned list but no CSI assignments. This should not happen
         * unless there is a config error and that should be caught in CW
         * or in the AMS validation fns, so we will let this slide.
         */
     
        for ( entityRef = clAmsEntityListGetFirst(&si->config.csiList);
              entityRef != (ClAmsEntityRefT *) NULL;
              entityRef = clAmsEntityListGetNext(&si->config.csiList, entityRef) )
        {
            ClAmsCSIT *csi = (ClAmsCSIT *) entityRef->ptr;
            ClAmsCompT *comp;
            ClRcT rc2;

            AMS_CHECK_CSI ( csi );

            /*
             * Assign CSI without dependencies first. for active HA state
             * and do the reverse while assigning other HA states
             */
            if(haState == CL_AMS_HA_STATE_ACTIVE)
            {
                if(csi->config.csiDependenciesList.numEntities > 0)
                    continue;
            }
            else
            {
                if(haState == CL_AMS_HA_STATE_STANDBY)
                {
                    if(standbyAssignmentOrder)
                    {
                        if(csi->config.csiDependentsList.numEntities > 0)
                            continue;
                    }
                    else
                    {
                        if(csi->config.csiDependenciesList.numEntities > 0)
                            continue;
                    }
                }
                else
                {
                    if(csi->config.csiDependentsList.numEntities > 0)
                        continue;
                }
            }
            
            rc2 = clAmsPeSUFindCompForCSIAssignment(su, csi, haState, &comp);
            
            if ( rc2 != CL_OK )
            {
                if ( CL_GET_ERROR_CODE(rc2) == CL_AMS_ERR_CSI_NOT_ASSIGNABLE )
                {
                    AMS_ENTITY_LOG(su, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_ERROR,
                                   ("Error: No component available for CSI [%s]. Config Error? Exiting..\n",
                                    csi->config.entity.name.value));
                }
                else if ( CL_GET_ERROR_CODE(rc2) == CL_ERR_DOESNT_EXIST )
                {
                    AMS_ENTITY_LOG(su, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_ERROR,
                                   ("Error: CSI [%s]'s type is unsupported. Config Error? Exiting..\n",
                                    csi->config.entity.name.value));
                }
                else
                {
                    AMS_ENTITY_LOG(su, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_ERROR,
                                   ("Error: Error [0x%x] finding component for CSI [%s]. Exiting..\n",
                                    rc2,
                                    csi->config.entity.name.value));
                }

                clAmsSUDeleteSIRefFromSIList(
                                             &su->status.siList,
                                             si);

                clAmsSIDeleteSURefFromSUList(
                                             &si->status.suList,
                                             su);

                return rc2;
            }

            if(comp->config.property == CL_AMS_COMP_PROPERTY_PROXIED_PREINSTANTIABLE
               ||
               comp->config.property == CL_AMS_COMP_PROPERTY_PROXIED_NON_PREINSTANTIABLE
               )
            {
                if(!comp->status.proxyComp)
                {
                    if(!clAmsInvocationsPendingForSI(si, su))
                    {
                        clAmsSUDeleteSIRefFromSIList(
                                                     &su->status.siList,
                                                     si);
                        clAmsSIDeleteSURefFromSUList(
                                                     &si->status.suList,
                                                     su);
                    }
                    continue;
                }
            }

            AMS_CALL ( clAmsPeCompAssignCSI(comp, csi, haState) );
        }
    }
    else if ( rc == CL_OK )
    {
        /*
         * This is a reassignment of the SI to this SU with possibly updated
         * state and rank. 
         *
         * If this is a standby assignment, find the SU with the active SI
         * assignment. If no such SU is found, print out a warning. Also,
         * compute the new standby rank of this SI by counting the number
         * of SI assignments with lower standby ranks.
         *
         * Note: For now this case will not be invoked as we have a separate
         * function to reassign work, but goal is to start using this later.
         */

        AMS_ENTITY_LOG (su, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
                        ("Re-Assigning SI [%s] to SU [%s] with HA State [%s]\n",
                         si->config.entity.name.value,
                         su->config.entity.name.value,
                         CL_AMS_STRING_H_STATE(haState)));

        if ( haState == CL_AMS_HA_STATE_ACTIVE )
        {
            activeSU = su;
            standbyRank = 0;
        }
        else if ( haState == CL_AMS_HA_STATE_STANDBY )
        {
            activeSU = NULL;
            standbyRank = 1;

            for ( entityRef = clAmsEntityListGetFirst(&si->status.suList);
                  entityRef != (ClAmsEntityRefT *) NULL;
                  entityRef = clAmsEntityListGetNext(&si->status.suList, entityRef) )
            {
                ClAmsSISURefT *sRef = (ClAmsSISURefT *) entityRef;

                if ( sRef->haState == CL_AMS_HA_STATE_ACTIVE 
                     ||
                     sRef->haState == CL_AMS_HA_STATE_QUIESCING)
                {
                    activeSU = (ClAmsSUT *) entityRef->ptr;
                }

                if ( sRef->haState == CL_AMS_HA_STATE_STANDBY )
                {
                    if ( sRef->rank < siRef->rank )
                    {
                        standbyRank++;
                    }
                }
            }

            if ( !activeSU )
            {
                AMS_ENTITY_LOG (su, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
                                ("Warning: Assigning SI [%s] with HA state [Standby] to SU [%s] but no active assignment found.\n",
                                 si->config.entity.name.value,
                                 su->config.entity.name.value));

                activeSU = su;
            }
        }
        else
        {
            return CL_AMS_RC(CL_ERR_INVALID_STATE);
        }

        siRef->rank = standbyRank;

        AMS_CALL ( clAmsEntityListFindEntityRef2(
                                                 &si->status.suList, 
                                                 &su->config.entity,
                                                 0, 
                                                 (ClAmsEntityRefT **)&suRef) );

        suRef->rank = standbyRank;

        /*
         * Walk the list of CSIs in the SI (this list is ordered by CSI rank)
         * and assign a CSI back to the component to which it was previously
         * assigned. In the case of standby assignments, the rank may now
         * change, this is taken care of in the CSI assign function, using
         * logic similar to the one above for SI rank.
         */

        for ( entityRef = clAmsEntityListGetFirst(&si->config.csiList);
              entityRef != (ClAmsEntityRefT *) NULL;
              entityRef = clAmsEntityListGetNext(&si->config.csiList, entityRef) )
        {
            ClAmsCSIT *csi = (ClAmsCSIT *) entityRef->ptr;
            ClAmsCompT *comp;
            
            if(haState == CL_AMS_HA_STATE_ACTIVE)
            {
                if(csi->config.csiDependenciesList.numEntities > 0 )
                {
                    continue;
                }
            }
            else
            {
                if(haState == CL_AMS_HA_STATE_STANDBY)
                {
                    if(standbyAssignmentOrder)
                    {
                        if(csi->config.csiDependentsList.numEntities > 0)
                            continue;
                    }
                    else
                    {
                        if(csi->config.csiDependenciesList.numEntities > 0)
                            continue;
                    }
                }
                else
                {
                    if(csi->config.csiDependentsList.numEntities > 0 )
                    {
                        continue;
                    }
                }
            }

            AMS_CALL ( clAmsPeSUFindCompWithCSIAssignment(su, csi, &comp) );

            if(comp->config.property == CL_AMS_COMP_PROPERTY_PROXIED_PREINSTANTIABLE
               ||
               comp->config.property == CL_AMS_COMP_PROPERTY_PROXIED_NON_PREINSTANTIABLE
               )
            {
                if(!comp->status.proxyComp)
                {
                    continue;
                }
            }

            AMS_CALL ( clAmsPeCompAssignCSI(comp, csi, haState) );
        }
    }
    else
    {
        /*
         * Got an error while searching for SI assignment.
         */

        AMS_ENTITY_LOG (su, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_ERROR,
                        ("Error [0x%x]: Assigning SI [%s] to SU [%s] with HA State [%s]\n",
                         rc,
                         si->config.entity.name.value,
                         su->config.entity.name.value,
                         CL_AMS_STRING_H_STATE(haState)));

        return rc;
    }


    return CL_OK;
}

/*
 * clAmsPeSUAssignSIError
 * ----------------------
 * This fn is called when a CSI assignment fails and thus it is known that
 * the entire SI assignment is also going to fail. At the time the function
 * is called, it is possible that there are other pending CSI assignments
 * that have not yet been confirmed.
 */

ClRcT
clAmsPeSUAssignSIError(
        CL_IN   ClAmsSUT *su,
        CL_IN   ClAmsSIT *si,
        CL_IN   ClRcT error)
{
    ClAmsSUSIRefT *siRef;

    AMS_CHECK_SU ( su );
    AMS_CHECK_SI ( si );

    AMS_FUNC_ENTER ( ("SU [%s]\n",su->config.entity.name.value) );

    AMS_ENTITY_LOG(su, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_ERROR,
        ("SI [%s] assignment to SU [%s] returned Error [0x%x]\n",
         si->config.entity.name.value,
         su->config.entity.name.value,
         error) );

    AMS_CALL ( clAmsEntityListFindEntityRef2(
                    &su->status.siList,
                    &si->config.entity,
                    0,
                    (ClAmsEntityRefT **)&siRef) );

    if ( su->status.recovery == CL_AMS_RECOVERY_SU_RESTART )
    {
        if ( su->status.operState == CL_AMS_OPER_STATE_ENABLED )
        {
            AMS_CALL ( clAmsPeSUFaultCallback_Step2(su, error) );
        }
    }

    return CL_OK;
}

/*
 * clAmsPeSUAssignSICallback
 * -------------------------
 * This fn is called when confirmation is received that all CSIs in a SI
 * have been successfully assigned to a SU.
 */

ClRcT
clAmsPeSUAssignSICallback(
        CL_IN   ClAmsSUT *su,
        CL_IN   ClAmsSIT *si,
        CL_IN   ClRcT error)
{
    ClAmsSUSIRefT *siRef;
    void *siReassignEntry = NULL;
    ClRcT rc = CL_OK;

    AMS_CHECK_SU ( su );
    AMS_CHECK_SI ( si );

    AMS_FUNC_ENTER ( ("SU [%s]\n",su->config.entity.name.value) );

    clAmsEntityOpClear(&si->config.entity, &si->status.entity,
                       CL_AMS_ENTITY_OP_SI_REASSIGN_MPLUSN,
                       &siReassignEntry, NULL);
    if(siReassignEntry)
    {
        clHeapFree(siReassignEntry);
    }

    if ( error )
    {
        clAmsPeEntityOpReplay(&su->config.entity, &su->status.entity, 
                              CL_AMS_ENTITY_OP_ACTIVE_REMOVE_MPLUSN, CL_TRUE);
        clAmsPeEntityOpReplay(&su->config.entity, &su->status.entity,
                              CL_AMS_ENTITY_OP_SWAP_ACTIVE_MPLUSN, CL_TRUE);
        return clAmsPeSUAssignSIError(su, si, error);
    }

    AMS_CHECK_RC_ERROR ( clAmsEntityListFindEntityRef2(
                    &su->status.siList,
                    &si->config.entity,
                    0,
                    (ClAmsEntityRefT **)&siRef) );

    AMS_ENTITY_LOG (su, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("Confirming SI [%s] assigned to SU [%s] has HA State [%s]\n",
             si->config.entity.name.value,
             su->config.entity.name.value,
             CL_AMS_STRING_H_STATE(siRef->haState)));

    if ( (siRef->haState == CL_AMS_HA_STATE_ACTIVE) &&
         (si->status.numActiveAssignments == 1) )
    {
        CL_AMS_SET_O_STATE(si, CL_AMS_OPER_STATE_ENABLED);

        AMS_CHECK_RC_ERROR ( clAmsPeSIEvaluateWork(si) );

        AMS_CHECK_RC_ERROR ( clAmsPeSIUpdateDependents(si) );
    }

    /*
     * Unwind swap active assignment context.
     */
    clAmsPeEntityOpReplay(&su->config.entity, &su->status.entity, CL_AMS_ENTITY_OP_SWAP_ACTIVE_MPLUSN, CL_FALSE);
    
    /*
     * Replay pending active removes.
     */
    clAmsPeEntityOpReplay(&su->config.entity, &su->status.entity, CL_AMS_ENTITY_OP_ACTIVE_REMOVE_MPLUSN, CL_FALSE);


    if ( !clAmsInvocationsPendingForSU(su) )
    {
        if ( su->status.recovery == CL_AMS_RECOVERY_SU_RESTART )
        {
            AMS_CALL ( clAmsPeSUFaultCallback_Step2(su, error) );
        }
    }

    return CL_OK;

    exitfn:

    clAmsPeEntityOpReplay(&su->config.entity, &su->status.entity,
                          CL_AMS_ENTITY_OP_SWAP_ACTIVE_MPLUSN, CL_FALSE);

    clAmsPeEntityOpReplay(&su->config.entity, &su->status.entity,
                          CL_AMS_ENTITY_OP_ACTIVE_REMOVE_MPLUSN, CL_FALSE);
    
    return rc;
}

/*
 * clAmsPeSURemoveSI
 * -----------------
 * Given a SI to remove from a SU, this fn breaks down the SI into its constituent
 * CSIs and then removes them from the component to which they are assigned.
 */

ClRcT
clAmsPeSURemoveSI(
        CL_IN   ClAmsSUT *su,
        CL_IN   ClAmsSIT *si,
        CL_IN   ClUint32T switchoverMode)
{
    ClAmsEntityRefT *entityRef;
    ClAmsSUSIRefT *siRef;
    ClRcT rc;

    AMS_CHECK_SU ( su );
    AMS_CHECK_SI ( si );

    AMS_FUNC_ENTER ( ("SU [%s]\n",su->config.entity.name.value) );

    rc = clAmsEntityListFindEntityRef2(
                    &su->status.siList,
                    &si->config.entity,
                    0,
                    (ClAmsEntityRefT **)&siRef);

    if ( CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST )
    {
        AMS_ENTITY_LOG(su, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
            ("SU [%s] is not assigned SI [%s]. Ignoring [Remove SI] request..\n",
            su->config.entity.name.value,
            si->config.entity.name.value));

        return CL_OK;
    }

    if ( siRef->haState == CL_AMS_HA_STATE_NONE )
    {
        return CL_OK;
    }

    AMS_ENTITY_LOG(su, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
        ("Removing SI [%s] assigned to SU [%s]\n",
        si->config.entity.name.value,
        su->config.entity.name.value));

    /*
     * Walk the list of CSIs in the SI (this list is ordered by CSI rank)
     * and find the component to which a CSI has been assigned. Then send
     * a remove csi call to the component.
     */
 
    for ( entityRef = clAmsEntityListGetFirst(&si->config.csiList);
          entityRef != (ClAmsEntityRefT *) NULL;
          entityRef = clAmsEntityListGetNext(&si->config.csiList, entityRef) )
    {
        ClAmsCSIT *csi = (ClAmsCSIT *) entityRef->ptr;
        ClAmsCompT *comp;

        AMS_CHECK_CSI ( csi );
        
        /*
         * No need to skip dependencies on switchover mode of fast.
         */
        if(!(switchoverMode & CL_AMS_ENTITY_SWITCHOVER_FAST)
           &&
           csi->config.csiDependentsList.numEntities > 0 
           &&
           clAmsPeCheckCSIDependentsAssigned(su, csi))
            continue;

        rc = clAmsPeSUFindCompWithCSIAssignment(su, csi, &comp);

        if ( CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST )
        {
            continue;
        }

        AMS_CALL ( clAmsPeCompRemoveCSI(comp,
                                        csi,
                                        switchoverMode) );
    }

    return CL_OK;
}

/*
 * clAmsPeSURemoveSICallback
 * -------------------------
 * This fn is called when a SI is removed from a SU
 */

ClRcT
clAmsPeSURemoveSICallback(
        CL_IN   ClAmsSUT *su,
        CL_IN   ClAmsSIT *si,
        CL_IN   ClRcT error,
        CL_IN   ClUint32T switchoverMode)
{

    AMS_CHECK_SU ( su );
    AMS_CHECK_SI ( si );

    AMS_FUNC_ENTER ( ("SU [%s]\n",su->config.entity.name.value) );

    // WORKQUEUE
    
    /*
     * Replay any pending REMOVE operations on this SU.
     */
    clAmsPeEntityOpReplay(&su->config.entity, &su->status.entity, CL_AMS_ENTITY_OP_REMOVE_MPLUSN, CL_FALSE);

    /*
     * Replay pending REDUCE remove ops.
     */
    clAmsPeEntityOpReplay(&su->config.entity, &su->status.entity, CL_AMS_ENTITY_OP_REDUCE_REMOVE_MPLUSN, CL_FALSE);

    /*
     * Replay pending swaps.
     */
    clAmsPeEntityOpReplay(&su->config.entity, &su->status.entity, CL_AMS_ENTITY_OP_SWAP_REMOVE_MPLUSN, CL_FALSE);

    if ( !si->status.suList.numEntities )
    {
        AMS_CALL ( clAmsPeSIRemoveCallback(si, error, switchoverMode) );
    }

    return CL_OK;
}

/*
 * clAmsPeSUQuiesceSI
 * ------------------
 * This fn is called to quiesce a SI. It breaks down the SI into its CSIs and
 * quiesces them individually.
 */

ClRcT
clAmsPeSUQuiesceSI(
        CL_IN       ClAmsSUT *su,
        CL_IN       ClAmsSIT *si,
        CL_IN       ClUint32T switchoverMode)
{
    ClAmsEntityRefT *entityRef;
    ClAmsSUSIRefT *siRef;
    ClRcT rc;
    ClRcT (*quiesceFn)(ClAmsCompT *, ClAmsCSIT *, ClUint32T);
    //ClUint32T assignedSIs;

    AMS_CHECK_SU ( su );
    AMS_CHECK_SI ( si );

    AMS_FUNC_ENTER ( ("SU [%s]\n",su->config.entity.name.value) );

    rc = clAmsEntityListFindEntityRef2(
                    &su->status.siList,
                    &si->config.entity,
                    0,
                    (ClAmsEntityRefT **)&siRef);

    if ( CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST )
    {
        AMS_ENTITY_LOG(su, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
            ("SU [%s] is not assigned SI [%s]. Ignoring [Quiesce SI] request..\n",
            su->config.entity.name.value,
            si->config.entity.name.value));

        return CL_OK;
    }

    if ( (switchoverMode & CL_AMS_ENTITY_SWITCHOVER_GRACEFUL))
    {
        if ( siRef->haState != CL_AMS_HA_STATE_ACTIVE )
        {
            AMS_ENTITY_LOG(su, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
                ("SI [%s] assigned to SU [%s] is not Active and cannot be marked Quiescing\n",
                 si->config.entity.name.value,
                 su->config.entity.name.value));
            
            return CL_OK;
        }

        quiesceFn = clAmsPeCompQuiesceCSIGracefully;
    }
    else
    {
        if ( (siRef->haState != CL_AMS_HA_STATE_ACTIVE) &&
             (siRef->haState != CL_AMS_HA_STATE_QUIESCING) )
        {
            AMS_ENTITY_LOG(su, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
                ("SI [%s] assigned to SU [%s] is not Active or Quiescing and cannot be marked Quiesced\n",
                 si->config.entity.name.value,
                 su->config.entity.name.value));

            return CL_OK;
        }

        quiesceFn = clAmsPeCompQuiesceCSIImmediately;
    }

    //assignedSIs = su->status.numActiveSIs + su->status.numStandbySIs;

    AMS_ENTITY_LOG(su, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
        ("Quiescing SI [%s] assigned to SU [%s]\n",
        si->config.entity.name.value,
        su->config.entity.name.value));

    /*
     * Walk the list of CSIs in the SI (this list is ordered by CSI rank)
     * and find the component to which a CSI has been assigned. Then send
     * a quiesce csi immediately call to the component.
     */
 
    for ( entityRef = clAmsEntityListGetFirst(&si->config.csiList);
          entityRef != (ClAmsEntityRefT *) NULL;
          entityRef = clAmsEntityListGetNext(&si->config.csiList, entityRef) )
    {
        ClAmsCSIT *csi = (ClAmsCSIT *) entityRef->ptr;
        ClAmsCompT *comp;

        AMS_CHECK_CSI ( csi );

        if(csi->config.csiDependentsList.numEntities > 0
           &&
           clAmsPeCheckCSIDependentsAssigned(su, csi))
            continue;

        AMS_CALL ( clAmsPeSUFindCompWithCSIAssignment(su, csi, &comp) );
        AMS_CALL ( quiesceFn(comp, csi, switchoverMode) );
    }

    return CL_OK;
}

ClRcT
clAmsPeSUQuiesceSIGracefullyCallback(
        CL_IN       ClAmsSUT *su,
        CL_IN       ClAmsSIT *si,
        CL_IN       ClRcT error,
        CL_IN       ClUint32T switchoverMode)
{
    AMS_FUNC_ENTER ( ("SU [%s]\n", su->config.entity.name.value) );

    AMS_ENTITY_LOG(su, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
        ("Confirming SI [%s] assigned to SU [%s] is now [Quiesced] gracefully\n",
         si->config.entity.name.value,
         su->config.entity.name.value) ); 

    //AMS_CALL ( clAmsPeSURemoveSI(su, si, switchoverMode) );

    return CL_OK;
}

ClRcT
clAmsPeSUQuiesceSIImmediatelyCallback(
        CL_IN       ClAmsSUT *su,
        CL_IN       ClAmsSIT *si,
        CL_IN       ClRcT error,
        CL_IN       ClUint32T switchoverMode)
{
    AMS_FUNC_ENTER ( ("SU [%s]\n", su->config.entity.name.value) );

    AMS_ENTITY_LOG(su, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
        ("Confirming SI [%s] assigned to SU [%s] is now [Quiesced]\n",
         si->config.entity.name.value,
         su->config.entity.name.value) ); 

    //AMS_CALL ( clAmsPeSURemoveSI(su, si, switchoverMode) );

    return CL_OK;
}

/*
 * clAmsPeSUFindCompForCSIAssignment
 * ---------------------------------
 * Given a CSI and a list of components in a SU this fn finds a component that
 * matches the CSI type and has capacity to host the CSI. It is possible to 
 * have multiple components in a SU that support the same CSI type, in which
 * case, this fn returns the first one found that has spare capacity.
 *
 * Note: The capacity of a component to accept CSI assignments is defined by
 * the component capability. The redundancy model for a service group can limit
 * the type of components that can be defined to be a part of the SUs in the
 * SG. However, we do not check in this fn if there is a mismatch. That is
 * left to configuration time validation functions.
 */

ClRcT
clAmsPeSUFindCompForCSIAssignment(
        CL_IN   ClAmsSUT        *su,
        CL_IN   ClAmsCSIT       *csi, 
        CL_IN   ClAmsHAStateT   haState,
        CL_OUT  ClAmsCompT      **foundComp)
{
    ClBoolT foundType = CL_FALSE;
    ClAmsEntityRefT *entityRef;
    ClAmsSGT *sg = NULL;
    ClUint32T i;

    AMS_CHECK_SU ( su );
    AMS_CHECKPTR ( !csi || !foundComp );

    AMS_FUNC_ENTER ( ("SU [%s]\n", su->config.entity.name.value) );

    sg = (ClAmsSGT*)su->config.parentSG.ptr;
    
    AMS_CHECK_SG( sg );

    *foundComp = NULL;
    
    for ( entityRef = clAmsEntityListGetFirst(&su->config.compList);
          entityRef != (ClAmsEntityRefT *) NULL;
          entityRef = clAmsEntityListGetNext(&su->config.compList, entityRef) )
    {
        ClAmsCompT *comp = (ClAmsCompT *) entityRef->ptr;

        for(i = 0; i < comp->config.numSupportedCSITypes; ++i)
        {
            if ( !memcmp(comp->config.pSupportedCSITypes[i].value,
                         csi->config.type.value, 
                         csi->config.type.length) )
                goto found;
        }

        continue;

        found:

        foundType = CL_TRUE;

        /*
         * Found matching component, now check if it has the capacity. It is
         * assumed that the max capacities are set appropriately for the
         * capability of the component.
         */

        switch ( comp->config.capabilityModel )
        {
        case CL_AMS_COMP_CAP_ONE_ACTIVE_OR_ONE_STANDBY:
        case CL_AMS_COMP_CAP_X_ACTIVE_OR_Y_STANDBY:
        case CL_AMS_COMP_CAP_ONE_ACTIVE_OR_X_STANDBY:
            {
                /*
                 * The component should have capacity for the type of
                 * CSI assignment desired, but no assignments with a
                 * different HA state.
                 */

                if ( haState == CL_AMS_HA_STATE_ACTIVE )
                {
                    if ( sg->config.redundancyModel != CL_AMS_SG_REDUNDANCY_MODEL_CUSTOM
                         &&
                         comp->status.numStandbyCSIs )
                        continue;
                    else if ( comp->status.numActiveCSIs < 
                              comp->config.numMaxActiveCSIs )
                    {
                        *foundComp = comp;
                        goto FOUND_COMPONENT;
                    }
                }

                if ( haState == CL_AMS_HA_STATE_STANDBY )
                {
                    if ( sg->config.redundancyModel != CL_AMS_SG_REDUNDANCY_MODEL_CUSTOM
                         &&
                         comp->status.numActiveCSIs )
                        continue;
                    else if ( comp->status.numStandbyCSIs < 
                              comp->config.numMaxStandbyCSIs )
                    {
                        *foundComp = comp;
                        goto FOUND_COMPONENT;
                    }
                }

                break;
            }

        case CL_AMS_COMP_CAP_X_ACTIVE:
        case CL_AMS_COMP_CAP_ONE_ACTIVE:
        case CL_AMS_COMP_CAP_NON_PREINSTANTIABLE:
            {
                /*
                 * The component should have capacity for an active
                 * assignment. A non preinstantiable is treated as 
                 * a component that can take one active assignment.
                 */

                if ( haState == CL_AMS_HA_STATE_ACTIVE )
                {
                    if ( comp->status.numActiveCSIs < 
                         comp->config.numMaxActiveCSIs )
                    {
                        *foundComp = comp;
                        goto FOUND_COMPONENT;
                    }
                }

                break;
            }

        case CL_AMS_COMP_CAP_X_ACTIVE_AND_Y_STANDBY:
            {
                /*
                 * The component should have capacity for the type
                 * of CSI assignment desired. It is acceptable for
                 * the component to have assignments with another
                 * HA state.
                 */

                if ( haState == CL_AMS_HA_STATE_ACTIVE )
                {
                    if ( comp->status.numActiveCSIs < 
                         comp->config.numMaxActiveCSIs )
                    {
                        *foundComp = comp;
                        goto FOUND_COMPONENT;
                    }
                }

                if ( haState == CL_AMS_HA_STATE_STANDBY )
                {
                    if ( comp->status.numStandbyCSIs < 
                         comp->config.numMaxStandbyCSIs )
                    {
                        *foundComp = comp;
                        goto FOUND_COMPONENT;
                    }
                }

                break;
            }
 
        default:
            {
                return CL_AMS_RC(CL_ERR_NOT_IMPLEMENTED);
            }
        }

    }

    /*
     * Since we are here, it means that either a component was not found
     * that matched the type or one was found but it did not have the
     * capacity to support a new CSI.
     */

    if ( foundType )
        return CL_AMS_RC(CL_AMS_ERR_CSI_NOT_ASSIGNABLE);
    else
        return CL_AMS_RC(CL_ERR_DOESNT_EXIST);

    FOUND_COMPONENT:

    AMS_ENTITY_LOG(su, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
                   ("SU [%s]: Found Comp [%s] for CSI [%s] assignment\n",
                    su->config.entity.name.value,
                    (*foundComp)->config.entity.name.value,
                    csi->config.entity.name.value));

    return CL_OK;
}


/*
 * clAmsPeSUFindCompWithCSIAssignment
 * ----------------------------------
 * Given a CSI and a list of components in a SU, this fn finds the component to
 * to which the CSI is assigned.
 */

ClRcT
clAmsPeSUFindCompAndCSIWithCSIAssignment(
        CL_IN   ClAmsSUT        *su,
        CL_IN   ClAmsCSIT       *csi, 
        CL_OUT  ClAmsCompT      **foundComp,
        CL_OUT  ClAmsEntityRefT **foundCSI)
{
    ClAmsEntityRefT *entityRef;
    ClAmsEntityRefT *csiRef;
    ClCntKeyHandleT entityKeyHandle;
    ClAmsCompCSIRefT *foundCSIRef;
    ClUint32T i;
    ClRcT rc = CL_OK;

    AMS_CHECK_SU ( su );
    AMS_CHECK_CSI ( csi );
    AMS_CHECKPTR ( !foundComp );

    AMS_FUNC_ENTER ( ("SU [%s]: CSI [%s]\n",
                      su->config.entity.name.value,
                      csi->config.entity.name.value) );

    *foundComp = NULL;
    
    AMS_CALL ( clAmsEntityRefGetKey(
                                    &csi->config.entity,
                                    csi->config.rank,
                                    &entityKeyHandle,
                                    CL_FALSE) ); /* isRankedList : comp->status.csiList */

    if  ( ! (csiRef = clHeapAllocate(sizeof(ClAmsEntityRefT))) )
        return CL_AMS_RC(CL_ERR_NO_MEMORY);

    memcpy(&csiRef->entity,
           &csi->config.entity,
           sizeof(ClAmsEntityT) ); 

    for ( entityRef = clAmsEntityListGetFirst(&su->config.compList);
          entityRef != (ClAmsEntityRefT *) NULL;
          entityRef = clAmsEntityListGetNext(&su->config.compList, entityRef) )
    {
        ClAmsCompT *comp = (ClAmsCompT *) entityRef->ptr;

        for(i = 0; i <  comp->config.numSupportedCSITypes; ++i)
        {
            if ( !memcmp(
                         comp->config.pSupportedCSITypes[i].value,
                         csi->config.type.value, 
                         csi->config.type.length) )
                goto found;
        }

        continue;

        /*
         * Found matching component, now check it.
         */
        found:
        rc = clAmsEntityListFindEntityRef(
                                          &comp->status.csiList,
                                          csiRef,
                                          entityKeyHandle,
                                          (ClAmsEntityRefT **)&foundCSIRef); 

        if ( rc == CL_OK )
        {
            *foundComp = comp;
            if(foundCSI) *foundCSI = (ClAmsEntityRefT*)foundCSIRef;
            goto out;
        }
    }

    rc = CL_AMS_RC(CL_ERR_NOT_EXIST);

    out:

    clHeapFree(csiRef);

    return rc;
}

ClRcT
clAmsPeSUFindCompWithCSIAssignment(
        CL_IN   ClAmsSUT        *su,
        CL_IN   ClAmsCSIT       *csi, 
        CL_OUT  ClAmsCompT      **foundComp)
{
    return clAmsPeSUFindCompAndCSIWithCSIAssignment(su, csi, foundComp, NULL);
}

/*-----------------------------------------------------------------------------
 * SU Utility Functions
 *---------------------------------------------------------------------------*/

/*
 * clAmsPeSUComputeAdminState
 * --------------------------
 * Compute the effective admin state of a SU based on its admin state
 * and the admin states of its node, SG and application group.
 *
 * Future: Add application and cluster.
 */

static void clAmsPeSUComputeAdminStateExtended(
        CL_IN       ClAmsSUT *su,
        CL_OUT      ClAmsAdminStateT *adminState,
        CL_IN       ClAmsAdminStateT suAdminState)
{
    ClAmsNodeT *node;
    ClAmsSGT *sg;
    ClAmsAdminStateT nodeAdminState, sgAdminState;

    CL_ASSERT(adminState);
    CL_ASSERT(AMS_ENTITY_OK(su,CL_AMS_ENTITY_TYPE_SU));
    
    AMS_FUNC_ENTER ( ("SU [%s]\n",su->config.entity.name.value) );

    /* Instead of returning an error, return the effective admin
       state given the error
    */

    /* If the SG or node does not exist, the effective admin state is "off" or LOCKED_I */
    sg = (ClAmsSGT *)su->config.parentSG.ptr;
    if (!AMS_ENTITY_OK(sg,CL_AMS_ENTITY_TYPE_SG))
    {
        *adminState = CL_AMS_ADMIN_STATE_LOCKED_I;
        return;
    }
    node = (ClAmsNodeT *)su->config.parentNode.ptr;
    if (!AMS_ENTITY_OK(node,CL_AMS_ENTITY_TYPE_NODE))
    {
      *adminState = CL_AMS_ADMIN_STATE_LOCKED_I;
      return;          
    }

    /* If the SG or node's state can't be computed, its safe for us to be "off" */    
    if (clAmsPeNodeComputeAdminState(node, &nodeAdminState) != CL_OK)
    {
        *adminState = CL_AMS_ADMIN_STATE_LOCKED_I;
        return;
    }

    sgAdminState = clAmsPeSGComputeAdminState(sg);

    if ( (suAdminState == CL_AMS_ADMIN_STATE_UNLOCKED) &&
         (nodeAdminState == CL_AMS_ADMIN_STATE_UNLOCKED) &&
         (sgAdminState == CL_AMS_ADMIN_STATE_UNLOCKED) )
    {
        *adminState = CL_AMS_ADMIN_STATE_UNLOCKED;
        return;
    }

    if ( (suAdminState == CL_AMS_ADMIN_STATE_LOCKED_I) ||
         (nodeAdminState        == CL_AMS_ADMIN_STATE_LOCKED_I) ||
         (sgAdminState          == CL_AMS_ADMIN_STATE_LOCKED_I) )
    {
        *adminState = CL_AMS_ADMIN_STATE_LOCKED_I;
        return;
    }

    if ( (suAdminState == CL_AMS_ADMIN_STATE_LOCKED_A) ||
         (nodeAdminState        == CL_AMS_ADMIN_STATE_LOCKED_A) ||
         (sgAdminState          == CL_AMS_ADMIN_STATE_LOCKED_A) )
    {
        *adminState = CL_AMS_ADMIN_STATE_LOCKED_A;
        return;
    }

    *adminState = CL_AMS_ADMIN_STATE_SHUTTINGDOWN;

    return;
}

void
clAmsPeSUComputeAdminState(
        CL_IN       ClAmsSUT *su,
        CL_OUT      ClAmsAdminStateT *adminState)
{
    CL_ASSERT(AMS_ENTITY_OK(su,CL_AMS_ENTITY_TYPE_SU));
    CL_ASSERT(adminState);

    clAmsPeSUComputeAdminStateExtended(su, adminState, su->config.adminState);
}

void clAmsPeCompComputeAdminState(
                                   CL_IN ClAmsCompT *comp,
                                   CL_OUT ClAmsAdminStateT *adminState)
{
    ClAmsSUT *su;
    CL_ASSERT(AMS_ENTITY_OK(comp,CL_AMS_ENTITY_TYPE_COMP));
    CL_ASSERT(adminState);

    su = (ClAmsSUT*)comp->config.parentSU.ptr;
    if (!AMS_ENTITY_OK(su,CL_AMS_ENTITY_TYPE_SU)) /* If the component has no SU, it can never be instantiated */
    {
        *adminState = CL_AMS_ADMIN_STATE_LOCKED_I;
        return;
    }
    
    clAmsPeSUComputeAdminStateExtended(su, adminState, su->config.adminState);
}

/*
 * clAmsPeSUComputeReadinessState
 * ------------------------------
 * Compute the readiness state of a SU based on other states.
 * Future: add application and cluster entities
 */

void
clAmsPeSUComputeReadinessState(
        CL_IN       ClAmsSUT *su,
        CL_OUT      ClAmsReadinessStateT *suState)
{
    ClAmsSGT *sg;
    ClAmsNodeT *node;
    ClAmsAdminStateT adminState = CL_AMS_ADMIN_STATE_LOCKED_I;

    CL_ASSERT(suState);
    CL_ASSERT(AMS_ENTITY_OK(su,CL_AMS_ENTITY_TYPE_SU));

    AMS_FUNC_ENTER ( ("SU [%s]\n",su->config.entity.name.value) );
    
    /* If the SG or the Node does not exist, we must go out of service */
    sg = (ClAmsSGT *) su->config.parentSG.ptr;
    if (!AMS_ENTITY_OK(sg,CL_AMS_ENTITY_TYPE_SG))
    {
      *suState = CL_AMS_READINESS_STATE_OUTOFSERVICE;
      clLog(CL_LOG_WARNING,"AMF","POL","SU [%.*s] has no SG",su->config.entity.name.length,su->config.entity.name.value);
      return;
    }
    node = (ClAmsNodeT *) su->config.parentNode.ptr;
    if (!AMS_ENTITY_OK(node,CL_AMS_ENTITY_TYPE_NODE))
    {
      clLog(CL_LOG_WARNING,"AMF","POL","SU [%.*s] has no Node",su->config.entity.name.length,su->config.entity.name.value);
      *suState = CL_AMS_READINESS_STATE_OUTOFSERVICE;
      return;          
    }
    
    clAmsPeSUComputeAdminState(su, &adminState);

    if ( (su->status.operState    == CL_AMS_OPER_STATE_ENABLED)   &&
         (node->status.operState  == CL_AMS_OPER_STATE_ENABLED)   &&
         (adminState              == CL_AMS_ADMIN_STATE_UNLOCKED) )
    {
        if ( su->config.isPreinstantiable )
        {
            if ( (su->status.presenceState == CL_AMS_PRESENCE_STATE_INSTANTIATED) ||
                 (su->status.presenceState == CL_AMS_PRESENCE_STATE_RESTARTING) )
            {
                *suState = CL_AMS_READINESS_STATE_INSERVICE;
                return;
            }
        }
        else
        {
                *suState = CL_AMS_READINESS_STATE_INSERVICE;
                return;
        }
    }

    if ( (su->status.operState   == CL_AMS_OPER_STATE_ENABLED) &&
         (node->status.operState == CL_AMS_OPER_STATE_ENABLED) &&
         (adminState             == CL_AMS_ADMIN_STATE_SHUTTINGDOWN) )
    {
        *suState = CL_AMS_READINESS_STATE_STOPPING;
        return;
    }

    *suState = CL_AMS_READINESS_STATE_OUTOFSERVICE;

    return;
}

/*
 * clAmsPeSUUpdateReadinessState
 * -----------------------------
 * Update the readiness state of a SU and its constituent components.
 */

ClRcT
clAmsPeSUUpdateReadinessState(
        CL_IN       ClAmsSUT *su)
{
    ClAmsReadinessStateT rstate;
    ClAmsEntityRefT *entityRef;

    CL_ASSERT(AMS_ENTITY_OK(su,CL_AMS_ENTITY_TYPE_SU));
    
    /*
     * Update state of SU
     */

    clAmsPeSUComputeReadinessState(su, &rstate);
    CL_AMS_SET_R_STATE(su, rstate);

    /*
     * Update state of its constituent components
     */

    for ( entityRef = clAmsEntityListGetFirst(&su->config.compList);
          entityRef != (ClAmsEntityRefT *) NULL;
          entityRef = clAmsEntityListGetNext(&su->config.compList, entityRef) )
    {
        ClAmsCompT *comp = (ClAmsCompT *) entityRef->ptr;

        AMS_CHECK_COMP ( comp );

        AMS_CALL ( clAmsPeCompUpdateReadinessState(comp) );
    }

    return CL_OK;
}

ClRcT
clAmsPeSUIsInstantiable(
        CL_IN   ClAmsSUT    *su)
{
    ClAmsAdminStateT adminState;
    ClAmsNodeT *node;

    AMS_CHECK_SU ( su );
    AMS_CHECK_NODE ( node = (ClAmsNodeT *) su->config.parentNode.ptr );
    
    AMS_FUNC_ENTER ( ("SU [%s]\n",su->config.entity.name.value) );

    clAmsPeSUComputeAdminState(su, &adminState);

    if ( ( (adminState == CL_AMS_ADMIN_STATE_LOCKED_A) ||
           (adminState == CL_AMS_ADMIN_STATE_UNLOCKED) ) &&
         ( su->status.operState == CL_AMS_OPER_STATE_ENABLED ) &&
         ( clAmsPeNodeIsInstantiable(node) == CL_OK ) )
    {
        return CL_OK;
    }

    return CL_AMS_RC(CL_AMS_ERR_ENTITY_NOT_ENABLED);
}

// Future: This fn should simply use su readiness state == in service
// readiness state computation uses node oper state which is not set
// today

ClRcT
clAmsPeSUIsAssignable(
        CL_IN   ClAmsSUT *su)
{
    ClAmsAdminStateT adminState;
    ClAmsNodeT *node;

    AMS_CHECK_SU ( su );

    AMS_CHECK_NODE (node = (ClAmsNodeT*)su->config.parentNode.ptr);

    AMS_FUNC_ENTER ( ("SU [%s]\n",su->config.entity.name.value) );

    clAmsPeSUComputeAdminState(su, &adminState);

    if ( (clAmsPeSUIsInstantiable(su) == CL_OK) &&
        (adminState == CL_AMS_ADMIN_STATE_UNLOCKED) )
    {
        return CL_OK;
    }

    return CL_AMS_RC(CL_AMS_ERR_ENTITY_NOT_ENABLED);
}

ClRcT
clAmsPeSUMarkInstantiable(
                          CL_IN   ClAmsSUT    *su
                          )
{
    ClAmsSGT *sg;
    ClAmsNodeT *node;
    ClAmsEntityRefT *suRef;
    
    AMS_CHECK_SU ( su );
    sg = (ClAmsSGT *) su->config.parentSG.ptr;
    if(!sg) return CL_OK;

    AMS_CHECK_SG( sg );
    AMS_CHECK_NODE ( node = (ClAmsNodeT*)su->config.parentNode.ptr);

    AMS_FUNC_ENTER ( ("SU [%s]\n",su->config.entity.name.value) );

    if( clAmsPeSUIsInstantiable(su) != CL_OK)
    {
        AMS_ENTITY_LOG (su, CL_AMS_MGMT_SUB_AREA_STATE_CHANGE,CL_DEBUG_TRACE, 
                        ("SU [%s] : SG [%s] List Add (SU is not instantiable)\n",
                         su->config.entity.name.value,
                         sg->config.entity.name.value));
        return CL_OK;
    }
    // Future: Note, SGAdd will check if the su is already on the list
    // and return error. This check can be removed and instead the db
    // code changed to not return error.

    if ( clAmsEntityListFindEntityRef2(&sg->status.instantiableSUList,
                                       &su->config.entity,
                                       0,
                                       (ClAmsEntityRefT **)&suRef) != CL_OK )
    {
        AMS_CALL ( clAmsSGAddSURefToSUList(&sg->status.instantiableSUList, su) );

        AMS_ENTITY_LOG (su, CL_AMS_MGMT_SUB_AREA_STATE_CHANGE,CL_DEBUG_TRACE, 
                        ("SU [%s] : SG [%s] List Add (none -> instantiable)\n",
                         su->config.entity.name.value,
                         sg->config.entity.name.value));
    }

    return CL_OK;
}

ClRcT
clAmsPeSUMarkUninstantiable(
        CL_IN   ClAmsSUT    *su)
{
    ClAmsSGT *sg;
    ClRcT rc = CL_OK;
    
    AMS_CHECK_SU ( su );
    AMS_CHECK_SG ( sg = (ClAmsSGT *) su->config.parentSG.ptr );

    AMS_FUNC_ENTER ( ("SU [%s]\n",su->config.entity.name.value) );

    if ( su->status.presenceState == CL_AMS_PRESENCE_STATE_NONE )
    {
        return CL_OK;
    }

    // Future: This check should be moved to SGDelete

    rc = clAmsSGDeleteSURefFromSUList(&sg->status.instantiableSUList, su);

    if ( (rc != CL_OK) && (CL_GET_ERROR_CODE(rc) != CL_ERR_NOT_EXIST) )
        return rc;

    AMS_ENTITY_LOG (su, CL_AMS_MGMT_SUB_AREA_STATE_CHANGE,CL_DEBUG_TRACE, 
        ("SU [%s] : SG [%s] List Remove (instantiable -> none) \n",
         su->config.entity.name.value,
         sg->config.entity.name.value));

    return CL_OK;
}

/*
 * clAmsPeSUMarkInstantiated
 * -------------------------
 * This fn marks an SU as instantiated if it is in the instantiated,
 * instantiating or restarting states. This is a transient state and
 * the SU is moved to the inservice state as soon as the instantiation
 * succeeds.
 */

ClRcT
clAmsPeSUMarkInstantiated(
        CL_IN   ClAmsSUT    *su)
{
    ClAmsSGT *sg;
    ClRcT rc = CL_OK;

    AMS_CHECK_SU ( su );
    AMS_CHECK_SG ( sg = (ClAmsSGT *) su->config.parentSG.ptr );

    AMS_FUNC_ENTER ( ("SU [%s]\n",su->config.entity.name.value) );

    rc = clAmsSGDeleteSURefFromSUList(&sg->status.instantiableSUList, su);

    if(rc != CL_OK && CL_GET_ERROR_CODE(rc) != CL_ERR_NOT_EXIST)
        return rc;

    if(CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST)
        return CL_OK;

    AMS_CALL ( clAmsSGAddSURefToSUList(&sg->status.instantiatedSUList, su) );

    AMS_ENTITY_LOG (su, CL_AMS_MGMT_SUB_AREA_STATE_CHANGE,CL_DEBUG_TRACE, 
        ("SU [%s] : SG [%s] List Change (instantiable -> instantiated) \n",
         su->config.entity.name.value,
         sg->config.entity.name.value));

    return CL_OK;
}

ClRcT
clAmsPeSUMarkUninstantiated(
        CL_IN   ClAmsSUT    *su)
{
    ClAmsSGT *sg;
    ClRcT rc = CL_OK;
    
    AMS_CHECK_SU ( su );
    AMS_CHECK_SG ( sg = (ClAmsSGT *) su->config.parentSG.ptr );

    AMS_FUNC_ENTER ( ("SU [%s]\n",su->config.entity.name.value) );

    rc = clAmsSGDeleteSURefFromSUList(&sg->status.instantiatedSUList, su);

    if ( (rc != CL_OK) && (CL_GET_ERROR_CODE(rc) != CL_ERR_NOT_EXIST) )
        return rc;

    if ( CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST )
        return CL_OK;

    AMS_CALL ( clAmsSGAddSURefToSUList(&sg->status.instantiableSUList, su) );

    AMS_ENTITY_LOG (su, CL_AMS_MGMT_SUB_AREA_STATE_CHANGE,CL_DEBUG_TRACE, 
        ("SU [%s] : SG [%s] List Change (instantiated -> instantiable) \n",
         su->config.entity.name.value,
         sg->config.entity.name.value));

    return CL_OK;
}

ClRcT
clAmsPeSUMarkReady(
        CL_IN   ClAmsSUT    *su)
{
    ClAmsSGT *sg;
    ClRcT rc = CL_OK;

    AMS_CHECK_SU ( su );
    AMS_CHECK_SG ( sg = (ClAmsSGT *) su->config.parentSG.ptr );

    AMS_FUNC_ENTER ( ("SU [%s]\n",su->config.entity.name.value) );

    /*
    ClAmsReadinessStateT rstate;
    AMS_CALL ( clAmsPeSUComputeReadinessState(su, &rstate) );
    CL_AMS_SET_R_STATE(su, rstate);
    
    if ( (su->status.readinessState != CL_AMS_READINESS_STATE_INSERVICE) &&
         (su->status.presenceState  != CL_AMS_PRESENCE_STATE_INSTANTIATED) )
    {
        return CL_OK;
    }
    */

    rc = clAmsSGDeleteSURefFromSUList(&sg->status.instantiatedSUList, su);

    if(rc != CL_OK && CL_GET_ERROR_CODE(rc) != CL_ERR_NOT_EXIST)
        return rc;

    if(CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST)
        return CL_OK;

    AMS_CALL ( clAmsSGAddSURefToSUList(&sg->status.inserviceSpareSUList, su) );

    AMS_ENTITY_LOG (su, CL_AMS_MGMT_SUB_AREA_STATE_CHANGE,CL_DEBUG_TRACE, 
        ("SU [%s] : SG [%s] List Change (instantiated -> inserviceSpare) \n",
         su->config.entity.name.value,
         sg->config.entity.name.value));

    return CL_OK;
}

ClRcT
clAmsPeSUMarkNotReady(
        CL_IN   ClAmsSUT    *su)
{
    ClAmsSGT *sg;
    ClRcT rc = CL_OK;

    AMS_CHECK_SU ( su );

    AMS_FUNC_ENTER ( ("SU [%s]\n",su->config.entity.name.value) );

    AMS_CHECK_SG ( sg = (ClAmsSGT *) su->config.parentSG.ptr );

    rc = clAmsSGDeleteSURefFromSUList(&sg->status.inserviceSpareSUList, su);

    if(rc != CL_OK && CL_GET_ERROR_CODE(rc) != CL_ERR_NOT_EXIST)
        return rc;

    if(CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST)
        return CL_OK;

    AMS_CALL ( clAmsSGAddSURefToSUList(&sg->status.instantiatedSUList, su) );

    AMS_ENTITY_LOG (su, CL_AMS_MGMT_SUB_AREA_STATE_CHANGE,CL_DEBUG_TRACE, 
        ("SU [%s] : SG [%s] List Change (inserviceSpare -> instantiated) \n",
         su->config.entity.name.value,
         sg->config.entity.name.value));

    return CL_OK;
}

ClRcT
clAmsPeSUMarkAssigned(
        CL_IN   ClAmsSUT    *su)
{
    ClAmsSGT *sg;
    ClAmsNodeT *node;
    ClRcT rc = CL_OK;

    AMS_CHECK_SU ( su );
    AMS_CHECK_SG ( sg = (ClAmsSGT *) su->config.parentSG.ptr );
    AMS_CHECK_NODE ( node = (ClAmsNodeT *) su->config.parentNode.ptr );

    AMS_FUNC_ENTER ( ("SU [%s]\n",su->config.entity.name.value) );

    rc = clAmsSGDeleteSURefFromSUList(&sg->status.inserviceSpareSUList, su);

    if(rc != CL_OK && CL_GET_ERROR_CODE(rc) != CL_ERR_NOT_EXIST)
        return rc;

    if(CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST)
        return CL_OK;

    AMS_CALL ( clAmsSGAddSURefToSUList(&sg->status.assignedSUList, su) );

    node->status.numAssignedSUs ++;

    AMS_ENTITY_LOG (su, CL_AMS_MGMT_SUB_AREA_STATE_CHANGE,CL_DEBUG_TRACE, 
        ("SU [%s] : SG [%s] List Change (inserviceSpare -> assigned) \n",
         su->config.entity.name.value,
         sg->config.entity.name.value));

    return CL_OK;
}


ClRcT
clAmsPeSUMarkUnassigned(
        CL_IN   ClAmsSUT    *su)
{
    ClAmsSGT *sg;
    ClAmsNodeT *node;
    ClRcT rc = CL_OK;
    
    AMS_CHECK_SU ( su );
    AMS_CHECK_SG ( sg = (ClAmsSGT *) su->config.parentSG.ptr );
    AMS_CHECK_NODE ( node = (ClAmsNodeT *) su->config.parentNode.ptr );

    AMS_FUNC_ENTER ( ("SU [%s]\n",su->config.entity.name.value) );

    rc = clAmsSGDeleteSURefFromSUList(&sg->status.assignedSUList, su);

    if ( (rc != CL_OK) && (CL_GET_ERROR_CODE(rc) != CL_ERR_NOT_EXIST) )
        return rc;

    if ( CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST )
        return CL_OK;

    AMS_CALL ( clAmsSGAddSURefToSUList(&sg->status.inserviceSpareSUList, su) );

    node->status.numAssignedSUs --;

    AMS_ENTITY_LOG (su, CL_AMS_MGMT_SUB_AREA_STATE_CHANGE,CL_DEBUG_TRACE, 
        ("SU [%s] : SG [%s] List Change (assigned -> inserviceSpare)\n",
         su->config.entity.name.value,
         sg->config.entity.name.value));

    return CL_OK;
}

ClRcT
clAmsPeSUMarkTerminated(
        CL_IN   ClAmsSUT    *su)
{
    ClAmsSGT *sg;
    ClAmsReadinessStateT rstate;
    ClRcT rc;
    
    AMS_CHECK_SU ( su );
    AMS_CHECK_SG ( sg = (ClAmsSGT *) su->config.parentSG.ptr );

    AMS_FUNC_ENTER ( ("SU [%s]\n",su->config.entity.name.value) );

    clAmsPeSUComputeReadinessState(su, &rstate);
    CL_AMS_SET_R_STATE(su, rstate);

    rc = clAmsSGDeleteSURefFromSUList(&sg->status.inserviceSpareSUList, su);

    if ( (rc != CL_OK) && (CL_GET_ERROR_CODE(rc) != CL_ERR_NOT_EXIST) )
        return rc;

    if ( CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST )
        return CL_OK;

    AMS_CALL ( clAmsSGAddSURefToSUList(&sg->status.instantiableSUList, su) );

    AMS_ENTITY_LOG (su, CL_AMS_MGMT_SUB_AREA_STATE_CHANGE,CL_DEBUG_TRACE, 
        ("SU [%s] : SG [%s] List Change (inserviceSpare -> instantiable)\n",
         su->config.entity.name.value,
         sg->config.entity.name.value));

    return CL_OK;
}

//not used for now 
ClRcT
clAmsPeSUMarkRestarting(
        CL_IN   ClAmsSUT    *su)
{
    ClAmsSGT *sg;
    ClRcT rc = CL_OK;

    AMS_CHECK_SU ( su );
    AMS_CHECK_SG ( sg = (ClAmsSGT *) su->config.parentSG.ptr );

    AMS_FUNC_ENTER ( ("SU [%s]\n",su->config.entity.name.value) );

    AMS_ENTITY_LOG (su, CL_AMS_MGMT_SUB_AREA_STATE_CHANGE,CL_DEBUG_TRACE, 
        ("SU [%s] : SG [%s] List Change (assigned -> instantiated)\n",
         su->config.entity.name.value,
         sg->config.entity.name.value));

    rc = clAmsSGDeleteSURefFromSUList(&sg->status.assignedSUList, su);
    
    if(rc != CL_OK && CL_GET_ERROR_CODE(rc) != CL_ERR_NOT_EXIST)
        return rc;

    if(CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST)
        return CL_OK;

    AMS_CALL ( clAmsSGAddSURefToSUList(&sg->status.instantiatedSUList, su) );

    return CL_OK;
}

/*
 * A fault can be raised on an SU that is in any state. Typically the fault is
 * meaningful if the SU is inserviceSpare or assigned. In any other state, the
 * fault has no material impact on availability.
 *
 * XXX Need to consider the above in marking a SU as faulty.
 */

//deprecated
ClRcT
clAmsPeSUMarkFaulty(
        CL_IN   ClAmsSUT    *su)
{
    ClAmsSGT *sg;
    ClRcT rc = CL_OK;

    AMS_CHECK_SU ( su );
    AMS_CHECK_SG ( sg = (ClAmsSGT *) su->config.parentSG.ptr );

    AMS_FUNC_ENTER ( ("SU [%s]\n",su->config.entity.name.value) );

    /*
     * Check if this is an assigned SU or an instantiated SU
     */

    if ( su->status.numActiveSIs || su->status.numStandbySIs )
    {
        rc = clAmsSGDeleteSURefFromSUList(&sg->status.assignedSUList, su);
        if(rc != CL_OK && CL_GET_ERROR_CODE(rc) != CL_ERR_NOT_EXIST)
            return rc;
        if(CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST)
            return CL_OK;
    }

    AMS_CALL ( clAmsSGAddSURefToSUList(&sg->status.faultySUList, su) );

    AMS_ENTITY_LOG (su, CL_AMS_MGMT_SUB_AREA_STATE_CHANGE,CL_DEBUG_TRACE, 
        ("SU [%s] : SG [%s] List Change ( -> faulty)\n",
         su->config.entity.name.value,
         sg->config.entity.name.value));

    return CL_OK;
}

//deprecated
ClRcT
clAmsPeSUMarkRepaired(
        CL_IN   ClAmsSUT    *su)
{
    ClAmsSGT *sg;
    ClRcT rc = CL_OK;

    AMS_CHECK_SU ( su );
    AMS_CHECK_SG ( sg = (ClAmsSGT *) su->config.parentSG.ptr );

    AMS_FUNC_ENTER ( ("SU [%s]\n",su->config.entity.name.value) );

    rc = clAmsSGDeleteSURefFromSUList(&sg->status.faultySUList, su);
    if(rc != CL_OK && CL_GET_ERROR_CODE(rc) != CL_ERR_NOT_EXIST)
        return rc;
    if(CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST)
        return CL_OK;
    AMS_CALL ( clAmsSGAddSURefToSUList(&sg->status.instantiableSUList, su) );

    AMS_ENTITY_LOG (su, CL_AMS_MGMT_SUB_AREA_STATE_CHANGE,CL_DEBUG_TRACE, 
        ("SU [%s] : SG [%s] List Change (faulty -> instantiable)\n",
         su->config.entity.name.value,
         sg->config.entity.name.value));

    return CL_OK;
}

/*
 * clAmsPeSUReset
 * --------------
 * Reset the status of a SU and its constituent components.
 */

ClRcT
clAmsPeSUReset(
        CL_IN ClAmsSUT *su)
{
    ClAmsEntityRefT *entityRef;

    AMS_CHECK_SU ( su );

    AMS_FUNC_ENTER ( ("SU [%s]\n",su->config.entity.name.value) );

    clAmsEntityReset((ClAmsEntityT *) su);

    for ( entityRef = clAmsEntityListGetFirst(&su->config.compList);
          entityRef != (ClAmsEntityRefT *) NULL;
          entityRef = clAmsEntityListGetNext(&su->config.compList,entityRef) )
    {
        ClAmsCompT *comp = (ClAmsCompT *) entityRef->ptr;

        AMS_CHECK_COMP ( comp );

        clAmsPeCompReset(comp);
    }

    return CL_OK;
}


static ClRcT clAmsPeRemoteProxiedSUsWalkCallback(ClAmsEntityT *entity, ClPtrT userArg)
{
    ClAmsNodeT *remoteNode = (ClAmsNodeT*)entity;
    ClAmsRemoteProxiedSUsWalkArgT *arg = userArg;
    ClListHeadT *proxiedSUList = arg->proxiedSUList;
    ClAmsNodeT *node = arg->node;
    ClAmsEntityRefT *suRef = NULL;

    if(remoteNode->config.isASPAware) return CL_OK;
    /*
     * Check if this non-asp aware node has a proxied SU pointing to the node 
     * having the proxy.
     */
    for(suRef = clAmsEntityListGetFirst(&remoteNode->config.suList);
        suRef != NULL;
        suRef = clAmsEntityListGetNext(&remoteNode->config.suList, suRef))
    {
        ClAmsSUT *proxiedSU = (ClAmsSUT*)suRef->ptr;
        ClAmsEntityRefT *compRef = NULL;
        for(compRef = clAmsEntityListGetFirst(&proxiedSU->config.compList);
            compRef != NULL;
            compRef = clAmsEntityListGetNext(&proxiedSU->config.compList, compRef))
        {
            ClAmsCompT *proxiedComp = (ClAmsCompT*)compRef->ptr;
            ClAmsCompT *proxy = (ClAmsCompT*)proxiedComp->status.proxyComp;
            if(!proxy) continue;
            else
            {
                ClAmsSUT *proxySU;
                ClAmsNodeT *proxyNode;
                AMS_CHECK_SU(proxySU = (ClAmsSUT*)proxy->config.parentSU.ptr);
                AMS_CHECK_NODE(proxyNode = (ClAmsNodeT*)proxySU->config.parentNode.ptr);
                if(proxyNode == node)
                {
                    ClAmsProxiedSUEntryT *entry = NULL;
                    entry = clHeapCalloc(1, sizeof(*entry));
                    CL_ASSERT(entry != NULL);
                    entry->su = proxiedSU;
                    clListAddTail(&entry->list, proxiedSUList);
                    break;
                }
            }
        }
    }

    return CL_OK;
}

ClRcT
clAmsPeRemoteProxiedSUsForNode(ClAmsNodeT *node, ClListHeadT *proxiedSUList)
{
    ClAmsRemoteProxiedSUsWalkArgT args = {0};
    args.node = node;
    args.proxiedSUList = proxiedSUList;
    return clAmsEntityDbWalkExtended(&gAms.db.entityDb[CL_AMS_ENTITY_TYPE_NODE],
                                     clAmsPeRemoteProxiedSUsWalkCallback,
                                     (ClPtrT)&args);
}

/*
 * clAmsPeSUHasProxiedComponents
 * -----------------------------
 * Returns CL_TRUE if the SU has any proxied components otherwise CL_FALSE.
 */

ClRcT
clAmsPeSUHasProxiedComponents(
        CL_IN   ClAmsSUT *su)
{
    ClAmsEntityRefT *entityRef;

    AMS_CHECK_SU ( su );

    AMS_FUNC_ENTER ( ("SU [%s]\n",su->config.entity.name.value) );

    for ( entityRef = clAmsEntityListGetFirst(&su->config.compList);
          entityRef != (ClAmsEntityRefT *) NULL;
          entityRef = clAmsEntityListGetNext(&su->config.compList, entityRef) )
    {
        ClAmsCompT *comp = (ClAmsCompT *) entityRef->ptr;

        AMS_CHECK_COMP ( comp );

        if ( (comp->config.property == 
                    CL_AMS_COMP_PROPERTY_PROXIED_PREINSTANTIABLE) ||
             (comp->config.property ==
                    CL_AMS_COMP_PROPERTY_PROXIED_NON_PREINSTANTIABLE) )
        {
            return CL_TRUE;
        }

    }

    return CL_FALSE;
}

/*
 * clAmsPeSUComputeLoad
 * --------------------
 * 
 * This fn computes the load on a SU. The definition of "load" is intended
 * to be system specific. It defaults to the number of SIs that are assigned
 * to the SU, but with appropriate callouts can be any designer specific
 * definition.
 *
 * The load has to be in the range of 0..100
 */

ClUint32T
clAmsPeSUComputeLoad(
        CL_IN   ClAmsSUT *su)
{
    ClAmsSGT *sg = (ClAmsSGT *) su->config.parentSG.ptr;

    AMS_CHECK_SG(sg);

    return ( (su->status.numActiveSIs + su->status.numStandbySIs) * 100 ) /
                (sg->config.maxActiveSIsPerSU + sg->config.maxStandbySIsPerSU);
}

/******************************************************************************
 * SI Functions
 *****************************************************************************/

/*-----------------------------------------------------------------------------
 * Functions called from the Management API
 *---------------------------------------------------------------------------*/

/*
 * clAmsPeSIUnlock
 * ---------------
 * Unlocking a SI allows AMS to assign an active or standby state to an SU
 * on behalf of the SI.
 */

ClRcT
clAmsPeSIUnlock(
        CL_IN       ClAmsSIT        *si)
{
    ClAmsAdminStateT adminState;
    ClAmsSGT *sg = NULL;

    AMS_CHECK_SI ( si );

    AMS_CHECK_SG( sg = (ClAmsSGT*)si->config.parentSG.ptr);

    AMS_FUNC_ENTER ( ("SI [%s]\n", si->config.entity.name.value) );

    AMS_ENTITY_LOG (si, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
            ("Admin Operation [Unlock] on SI [%s]\n",
             si->config.entity.name.value));

    if(clAmsInvocationPendingForSI(si))
    {
        clLogInfo("SI", "UNLOCK", 
                  "SG [%s] containing SI [%s] has pending invocations. Deferring unlock",
                  sg->config.entity.name.value, si->config.entity.name.value);
        return CL_AMS_RC(CL_ERR_TRY_AGAIN);
    }

    CL_AMS_SET_A_STATE(si, CL_AMS_ADMIN_STATE_UNLOCKED);

    AMS_CALL ( clAmsPeSIComputeAdminState(si, &adminState) );

    if ( adminState == CL_AMS_ADMIN_STATE_UNLOCKED )
    {
        AMS_CALL ( clAmsPeSIEvaluateWork(si) );
    }

    return CL_OK;
}

ClRcT
clAmsPeSIUnlockCallback(
        CL_IN   ClAmsSIT *si,
        CL_IN   ClUint32T error)
{
    return CL_OK;
}

/*
 * clAmsPeSILockAssignment
 * -----------------------
 * When a SU is put in lock assignment, it cannot be assigned to
 * any SUs. Current assignments are removed.
 */

ClRcT
clAmsPeSILockAssignment(
        CL_IN       ClAmsSIT        *si)
{
    ClAmsAdminStateT adminState;
    ClAmsSGT *sg = NULL;

    AMS_CHECK_SI ( si );
    AMS_CHECK_SG ( sg = (ClAmsSGT*)si->config.parentSG.ptr);

    AMS_FUNC_ENTER ( ("SI [%s]\n",si->config.entity.name.value) );

    AMS_ENTITY_LOG (si, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
            ("Admin Operation [Lock Assignment] on SI [%s]\n",
             si->config.entity.name.value));

    if(clAmsInvocationPendingForSI(si))
    {
        clLogInfo("SI", "LOCK", 
                  "SG [%s] containing SI [%s] has pending invocations. Deferring lock",
                  sg->config.entity.name.value, si->config.entity.name.value);
        return CL_AMS_RC(CL_ERR_TRY_AGAIN);
    }

    AMS_CALL ( clAmsPeSIComputeAdminState(si, &adminState) );

    CL_AMS_SET_A_STATE(si, CL_AMS_ADMIN_STATE_LOCKED_A);

    if ( adminState == CL_AMS_ADMIN_STATE_LOCKED_I )
    {
        AMS_CALL ( clAmsPeSIEvaluateWork(si) );
    }
    else if ( adminState == CL_AMS_ADMIN_STATE_UNLOCKED )
    {
        AMS_CALL ( clAmsPeSISwitchoverWork(si, CL_AMS_ENTITY_SWITCHOVER_IMMEDIATE) );
    }

    return CL_OK;
}

/*
 * clAmsPeSILockAssignmentCallback
 * -------------------------------
 * This is the callback after all operations related to SI lock assignment have
 * been completed.
 */

ClRcT
clAmsPeSILockAssignmentCallback(
        CL_IN   ClAmsSIT *si,
        CL_IN   ClUint32T error)
{
    ClAmsSGT *sg;

    AMS_CHECK_SI ( si );
    AMS_CHECK_SG ( sg = (ClAmsSGT *) si->config.parentSG.ptr );

    AMS_FUNC_ENTER ( ("SI [%s]\n", si->config.entity.name.value) );

    /*
     * Make a gratuitous call to SG evaluate work. This is to ensure that other
     * SIs that may be currently unassigned now get the chance to be assigned.
     */

    AMS_CALL ( clAmsPeSGEvaluateWork(sg) );

    return CL_OK;
}

/*
 * clAmsPeSILockInstantiation
 * --------------------------
 * Invalid operation for SI as SI cannot be terminated like other entities.
 * Function is there only as placeholder.
 */

ClRcT
clAmsPeSILockInstantiation(
        CL_IN       ClAmsSIT        *si)
{
    AMS_CHECK_SI ( si );

    AMS_FUNC_ENTER ( ("SI [%s]\n",si->config.entity.name.value) );

    return CL_AMS_RC(CL_ERR_NO_OP);
}

/*
 * clAmsPeSIShutdown
 * -----------------
 * When a SI is put in shutdown, the SI cannot remain assigned to any SU.
 * SUs to which the SI is currently assigned as active or standby are asked
 * to remove their assignments.
 */

ClRcT
clAmsPeSIShutdown(
        CL_IN       ClAmsSIT        *si)
{
    ClAmsAdminStateT oldAdminState;
    ClAmsSGT *sg =  NULL;

    AMS_CHECK_SI ( si );

    AMS_CHECK_SG ( sg = (ClAmsSGT*)si->config.parentSG.ptr);

    AMS_FUNC_ENTER ( ("SI [%s]\n", si->config.entity.name.value) );

    AMS_ENTITY_LOG (si, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
            ("Admin Operation [Shutdown] on SI [%s]\n",
             si->config.entity.name.value));

    if(clAmsInvocationPendingForSI(si))
    {
        clLogInfo("SI", "SHUTDOWN", "SG [%s] containing SI [%s] has pending invocations",
                  sg->config.entity.name.value, si->config.entity.name.value);
        return CL_AMS_RC(CL_ERR_TRY_AGAIN);
    }

    AMS_CALL ( clAmsPeSIComputeAdminState(si, &oldAdminState) );

    CL_AMS_SET_A_STATE(si, CL_AMS_ADMIN_STATE_SHUTTINGDOWN);

    if ( oldAdminState == CL_AMS_ADMIN_STATE_UNLOCKED )
    {
        AMS_CALL ( clAmsPeSISwitchoverWork(si, CL_AMS_ENTITY_SWITCHOVER_GRACEFUL) );
    }

    return CL_OK;

}

/*
 * clAmsPeSIShutdownCallback
 * -------------------------
 * Called when all SI assignments have been shutdown.
 */

ClRcT
clAmsPeSIShutdownCallback(
        CL_IN       ClAmsSIT        *si,
        CL_IN       ClUint32T       result)
{
    ClAmsSGT *sg;
    
    AMS_CHECK_SI ( si );
    AMS_CHECK_SG ( sg = (ClAmsSGT *) si->config.parentSG.ptr );

    AMS_FUNC_ENTER ( ("SI [%s]\n",si->config.entity.name.value) );

    if ( si->config.adminState == CL_AMS_ADMIN_STATE_SHUTTINGDOWN )
    {
        CL_AMS_SET_A_STATE(si, CL_AMS_ADMIN_STATE_LOCKED_A);
    }

    /*
     * Make a gratuitous call to SG evaluate work. This is to ensure that other
     * SIs that may be currently unassigned now get the chance to be assigned.
     */

    AMS_CALL ( clAmsPeSGEvaluateWork(sg) );

    return CL_OK;
}

/*-----------------------------------------------------------------------------
 * Functions called as a result of AMS actions
 *---------------------------------------------------------------------------*/

static ClRcT
clAmsPeSGReductionProcedureMPlusN(ClAmsSGT *sg, ClAmsSIT *si)
{
    ClRcT rc = CL_AMS_RC(CL_ERR_OP_NOT_PERMITTED);
    ClAmsSUT *activeSU = NULL;
    ClAmsSUT *standbySU = NULL;
    ClAmsEntityRefT *eRef = NULL;
    ClAmsEntityReduceRemoveOpT reduceRemoveOp = {0};

    if(!sg->config.reductionProcedure)
        return rc;

    if(sg->status.numCurrActiveSUs >= clAmsPeSGComputeMaxActiveSU(sg))
        return rc;
    /*
     * Check if the SI is active assignable.
     */
    if(clAmsPeSIIsActiveAssignable(si) != CL_OK)
        return rc;
    /*
     * if SI is active assignable, check if this SI can be fully assigned.
     * If fully assignable, then SG engine re-evaluation would get it right.
     */
    if(clAmsPeSGFindSUForActiveAssignmentMPlusN(sg, &activeSU, si) == CL_OK)
        return rc;

    /*
     * Okay - now we have to remove standby assignments from SUs
     * that are ranked the least.
     */
    for(eRef = clAmsEntityListGetFirst(&sg->status.assignedSUList);
        eRef != NULL;
        eRef = clAmsEntityListGetNext(&sg->status.assignedSUList, eRef))
    {
        ClAmsSUT *su = (ClAmsSUT*)eRef->ptr;

        if(!su->status.numStandbySIs) continue;

        if(clAmsPeSUIsAssignable(su) != CL_OK) continue;
        /*
         * If this guy has a remove OP pending, ignore.
         */
        if(clAmsEntityOpPending(&su->config.entity, &su->status.entity, 
                                CL_AMS_ENTITY_OP_REMOVES_MPLUSN))
            continue;
        /*
         * Okay - mark this standby. and lower ranked would override
         */
        standbySU = su;
    }

    if(!standbySU) return rc;

    if(!standbySU->status.siList.numEntities) return rc;

    /*
     * Also push the pending op and mark it for removal
     * Okay we have this standby SU for removal. 
     */
    clLogInfo("SG", "REDUCE", "Running reduction procedure for SG [%s], SI [%s] "
              "by removing standby assignment from SU [%s]",
              sg->config.entity.name.value, si->config.entity.name.value,
              standbySU->config.entity.name.value);
    reduceRemoveOp.sisRemoved = standbySU->status.siList.numEntities;
    clAmsEntityOpPush(&standbySU->config.entity, &standbySU->status.entity, 
                      (void*)&reduceRemoveOp,
                      (ClUint32T)sizeof(reduceRemoveOp),
                      CL_AMS_ENTITY_OP_REDUCE_REMOVE_MPLUSN);
                      
    if((rc = clAmsPeSURemoveWork(standbySU, CL_AMS_ENTITY_SWITCHOVER_IMMEDIATE |
                                 CL_AMS_ENTITY_SWITCHOVER_SU)) != CL_OK)
    {
        void *data = NULL;
        clLogError("SG", "REDUCE", "SU [%s] remove work returned [%#x]",
                   standbySU->config.entity.name.value, rc);
        clAmsEntityOpClear(&standbySU->config.entity, &standbySU->status.entity,
                           CL_AMS_ENTITY_OP_REDUCE_REMOVE_MPLUSN, &data, NULL);
        if(data) clHeapFree(data);
        return rc;
    }
    
    return CL_OK;
}

ClRcT
clAmsPeSGReductionProcedure(ClAmsSGT *sg, ClAmsSIT *si)
{
    if(!sg->config.reductionProcedure) return CL_AMS_RC(CL_ERR_OP_NOT_PERMITTED);

    switch(sg->config.redundancyModel)
    {
    case CL_AMS_SG_REDUNDANCY_MODEL_M_PLUS_N:
        return clAmsPeSGReductionProcedureMPlusN(sg, si);
    default: break;
    }

    return CL_AMS_RC(CL_ERR_OP_NOT_PERMITTED);
}

ClRcT
clAmsPeSIEvaluateWork(
        CL_IN   ClAmsSIT *si)
{
    ClAmsSGT *sg;
    ClAmsAdminStateT adminState;

    AMS_CHECK_SI ( si );
    AMS_CHECK_SG ( sg = (ClAmsSGT *) si->config.parentSG.ptr );

    AMS_FUNC_ENTER ( ("SI [%s]\n",si->config.entity.name.value) );

    /*
     * A SI can only be assigned if it is unlocked.
     */

    AMS_CALL ( clAmsPeSIComputeAdminState(si, &adminState) );

    if ( adminState != CL_AMS_ADMIN_STATE_UNLOCKED )
    {
        AMS_ENTITY_LOG(si, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("SI [%s] is in admin state [%s]. Ignoring work evaluation request..\n",
             si->config.entity.name.value,
             CL_AMS_STRING_A_STATE(adminState)));

        return CL_OK;
    }

    AMS_ENTITY_LOG (si, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE, 
            ("Evaluating work for SI [%s]\n",
             si->config.entity.name.value));

    if(clAmsPeSGReductionProcedure(sg, si) == CL_OK)
    {
        return CL_OK;
    }

    AMS_CALL ( clAmsPeSGEvaluateWork(sg) );

    return CL_OK;
}

/*
 * clAmsPeSISwitchoverWork
 * -----------------------
 * Deactivate an SI for all SUs.
 */

ClRcT
clAmsPeSISwitchoverWork(
        CL_IN ClAmsSIT *si,
        CL_IN ClUint32T switchoverMode)
{
    ClAmsEntityRefT *entityRef;

    AMS_CHECK_SI(si);

    AMS_FUNC_ENTER ( ("SI [%s]\n",si->config.entity.name.value) );

    if ( si->status.suList.numEntities )
    {
        entityRef = clAmsEntityListGetFirst(&si->status.suList);

        while ( entityRef != (ClAmsEntityRefT *) NULL )
        {
            ClAmsSUT *su = (ClAmsSUT *) entityRef->ptr;
            ClAmsSISURefT *suRef = (ClAmsSISURefT *) entityRef;

            /*
             * Get the next SI upfront as the list could be destroyed by the
             * later actions.
             */

            entityRef = clAmsEntityListGetNext(&si->status.suList, entityRef);

            /*
             * Active SIs are quiesced before removing, others are directly
             * removed.
             */

            if ( (suRef->haState == CL_AMS_HA_STATE_ACTIVE) ||
                 (suRef->haState == CL_AMS_HA_STATE_QUIESCING) )
            {
                AMS_CALL ( clAmsPeSUQuiesceSI(su, si, switchoverMode) );
            }
            else
            if ( (suRef->haState == CL_AMS_HA_STATE_STANDBY) ||
                 (suRef->haState == CL_AMS_HA_STATE_QUIESCED) )
            {
                AMS_CALL ( clAmsPeSURemoveSI(su, si, switchoverMode) );
            }
        }
    }
    else
    {
        AMS_CALL ( clAmsPeSISwitchoverCallback(si, CL_OK, switchoverMode) );
    }

    return CL_OK;
}

ClRcT
clAmsPeSISwitchoverCallback(
        CL_IN   ClAmsSIT *si,
        CL_IN   ClRcT error,
        CL_IN   ClUint32T switchoverMode)
{
    AMS_CHECK_SI ( si );

    AMS_FUNC_ENTER ( ("SI [%s]\n", si->config.entity.name.value) );

    AMS_CALL ( clAmsPeSIUpdateDependents(si) );

    if ( (si->config.adminState == CL_AMS_ADMIN_STATE_SHUTTINGDOWN) ||
         (si->config.adminState == CL_AMS_ADMIN_STATE_LOCKED_A) )
    {
        AMS_CALL ( clAmsPeSIRemoveWork(si, switchoverMode) );
    }

    return CL_OK;
}

/*
 * clAmsPeSIRemoveWork
 * -------------------
 * Remove an SI assignment from all SUs
 */

ClRcT
clAmsPeSIRemoveWork(
        CL_IN   ClAmsSIT *si,
        CL_IN   ClUint32T switchoverMode)
{
    ClAmsEntityRefT *entityRef;

    AMS_CHECK_SI ( si );

    AMS_FUNC_ENTER ( ("SI [%s]\n", si->config.entity.name.value) );

    if ( si->status.suList.numEntities )
    {
        entityRef = clAmsEntityListGetFirst(&si->status.suList);

        while ( entityRef != (ClAmsEntityRefT *) NULL )
        {
            ClAmsSUT *su;

            su = (ClAmsSUT *) entityRef->ptr;
            entityRef = clAmsEntityListGetNext(&si->status.suList, entityRef);

            AMS_CALL ( clAmsPeSURemoveSI(su, si, switchoverMode) );
        }
    }
    else
    {
        AMS_CALL ( clAmsPeSIRemoveCallback(si, CL_OK, switchoverMode) );
    }

    return CL_OK;
}

/*
 * clAmsPeSIRemoveCallback
 * -----------------------
 * Called when all SI assignments are removed.
 */

ClRcT
clAmsPeSIRemoveCallback(
        CL_IN   ClAmsSIT *si,
        CL_IN   ClRcT error,
        CL_IN   ClUint32T switchoverMode)
{
    AMS_CHECK_SI ( si );

    AMS_FUNC_ENTER ( ("SI [%s]\n", si->config.entity.name.value) );

    // WORKQUEUE

    if ( si->config.adminState == CL_AMS_ADMIN_STATE_LOCKED_A )
    {
        AMS_CALL ( clAmsPeSILockAssignmentCallback(si, error) );
    }

    if ( si->config.adminState == CL_AMS_ADMIN_STATE_SHUTTINGDOWN )
    {
        AMS_CALL ( clAmsPeSIShutdownCallback(si, error) );
    }

    return CL_OK;
}

/*
 * clAmsPeSIUpdateDependents
 * -------------------------
 * Update SI dependents
 */

ClRcT
clAmsPeSIUpdateDependents(
        CL_IN   ClAmsSIT *si)
{
    ClAmsEntityRefT *eRef;

    AMS_CHECK_SI ( si );

    AMS_FUNC_ENTER ( ("SI [%s]\n", si->config.entity.name.value) );

    AMS_ENTITY_LOG (si, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("Updating dependents of SI [%s]\n",
             si->config.entity.name.value));

    for ( eRef = clAmsEntityListGetFirst(&si->config.siDependentsList);
          eRef != (ClAmsEntityRefT *) NULL;
          eRef = clAmsEntityListGetNext(&si->config.siDependentsList, eRef) )
    {
        ClAmsSIT *dependentSI = (ClAmsSIT *) eRef->ptr;

        AMS_CHECK_SI ( dependentSI );

        if ( (si->status.operState == CL_AMS_OPER_STATE_ENABLED) && 
             (dependentSI->status.operState != CL_AMS_OPER_STATE_ENABLED) )
        {
            AMS_CALL ( clAmsPeSIEvaluateWork(dependentSI) );
        }

        if ( (si->status.operState != CL_AMS_OPER_STATE_ENABLED) && 
             (dependentSI->status.operState == CL_AMS_OPER_STATE_ENABLED) )
        {
            AMS_CALL ( clAmsPeSISwitchoverWork(dependentSI,
                                               CL_AMS_ENTITY_SWITCHOVER_IMMEDIATE) );
        }
    }

    return CL_OK;
}

/*-----------------------------------------------------------------------------
 * SI Utility Functions
 *---------------------------------------------------------------------------*/

/*
 * clAmsPeSIIsActiveAssignable
 * ---------------------------
 * Return CL_TRUE if a SI is assignable and can be assigned an active HA state.
 * This fn should work for all SG models.
 */
ClRcT
clAmsPeSIIsActiveAssignable(CL_IN ClAmsSIT *si)
{
    ClAmsAdminStateT adminState;
    ClAmsSGT *sg;

    AMS_CHECK_SI (si);
    AMS_CHECK_SG (sg = (ClAmsSGT *) si->config.parentSG.ptr);
    
    AMS_FUNC_ENTER ( ("SI [%s]\n", si->config.entity.name.value) );

    AMS_CALL ( clAmsPeSIComputeAdminState(si, &adminState) );

    if ( adminState == CL_AMS_ADMIN_STATE_UNLOCKED )
    {
        if ( si->status.numActiveAssignments < sg->config.numPrefActiveSUsPerSI )
        {
            if(si->status.numStandbyAssignments)
            {
                ClAmsEntityRefT *ref = NULL;
                /*
                 * If SI doesn't have a reassign pending. Then its not active
                 * assignable. Else check for entity remove op.
                 */
                if(!clAmsEntityOpPending(&si->config.entity, &si->status.entity,
                                        CL_AMS_ENTITY_OP_SI_REASSIGN_MPLUSN))
                {
                    return CL_AMS_RC(CL_AMS_ERR_SI_NOT_ASSIGNABLE);
                }

                for(ref = clAmsEntityListGetFirst(&si->status.suList);
                    ref != NULL;
                    ref = clAmsEntityListGetNext(&si->status.suList, ref))
                {
                    ClAmsSUT *su = (ClAmsSUT*)ref->ptr;
                    if(!su->status.numStandbySIs) continue;
                    if(clAmsEntityOpPending(&su->config.entity,
                                            &su->status.entity,
                                            CL_AMS_ENTITY_OP_REMOVES_MPLUSN))
                    {
                        return CL_AMS_RC(CL_AMS_ERR_SI_NOT_ASSIGNABLE);
                    }
                }

            }
            return clAmsEntityListWalkGetEntity(
                                                &si->config.siDependenciesList,
                                                (ClAmsEntityCallbackT)clAmsPeSIIsActiveAssignable2);
        }
    }

    return CL_AMS_RC(CL_AMS_ERR_SI_NOT_ASSIGNABLE);
}

ClRcT
clAmsPeSIIsActiveAssignable2(
        CL_IN ClAmsSIT *si)
{
    AMS_CHECK_SI(si);
    
    AMS_FUNC_ENTER ( ("SI [%s]\n", si->config.entity.name.value) );

    if ( si->status.operState == CL_AMS_OPER_STATE_ENABLED ) 
    {
        return CL_OK;

        // Recursion is not necessary. If an SI is enabled, means all of its
        // dependencies are enabled.
        //
        // return clAmsEntityListWalkGetEntity(
        //            &si->config.dependenciesList,
        //            clAmsPeSIIsAssignable2);
    }

    return CL_AMS_RC(CL_AMS_ERR_SI_NOT_ASSIGNABLE);
}

/*
 * clAmsPeSIIsStandbyAssignable
 * ----------------------------
 * Return CL_TRUE if a SI is assignable and can be assigned a standby HA state.
 * This fn should work for all SG models.
 */

ClRcT
clAmsPeSIIsStandbyAssignable(
        CL_IN ClAmsSIT *si)
{
    ClAmsAdminStateT adminState;
    ClAmsSGT *sg;

    AMS_CHECK_SI (si);
    AMS_CHECK_SG (sg = (ClAmsSGT *) si->config.parentSG.ptr);
    
    AMS_FUNC_ENTER ( ("SI [%s]\n", si->config.entity.name.value) );

    AMS_CALL ( clAmsPeSIComputeAdminState(si, &adminState) );

    if ( (adminState == CL_AMS_ADMIN_STATE_UNLOCKED) &&
         (si->config.numStandbyAssignments) &&
         (si->status.numStandbyAssignments < si->config.numStandbyAssignments) )
    {
        if ( (si->status.operState == CL_AMS_OPER_STATE_ENABLED) &&
             (si->status.numActiveAssignments) )
        {
            return clAmsEntityListWalkGetEntity(
                        &si->config.siDependenciesList,
                        (ClAmsEntityCallbackT)clAmsPeSIIsActiveAssignable2);
        }
    }

    return CL_AMS_RC(CL_AMS_ERR_SI_NOT_ASSIGNABLE);
}

/*
 * clAmsPeSIComputeAdminState
 * --------------------------
 * The computed admin state for a SI is dependent on the SI's own admin state
 * and the admin state of its SG, application and cluster.
 */

ClRcT
clAmsPeSIComputeAdminState(
        CL_IN ClAmsSIT *si,
        CL_OUT ClAmsAdminStateT *adminState)
{
    ClAmsSGT *sg;
    ClAmsAdminStateT sgAdminState;

    AMS_CHECK_SI (si);
    AMS_CHECKPTR (!adminState);
    AMS_CHECK_SG ( (sg = (ClAmsSGT *)si->config.parentSG.ptr) );

    AMS_FUNC_ENTER ( ("SI [%s]\n", si->config.entity.name.value) );

    sgAdminState = clAmsPeSGComputeAdminState(sg);
    
    if ( si->config.adminState      == CL_AMS_ADMIN_STATE_LOCKED_I )
    {
        *adminState = CL_AMS_ADMIN_STATE_LOCKED_A;
        return CL_AMS_RC(CL_AMS_ERR_INVALID_ENTITY_STATE);
    }

    if ( (si->config.adminState     == CL_AMS_ADMIN_STATE_UNLOCKED) &&
         (sgAdminState              == CL_AMS_ADMIN_STATE_UNLOCKED) )
    {
        ClAmsEntityRefT *siRef = NULL;
        *adminState = CL_AMS_ADMIN_STATE_UNLOCKED;
        /*
         * Check the admin state of our dependencies.
         */
        for(siRef = clAmsEntityListGetFirst(&si->config.siDependenciesList);
            siRef != NULL;
            siRef = clAmsEntityListGetNext(&si->config.siDependenciesList, siRef))
        {
            ClAmsAdminStateT dependencyState = CL_AMS_ADMIN_STATE_UNLOCKED;
            ClAmsSIT *dependencySI = (ClAmsSIT*)siRef->ptr;
            if(!dependencySI) continue;
            clAmsPeSIComputeAdminState(dependencySI, &dependencyState);
            if(dependencyState != CL_AMS_ADMIN_STATE_UNLOCKED)
            {
                *adminState = dependencyState;
                break;
            }
        }

        return CL_OK;
    }

    if ( (si->config.adminState     == CL_AMS_ADMIN_STATE_LOCKED_A) ||
         (sgAdminState              == CL_AMS_ADMIN_STATE_LOCKED_I) ||
         (sgAdminState              == CL_AMS_ADMIN_STATE_LOCKED_A) )
    {
        *adminState = CL_AMS_ADMIN_STATE_LOCKED_A;
        return CL_OK;
    }

    *adminState = CL_AMS_ADMIN_STATE_SHUTTINGDOWN;

    return CL_OK;
}

/******************************************************************************
 * Component Functions
 *****************************************************************************/

/*-----------------------------------------------------------------------------
 * Functions called from the Management API
 *---------------------------------------------------------------------------*/

/*
 * clAmsPeCompAdminRestart
 * -----------------------
 * This fn is invoked to inform the PE that a component should be restarted.
 */

ClRcT
clAmsPeCompAdminRestart(
        CL_IN   ClAmsCompT *comp,
        CL_IN   ClUint32T switchoverMode)
{
    ClAmsSUT *su = NULL;

    AMS_CHECK_COMP ( comp );

    AMS_CHECK_SU( su = (ClAmsSUT*)comp->config.parentSU.ptr);

    AMS_FUNC_ENTER ( ("Component [%s]\n", comp->config.entity.name.value) );

    AMS_ENTITY_LOG (comp, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
            ("Admin Operation [Restart] on Component [%s]\n",
             comp->config.entity.name.value));

    if(clAmsInvocationsPendingForSU(su))
    {
        clLogInfo("COMP", "RESTART", "SU [%s] containing comp [%s] has invocations pending",
                  su->config.entity.name.value, comp->config.entity.name.value);
        return CL_AMS_RC(CL_ERR_TRY_AGAIN);
    }

    if ( (comp->status.presenceState != CL_AMS_PRESENCE_STATE_INSTANTIATED) ||
         (clAmsPeSUIsInstantiable((ClAmsSUT *)comp->config.parentSU.ptr) != CL_OK) )
    {
        AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("Component [%s] is not instantiable or instantiated. Cannot Restart..\n",
             comp->config.entity.name.value));

        return CL_AMS_RC(CL_ERR_NO_OP);
    }

    if ( !comp->config.isRestartable )
    {
        AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("Component [%s] is not restartable. Ignoring Restart..\n",
             comp->config.entity.name.value));

        return CL_AMS_RC(CL_ERR_NO_OP);
    }

    return clAmsPeCompRestart(comp, switchoverMode);
}

/*
 * clAmsPeCompRestart
 * ------------------
 * The real comp restart function called internally. Note, this fn and
 * its related callbacks assume that the comp is restartable.
 */

ClRcT
clAmsPeCompRestart(
        CL_IN   ClAmsCompT *comp,
        CL_IN   ClUint32T switchoverMode)
{
    ClBoolT cleanup = CL_FALSE;

    AMS_CHECK_COMP ( comp );

    AMS_FUNC_ENTER ( ("Component [%s]\n", comp->config.entity.name.value) );

    if ( clAmsPeSUIsInstantiable((ClAmsSUT *)comp->config.parentSU.ptr) != CL_OK )
    {
        AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("Component [%s] is not instantiable. Stopping in Restart/Termination\n",
             comp->config.entity.name.value));

        return CL_OK;
    }

    AMS_ENTITY_LOG (comp, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
            ("Restarting Component [%s]\n",
             comp->config.entity.name.value));

    /*
     * Do a cleanup/terminate, instantiate, reassign sequence.
     */

    CL_AMS_SET_P_STATE(comp, CL_AMS_PRESENCE_STATE_RESTARTING);

    if ( (switchoverMode & CL_AMS_ENTITY_SWITCHOVER_FAST) ||
         (comp->status.operState == CL_AMS_OPER_STATE_DISABLED) )
    {
        cleanup = CL_TRUE;
    }

    if ( cleanup )
    {
        AMS_CALL ( clAmsPeCompCleanup(comp) );
    }
    else
    {
        AMS_CALL ( clAmsPeCompTerminate(comp) );
    }

    return CL_OK;
}

ClRcT
clAmsPeCompRestartCallback_Step1(
        CL_IN       ClAmsCompT          *comp,
        CL_IN       ClUint32T           error)
{
    AMS_CHECK_COMP ( comp );

    AMS_FUNC_ENTER ( ("Component [%s]\n",comp->config.entity.name.value) );

    if ( error )
    {
        return clAmsPeCompRestartCallback_Step2(comp, error);
    }

    if ( clAmsPeSUIsInstantiable((ClAmsSUT *)comp->config.parentSU.ptr) != CL_OK )
    {
        AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("Component [%s] is not instantiable. Stopping in Restart/Instantiation\n",
             comp->config.entity.name.value));

        return CL_OK;
    }

    AMS_CALL ( clAmsPeCompInstantiate(comp) );

    return CL_OK;
}

ClRcT
clAmsPeCompRestartCallback_Step2(
        CL_IN       ClAmsCompT          *comp,
        CL_IN       ClUint32T           error)
{
    ClRcT rc = CL_OK;

    AMS_CHECK_COMP ( comp );

    AMS_FUNC_ENTER ( ("Component [%s]\n",comp->config.entity.name.value) );

    /*
     * If the restart operation fails and a pending fault exists, then
     * indicate to the fault handler that the operation has failed, else
     * report a new fault. The recovery action must be at least comp fail
     * over.
     */

    if ( error )
    {
        ClAmsRecoveryT recovery;
        ClUint32T escalation;

        if ( comp->status.recovery )
        {
            AMS_CALL ( clAmsPeCompFaultCallback(comp, error) );
        }

        escalation = clAmsPeEntityComputeFaultEscalation((ClAmsEntityT*) comp);
        recovery = CL_AMS_RECOVERY_COMP_FAILOVER;
        
        clAmsFaultQueueAdd((ClAmsEntityT*)comp);
        rc = clAmsPeCompFaultReport(comp, &recovery, &escalation);
        clAmsFaultQueueDelete((ClAmsEntityT*)comp);
        return rc;
    }

    if ( comp->status.readinessState != CL_AMS_READINESS_STATE_INSERVICE )
    {
        AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("Component [%s] is not inservice. Stopping in Restart/Reassignment\n",
             comp->config.entity.name.value));

        return CL_OK;
    }

    if ( (comp->config.property == CL_AMS_COMP_PROPERTY_SA_AWARE) ||
         (comp->config.property == CL_AMS_COMP_PROPERTY_PROXIED_PREINSTANTIABLE) )
    {
        AMS_CALL ( clAmsPeCompAssignCSIAgain(comp) );
    }

    return CL_OK;
}

/*-----------------------------------------------------------------------------
 * Functions called from the Event API
 *---------------------------------------------------------------------------*/

/*
 * clAmsPeCompFaultReport
 * ----------------------
 * This fn is called to report a fault on a component along with a recommended 
 * recovery action. The fn invokes the necessary cleanup actions on the 
 * component, to be followed up by a recovery action taking escalation into 
 * account as necessary. It returns back an updated recovery action if any 
 * escalation in recovery has taken place.
 */

ClRcT
clAmsPeCompFaultReport(
        CL_IN       ClAmsCompT                  *comp,
        CL_INOUT    ClAmsLocalRecoveryT         *recovery,
        CL_INOUT    ClUint32T                   *escalation)
{
    ClAmsSUT *su;
    ClAmsSGT *sg;
    ClAmsNodeT *node;
    ClAmsLocalRecoveryT recommendedRecovery;

    AMS_CHECKPTR ( !recovery || !escalation );
    AMS_CHECK_COMP ( comp );
    AMS_CHECK_SU ( su = (ClAmsSUT *) comp->config.parentSU.ptr );
    AMS_CHECK_SG ( sg = (ClAmsSGT *) su->config.parentSG.ptr );
    AMS_CHECK_NODE ( node = (ClAmsNodeT *) su->config.parentNode.ptr );

    AMS_FUNC_ENTER ( ("Component [%s]\n", comp->config.entity.name.value) );

    /*
     * Check if there is an entry in the fault queue for this component.
     */
    if(clAmsFaultQueueFind((ClAmsEntityT*)comp, NULL) != CL_OK)
    {
        clLogWarning("AMF", "FLT-COMP", "Fault on component [%s] is not present in the fault queue. Ignoring", comp->config.entity.name.value);
        return CL_OK;
    }
    /*
     * If this is an external fault and it coincides with a current operation
     * we call an appropriate operation abort fn and let it deal with the
     * injection of a fault if necessary.
     *
     * Note: For now, this means that we ignore the recommended recovery in
     * such cases.
     */

    if ( *escalation == CL_FALSE && *recovery < CL_AMS_RECOVERY_NO_RECOMMENDATION )
    {
        if ( clAmsEntityTimerIsRunning(
                                       (ClAmsEntityT *) comp,
                                       CL_AMS_COMP_TIMER_INSTANTIATE) == CL_TRUE )
        {
            AMS_ENTITY_LOG (comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_ERROR,
                            ("Fault on Component [%s]: Translating to Component Instantiate Error\n",
                             comp->config.entity.name.value) );

            return clAmsPeCompInstantiateError(
                                               comp, CL_AMS_RC(CL_AMS_ERR_OPERATION_FAILED));
        }

        if ( clAmsEntityTimerIsRunning(
                                       (ClAmsEntityT *) comp,
                                       CL_AMS_COMP_TIMER_PROXIEDCOMPINSTANTIATE) == CL_TRUE )
        {
            AMS_ENTITY_LOG (comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_ERROR,
                            ("Fault on Component [%s]: Translating to Proxied Component Instantiate Error\n",
                             comp->config.entity.name.value) );

            return clAmsPeCompProxiedCompInstantiateError(
                                                          comp, CL_AMS_RC(CL_AMS_ERR_OPERATION_FAILED));
        }

        if ( clAmsEntityTimerIsRunning((ClAmsEntityT *) comp, CL_AMS_COMP_TIMER_TERMINATE) == CL_TRUE )
        {
            AMS_ENTITY_LOG (comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_LOG_SEV_ERROR,
                            ("Fault on Component [%s] during terminate: Translating to Component Terminate Error\n",
                             comp->config.entity.name.value) );

            return clAmsPeCompTerminateError(
                                             comp, CL_AMS_RC(CL_AMS_ERR_OPERATION_FAILED));
        }

        if ( clAmsEntityTimerIsRunning(
                                       (ClAmsEntityT *) comp,
                                       CL_AMS_COMP_TIMER_CSISET) == CL_TRUE )
        {
            AMS_ENTITY_LOG (comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
                            ("Fault on Component [%s]: Translating to CSI Set Error\n",
                             comp->config.entity.name.value) );

            return clAmsPeCompAssignCSIError(
                                             comp, CL_AMS_RC(CL_AMS_ERR_OPERATION_FAILED));
        }

        if ( clAmsEntityTimerIsRunning(
                                       (ClAmsEntityT *) comp,
                                       CL_AMS_COMP_TIMER_QUIESCINGCOMPLETE) == CL_TRUE )
        {
            AMS_ENTITY_LOG (comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
                            ("Fault on Component [%s]: Translating to CSI Quiescing Error\n",
                             comp->config.entity.name.value) );

            return clAmsPeCompQuiesceCSIGracefullyError(
                                                        comp, CL_AMS_RC(CL_AMS_ERR_OPERATION_FAILED));
        }

        if ( clAmsEntityTimerIsRunning(
                                       (ClAmsEntityT *) comp,
                                       CL_AMS_COMP_TIMER_CSIREMOVE) == CL_TRUE )
        {
            AMS_ENTITY_LOG (comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
                            ("Fault on Component [%s]: Translating to CSI Remove Error\n",
                             comp->config.entity.name.value) );

            return clAmsPeCompRemoveCSIError(
                                             comp, CL_AMS_RC(CL_AMS_ERR_OPERATION_FAILED));
        }
    }


    /*
     * Compute recovery action and escalation
     */

    CL_AMS_SET_O_STATE(comp, CL_AMS_OPER_STATE_DISABLED);

    recommendedRecovery = *recovery;

    AMS_CALL ( clAmsPeCompComputeRecoveryAction(comp, recovery, escalation) );

    if ( *recovery == CL_AMS_RECOVERY_NONE )
    {
        clLogDebug(CL_LOG_AREA_AMS, CL_LOG_CONTEXT_AMS_FAULT_COMP, 
                   "Fault on Component "\
                   "[%s]: Recommended recovery = [%s], "\
                   "Computed recovery = [%s]. Ignoring fault",
                   comp->config.entity.name.value,
                   CL_AMS_STRING_RECOVERY(recommendedRecovery),
                   CL_AMS_STRING_RECOVERY(*recovery));

        return CL_OK;
    }

    /*
     * Stop all possible timers that could be running for the component
     * and clear the invocation list.
     */

    AMS_CALL ( clAmsEntityClearOps((ClAmsEntityT *)comp) );
    
    clAmsEntityOpsClear((ClAmsEntityT*)comp, &comp->status.entity);

    clLogDebug(CL_LOG_AREA_AMS, CL_LOG_CONTEXT_AMS_FAULT_COMP, 
               "Fault on Component [%s]: Recommended recovery = [%s], "\
               "Computed recovery = [%s]. Escalation = [%s]",
               comp->config.entity.name.value,
               CL_AMS_STRING_RECOVERY(recommendedRecovery),
               CL_AMS_STRING_RECOVERY(*recovery),
               *escalation ? "True" : "False");

    /*
     * Undertake recovery actions
     */

    switch ( *recovery )
    {
    case CL_AMS_RECOVERY_COMP_RESTART:
        {
            if ( !su->status.compRestartTimer.count )
            {
                AMS_CALL ( clAmsEntityTimerStart((ClAmsEntityT *) su,
                                                 CL_AMS_SU_TIMER_COMPRESTART) );
            }

            CL_AMS_SET_O_STATE(comp, CL_AMS_OPER_STATE_ENABLED);

            AMS_CALL ( clAmsPeCompRestart(comp, CL_AMS_ENTITY_SWITCHOVER_FAST) );

            break;
        }

    case CL_AMS_RECOVERY_SU_RESTART:
        {
            AMS_CALL( clAmsPeSUFaultReport(su, comp, recovery, escalation) );
            break;
        }

    case CL_AMS_RECOVERY_COMP_FAILOVER:
        {
            AMS_CALL ( clAmsPeSUFaultReport(su, comp, recovery, escalation) );

            break;
        }

    case CL_AMS_RECOVERY_NODE_SWITCHOVER:
    case CL_AMS_RECOVERY_NODE_FAILOVER:
    case CL_AMS_RECOVERY_NODE_FAILFAST:
        {
            AMS_CALL ( clAmsPeNodeFaultReport(node, comp, recovery, escalation) );

            break;
        }

        // Future

    case CL_AMS_RECOVERY_APP_RESTART:
        {
            return CL_AMS_RC(CL_ERR_NOT_IMPLEMENTED);
        }

    case CL_AMS_RECOVERY_CLUSTER_RESET:
        {
            AMS_CALL ( clAmsPeClusterFaultReport(&gAms, recovery, escalation) );

            break;
        }

    default:
        {
            return CL_AMS_RC(CL_ERR_NOT_IMPLEMENTED);
        }
    }

    return CL_OK;
}

/*
 * clAmsPeCompFaultCallback
 * ------------------------
 * This fn is internally invoked by AMS once a fault on a component has been
 * processed and the recovery action completed. Faults handled by this fn are: 
 * comp restart.
 */

ClRcT
clAmsPeCompFaultCallback(
        CL_IN       ClAmsCompT          *comp,
        CL_IN       ClUint32T           error)
{
    ClBoolT repairNecessary;
    ClAmsRecoveryT recovery;

    AMS_CHECK_COMP ( comp );

    AMS_FUNC_ENTER ( ("Component [%s]\n", comp->config.entity.name.value) );

    /*
     * The recovery action is now completed. 
     */

    repairNecessary = CL_TRUE;
    recovery = comp->status.recovery;

    if ( error || 
         (comp->status.presenceState == CL_AMS_PRESENCE_STATE_TERMINATION_FAILED) ||
         (comp->status.presenceState == CL_AMS_PRESENCE_STATE_INSTANTIATION_FAILED) )
    {
        clLogError(CL_LOG_AREA_AMS, CL_LOG_CONTEXT_AMS_FAULT_COMP,
                   "Fault on Component [%s]: Recovery [%s] Failed",
                   comp->config.entity.name.value,
                   CL_AMS_STRING_RECOVERY(comp->status.recovery));

    }
    else
    {
        clLogInfo(CL_LOG_AREA_AMS, CL_LOG_CONTEXT_AMS_FAULT_COMP,
                  "Fault on Component [%s]: Recovery [%s] Successful",
                  comp->config.entity.name.value,
                  CL_AMS_STRING_RECOVERY(comp->status.recovery));

        if ( comp->status.recovery == CL_AMS_RECOVERY_COMP_RESTART )
        {
            repairNecessary = CL_FALSE;

            comp->status.recovery = CL_AMS_RECOVERY_NONE;
        }
    }

#ifdef AMS_CPM_INTEGRATION

    ClRcT rc = CL_OK;

    clLogDebug(CL_LOG_AREA_AMS, CL_LOG_CONTEXT_AMS_FAULT_COMP,
               "Fault on Component [%s]: Reporting fault to FM. "\
               "Recovery [%s], Repair Necessary [%s]",
               comp->config.entity.name.value,
               CL_AMS_STRING_RECOVERY(recovery),
               CL_AMS_STRING_BOOLEAN(repairNecessary));

    if ( (rc = _clAmsSAFaultReportCallback(
                        (ClAmsEntityT *)comp,
                        recovery,
                        repairNecessary)) != CL_OK )
    {
        if (CL_GET_ERROR_CODE(rc) != CL_ERR_INVALID_HANDLE)  // Invalid handle is OK, it just means that alarm has not been initialized
          clLogWarning(CL_LOG_AREA_AMS, CL_LOG_CONTEXT_AMS_FAULT_COMP,
                     "Fault on Component [%s]: Error [0x%x] when reporting "\
                     "fault to FM. Continuing..",
                     comp->config.entity.name.value,
                     rc);
    }

#endif

    return CL_OK;
}

/*
 * clAmsPeCompComputeRecoveryAction
 * --------------------------------
 * Given a component and a recommended recovery action, this fn computes
 * an appropriate recovery taking escalation rules into account. It does
 * not initiate the recovery action as that may happen asynchronously.
 */

ClRcT
clAmsPeCompComputeRecoveryAction(
        CL_IN       ClAmsCompT                  *comp,
        CL_INOUT    ClAmsLocalRecoveryT         *recovery,
        CL_OUT      ClUint32T                   *escalation)
{
    ClAmsSUT *su;
    ClAmsSGT *sg;
    ClAmsNodeT *node;
    ClBoolT match = CL_FALSE;
    ClAmsAdminStateT adminState;
    ClAmsRecoveryT computedRecovery;

    extern ClAmsT gAms; 

    AMS_CHECKPTR ( !recovery || !escalation );
    AMS_CHECK_COMP ( comp );
    AMS_CHECK_SU ( su = (ClAmsSUT *) comp->config.parentSU.ptr );
    AMS_CHECK_SG ( sg = (ClAmsSGT *) su->config.parentSG.ptr );
    AMS_CHECK_NODE ( node = (ClAmsNodeT *) su->config.parentNode.ptr );

    AMS_FUNC_ENTER ( ("Component [%s]\n", comp->config.entity.name.value) );

    computedRecovery = *recovery;

    /*
     * Escalation rules can be positive or negative. Positive rules increase
     * the scope of recovery to least minimum acceptable. Negative rules
     * decrease the scope of recovery, typically to none, indicating that
     * the fault should be ignored.
     *
     * The symbols, +/-/= are used below to describe the type of rule.
     */

    /*
     * -ve
     * Check the state of the cluster. If cluster is not coming up or not
     * running, it makes no sense to do any recovery.
     */

    if ( (gAms.serviceState != CL_AMS_SERVICE_STATE_STARTINGUP) &&
         (gAms.serviceState != CL_AMS_SERVICE_STATE_RUNNING) )
    {
        computedRecovery = CL_AMS_RECOVERY_NONE;
    }

    /*
     * -ve
     * If the node is not a member of the cluster, then no recovery. This is
     * to catch duplicate errors on components after a node has left.
     */

    if ( node->config.isASPAware &&
         node->status.isClusterMember == CL_AMS_NODE_IS_NOT_CLUSTER_MEMBER )
    {
        computedRecovery = CL_AMS_RECOVERY_NONE;
    }

    /*
     * -ve
     * If the component is uninstantiated, then ignore the fault.
     */

    if ( comp->status.presenceState == CL_AMS_PRESENCE_STATE_UNINSTANTIATED )
    {
        computedRecovery = CL_AMS_RECOVERY_NONE;
    }

    /*
     * +ve
     * If there is no recommendation, then set the minimum
     */

    if ( computedRecovery == CL_AMS_RECOVERY_NO_RECOMMENDATION )
    {
        match = CL_TRUE;
        computedRecovery = comp->config.recoveryOnTimeout;

        if ( computedRecovery == CL_AMS_RECOVERY_NO_RECOMMENDATION )
        {
            computedRecovery = comp->config.isRestartable ?  
                            CL_AMS_RECOVERY_COMP_RESTART : 
                            CL_AMS_RECOVERY_COMP_FAILOVER;
        }
    }

    /*
     * +ve
     * If the SU is shutting down or locked assignment, then the least
     * level of recovery is COMP_FAILOVER. This is to ensure that we
     * don't restart a component/su that is not fully usable.
     */

    clAmsPeSUComputeAdminState(su, &adminState);

    if ( (adminState == CL_AMS_ADMIN_STATE_LOCKED_A) ||
         (adminState == CL_AMS_ADMIN_STATE_SHUTTINGDOWN) )
    {
        if ( (computedRecovery == CL_AMS_RECOVERY_NO_RECOMMENDATION) ||
             (computedRecovery == CL_AMS_RECOVERY_COMP_RESTART)      ||
             (computedRecovery == CL_AMS_RECOVERY_SU_RESTART) )
        {
            computedRecovery = CL_AMS_RECOVERY_COMP_FAILOVER;
        }
    }

    /*
     * +ve
     * If the node is leaving, then the least level of recovery is
     * COMP_FAILOVER. This is to ensure that we don't restart a 
     * component/su that is not fully usable.
     */

    if ( node->status.isClusterMember == CL_AMS_NODE_IS_LEAVING_CLUSTER )
    {
        if ( (computedRecovery == CL_AMS_RECOVERY_NO_RECOMMENDATION) ||
             (computedRecovery == CL_AMS_RECOVERY_COMP_RESTART)      ||
             (computedRecovery == CL_AMS_RECOVERY_SU_RESTART) )
        {
            computedRecovery = CL_AMS_RECOVERY_COMP_FAILOVER;
        }
    }

    /*
     * -ve
     * If this is an external fault, it is only allowed if it has a larger scope
     * than any existing fault recovery affecting the component.
     */

    if ( *escalation == CL_FALSE )
    {
        if ( clAmsPeEntityRecoveryScopeLarger(
                    comp->status.recovery, computedRecovery) || 
             clAmsPeEntityRecoveryScopeLarger(
                    su->status.recovery, computedRecovery) || 
             clAmsPeEntityRecoveryScopeLarger(
                    node->status.recovery, computedRecovery) )
        {
            computedRecovery = CL_AMS_RECOVERY_NONE;
        }
    }

    /*
     * +ve
     * If this is an escalated fault, make sure it is atleast at the same level
     * as the current recovery.
     */

    if ( *escalation == CL_TRUE )
    {
        if ( clAmsPeEntityRecoveryScopeLarger(
                    comp->status.recovery, computedRecovery) )
        {
            computedRecovery = comp->status.recovery;
        }

        if ( clAmsPeEntityRecoveryScopeLarger(
                    su->status.recovery, computedRecovery) )
        {
            computedRecovery = su->status.recovery;
        }

        if ( clAmsPeEntityRecoveryScopeLarger(
                    node->status.recovery, computedRecovery) )
        {
            computedRecovery = node->status.recovery;
        }
    }

    /*
     * Compute recovery using recommendation and escalation rules.
     */

    if ( computedRecovery == CL_AMS_RECOVERY_COMP_RESTART )
    {
        match = CL_TRUE;

        if ( comp->config.isRestartable )
        {
            comp->status.restartCount ++;
            su->status.compRestartCount ++;

            if ( su->status.compRestartCount > sg->config.compRestartCountMax )
            {
                computedRecovery = CL_AMS_RECOVERY_SU_RESTART;
                *escalation = CL_TRUE;
            }
        }
        else
        {
            computedRecovery = CL_AMS_RECOVERY_SU_RESTART;
            *escalation = CL_TRUE;
        }
    }

    if ( computedRecovery == CL_AMS_RECOVERY_SU_RESTART )
    {
        match = CL_TRUE;
    }

    if ( computedRecovery == CL_AMS_RECOVERY_COMP_FAILOVER )
    {
        match = CL_TRUE;
        comp->status.failoverCount++;
    }

    if ( computedRecovery == CL_AMS_RECOVERY_NODE_SWITCHOVER )
    {
        match = CL_TRUE;
    }

    if ( computedRecovery == CL_AMS_RECOVERY_NODE_FAILOVER )
    {
        match = CL_TRUE;
    }

    if ( computedRecovery == CL_AMS_RECOVERY_NODE_FAILFAST )
    {
        match = CL_TRUE;
    }

    if ( computedRecovery == CL_AMS_RECOVERY_APP_RESTART )
    {
        // no escalation rules for now
    }

    if ( computedRecovery == CL_AMS_RECOVERY_CLUSTER_RESET )
    {
        match = CL_TRUE;
    }

    if ( !*escalation && computedRecovery == comp->status.recovery)
    {
        match = CL_FALSE;
    }

    if ( !match )
    {
        computedRecovery = CL_AMS_RECOVERY_NONE;
    }

    *recovery = computedRecovery;

    if ( computedRecovery != CL_AMS_RECOVERY_NONE )
    {
        comp->status.recovery = computedRecovery;
    }


    return CL_OK;
}

/*
 * clAmsPeCompComputeSwitchoverMode
 * --------------------------------
 * Given a component and a switchover mode, this fn computes
 * an appropriate switchover mode.
 */

ClRcT
clAmsPeCompComputeSwitchoverMode(
        CL_IN       ClAmsCompT                  *comp,
        CL_INOUT    ClUint32T                   *switchoverMode)
{
    ClAmsSUT *su;
    ClAmsNodeT *node;
    ClUint32T computedSwitchoverMode;

    AMS_CHECKPTR ( !switchoverMode);
    AMS_CHECK_COMP ( comp );
    AMS_CHECK_SU ( su = (ClAmsSUT *) comp->config.parentSU.ptr );
    AMS_CHECK_NODE ( node = (ClAmsNodeT *) su->config.parentNode.ptr );

    AMS_FUNC_ENTER ( ("Component [%s]\n", comp->config.entity.name.value) );

    computedSwitchoverMode = *switchoverMode;

    if ( node->status.isClusterMember == CL_AMS_NODE_IS_NOT_CLUSTER_MEMBER 
         &&
         node->config.isASPAware)
    {
        computedSwitchoverMode |= CL_AMS_ENTITY_SWITCHOVER_FAST;   
    }

    /*
     * Try to avoid faults incase the node is unreachable.
     */
    if( node->status.isClusterMember == CL_AMS_NODE_IS_LEAVING_CLUSTER )
    {
        if(clAmsPeCheckNodeHasLeft(node) == CL_TRUE)
        {
            computedSwitchoverMode |= CL_AMS_ENTITY_SWITCHOVER_FAST;
        }
    }

    if ( node->status.operState == CL_AMS_OPER_STATE_DISABLED 
         &&
         node->config.isASPAware)
    {
        computedSwitchoverMode |= CL_AMS_ENTITY_SWITCHOVER_FAST;   
    }

    if(!node->config.isASPAware
       &&
       comp->config.property == CL_AMS_COMP_PROPERTY_SA_AWARE)
    {
        computedSwitchoverMode |= CL_AMS_ENTITY_SWITCHOVER_FAST;
    }

    if ( su->status.presenceState == CL_AMS_PRESENCE_STATE_RESTARTING )
    {
        computedSwitchoverMode |= CL_AMS_ENTITY_SWITCHOVER_FAST;   
    }

    if ( (comp->status.operState == CL_AMS_OPER_STATE_DISABLED) || 
         (comp->status.presenceState == CL_AMS_PRESENCE_STATE_UNINSTANTIATED) || 
         (comp->status.presenceState == CL_AMS_PRESENCE_STATE_RESTARTING) )
    {
        computedSwitchoverMode |= CL_AMS_ENTITY_SWITCHOVER_FAST;   
    }

    if( (computedSwitchoverMode & CL_AMS_ENTITY_SWITCHOVER_FAST) )
        computedSwitchoverMode &= ~(CL_AMS_ENTITY_SWITCHOVER_GRACEFUL | CL_AMS_ENTITY_SWITCHOVER_IMMEDIATE);

    *switchoverMode = computedSwitchoverMode;

    return CL_OK;
}
        

/*-----------------------------------------------------------------------------
 * Component life cycle management functions
 *---------------------------------------------------------------------------*/

/*
 * clAmsPeCompInstantiate
 * ----------------------
 * This fn is called to instantiate (start) a component. It may be called at
 * different times depending on the type of the component. So preinstantiable
 * components can be instantiated as part of a SU start/restart, a component
 * restart or a component instantiate retry. Nonpreinstantiable components
 * will be instantiated as part of a SI assignment to a SU when a CSI is
 * assigned to the component.
 */

ClRcT
clAmsPeCompInstantiate(
        CL_IN       ClAmsCompT        *comp)
{
    ClAmsPresenceStateT compPresenceState;
    ClAmsSUT *su;

    AMS_CHECK_COMP ( comp );
    AMS_CHECK_SU ( (su = (ClAmsSUT *) comp->config.parentSU.ptr) );

    AMS_FUNC_ENTER ( ("Component [%s]\n",comp->config.entity.name.value) );

    /*
     * Verify that component is in appropriate state to be instantiated.
     */

    if ( (comp->status.presenceState != CL_AMS_PRESENCE_STATE_UNINSTANTIATED) &&
         (comp->status.presenceState != CL_AMS_PRESENCE_STATE_INSTANTIATING)  &&
         (comp->status.presenceState != CL_AMS_PRESENCE_STATE_RESTARTING) )
    {
        AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
            ("Component [%s] is [%s]. Ignoring instantiate request..\n",
            comp->config.entity.name.value,
            CL_AMS_STRING_P_STATE(comp->status.presenceState)));

        return CL_OK;
    }

    CL_AMS_SET_EPOCH(comp);

    /*
     * There are limited attempts to instantiate a component.
     */

    if ( comp->status.instantiateCount < comp->config.numMaxInstantiate )
    {
        return clAmsPeCompInstantiate2(comp);
    }
    else
    {
        if ( comp->status.instantiateDelayCount <
                comp->config.numMaxInstantiateWithDelay )
        {
            /*
             * We can still try to instantiate but go slow.
             */

            AMS_CALL ( clAmsEntityTimerStart(
                            (ClAmsEntityT *) comp,
                            CL_AMS_COMP_TIMER_INSTANTIATEDELAY) );

            return CL_OK;
        }
    }

    CL_AMS_RESET_EPOCH(comp);

    /*
     * Too many attempts, give up.
     */

    compPresenceState = comp->status.presenceState;

    CL_AMS_SET_P_STATE(comp, CL_AMS_PRESENCE_STATE_INSTANTIATION_FAILED);
    CL_AMS_SET_O_STATE(comp, CL_AMS_OPER_STATE_DISABLED);

    /*
     * CODEREVIEW: As per latest MIB, this may be a node parameter, not SG
     * Future: SG may contain a config parameter to allow one node reboot
     * attempt in this situation.
     */

    // WORKQUEUE

    if ( compPresenceState == CL_AMS_PRESENCE_STATE_RESTARTING )
    {
        if ( su->status.numInstantiatedComp )
        {
            su->status.numInstantiatedComp --;
        }

        AMS_CALL ( clAmsPeCompRestartCallback_Step2(
                        comp,
                        CL_AMS_RC(CL_ERR_NOT_INITIALIZED)) );
    }
    else
    {
        AMS_CALL ( clAmsPeSUInstantiateError(
                        su,
                        CL_AMS_RC(CL_ERR_NOT_INITIALIZED)) );
    }

    return CL_OK;
}

ClRcT
clAmsPeCompInstantiate2(
        CL_IN       ClAmsCompT        *comp)
{
    ClRcT error = CL_OK;

    AMS_CHECK_COMP ( comp );

    AMS_FUNC_ENTER ( ("Component [%s]\n",comp->config.entity.name.value) );

    /*
     * Verify that component is in appropriate state to be instantiated.
     */

    if ( (comp->status.presenceState != CL_AMS_PRESENCE_STATE_UNINSTANTIATED) &&
         (comp->status.presenceState != CL_AMS_PRESENCE_STATE_INSTANTIATING)  &&
         (comp->status.presenceState != CL_AMS_PRESENCE_STATE_RESTARTING) )
    {
        AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
            ("Component [%s] is in Presence State [%s]. Ignoring instantiate request..\n",
            comp->config.entity.name.value,
            CL_AMS_STRING_P_STATE(comp->status.presenceState)));

        return CL_OK;
    }

    AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
        ("Instantiating [%s] Component [%s], Attempt [%d], Presence State [%s]\n",
         CL_AMS_STRING_COMP_PROPERTY(comp->config.property),
         comp->config.entity.name.value,
         comp->status.instantiateCount + 1,
         CL_AMS_STRING_P_STATE(comp->status.presenceState))); 

    /*
     * Update component state and count.
     */

    if ( comp->status.presenceState != CL_AMS_PRESENCE_STATE_RESTARTING )
    {
        CL_AMS_SET_P_STATE(comp, CL_AMS_PRESENCE_STATE_INSTANTIATING);
    }

    /*
     * Instantiate the component taking its property into account.
     */

    switch ( comp->config.property )
    {
        case CL_AMS_COMP_PROPERTY_SA_AWARE:
        {
            /*
             * Invoke the instantiation command for the component. The component
             * must respond with a register response within the specified timeout.
             */

            comp->status.instantiateCount++;

            AMS_CALL ( clAmsEntityTimerStart(
                            (ClAmsEntityT*) comp,
                            CL_AMS_COMP_TIMER_INSTANTIATE) ); 
            
#ifdef AMS_CPM_INTEGRATION

            ++comp->status.instantiateCookie;
            error = _clAmsSAComponentInstantiate(
                        &comp->config.entity.name,
                        &comp->config.entity.name,
                        comp->status.instantiateCookie);

            if ( error )
            {
                return clAmsPeCompInstantiateError(comp, error);
            }
#endif

            break;
        }

        case CL_AMS_COMP_PROPERTY_PROXIED_PREINSTANTIABLE:
        {
            /*
             * If a proxy for this component is up and running, the proxyComp
             * field of this component will have been set to a non NULL value.
             * Ask the proxy to instantiate the component. Once the component
             * is started, its proxy will register it with the AMF resulting
             * in a proxied component instantiate callback. If the proxyComp
             * field is NULL, it means there is no ready proxy that can take
             * care of this component, just print a message and do nothing.
             */

            if ( clAmsPeCompIsProxyReady(comp) == CL_FALSE )
            {
                AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
                    ("Component [%s] has no proxy. Defering instantiation\n",
                     comp->config.entity.name.value));

                return CL_OK;
            }

            comp->status.instantiateCount++;

            AMS_CALL ( clAmsEntityTimerStart(
                            (ClAmsEntityT*) comp,
                            CL_AMS_COMP_TIMER_PROXIEDCOMPINSTANTIATE) );

#ifdef AMS_CPM_INTEGRATION

            ClAmsCompT *proxy = (ClAmsCompT *) comp->status.proxyComp;
            ClAmsSUT *su = (ClAmsSUT*)comp->config.parentSU.ptr;
            if(su)
            {
                if(su->status.instantiateLevel < comp->config.instantiateLevel)
                    su->status.instantiateLevel = comp->config.instantiateLevel;
                if(su->config.isPreinstantiable)
                    ++su->status.numPIComp;
            }
            ++comp->status.instantiateCookie;
            error = _clAmsSAComponentInstantiate(
                            &comp->config.entity.name,
                            &proxy->config.entity.name,
                            comp->status.instantiateCookie);
            
#endif

            if ( error )
            {
                return clAmsPeCompProxiedCompInstantiateError(comp, error);
            }

            break;
        }

        case CL_AMS_COMP_PROPERTY_PROXIED_NON_PREINSTANTIABLE:
        {
            /*
             * If a proxy for this component is up and running, the proxyComp
             * field of this component will have been set to a non NULL value.
             * As this is a non-preinstantiable component, ask the proxy to
             * instantiate the component by making a CSI set call. Once the 
             * component is started, its proxy will register it with the AMF 
             * with a CSI set response. If the proxyComp field is NULL, it 
             * means there is no ready proxy that can take care of this 
             * component, just print a message and do nothing.
             */

            if ( clAmsPeCompIsProxyReady(comp) == CL_FALSE )
            {
                AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
                    ("Component [%s] has no proxy. Defering instantiation\n",
                     comp->config.entity.name.value));

                return CL_OK;
            }

            ++comp->status.instantiateCount;

            ClAmsCompCSIRefT *csiRef;
            ClAmsCSIT *csi;
            
            csiRef = (ClAmsCompCSIRefT *)clAmsEntityListGetFirst(
                                                        &comp->status.csiList);

            if ( !csiRef )
            {
                AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
                    ("Component [%s] has no CSI assignment. Defering instantiation\n",
                     comp->config.entity.name.value));

                return CL_OK;
            }

            csi = (ClAmsCSIT *) csiRef->entityRef.ptr;

            ClAmsCSIDescriptorT *csiDescriptor = NULL;
            ClInvocationT invocation = 0;

            AMS_CALL ( clAmsCSIMarshalCSIDescriptor(
                            &csiDescriptor,
                            csi,
                            CL_AMS_CSI_FLAG_ADD_ONE,
                            csiRef->haState,
                            comp->config.entity.name,
                            CL_AMS_CSI_NEW_ASSIGN,
                            0) );

            AMS_CALL ( clAmsInvocationCreate(
                            CL_AMS_CSI_SET_CALLBACK,
                            comp,
                            csi,
                            &invocation) );

            AMS_CALL ( clAmsEntityTimerStart(
                            (ClAmsEntityT *) comp,
                            CL_AMS_COMP_TIMER_CSISET) );

#ifdef AMS_CPM_INTEGRATION
 
            ClAmsCompT *proxy = (ClAmsCompT *) comp->status.proxyComp;
            ClAmsSUT *su = (ClAmsSUT*)comp->config.parentSU.ptr;
            if(su)
            {
                if(su->status.instantiateLevel < comp->config.instantiateLevel)
                    su->status.instantiateLevel = comp->config.instantiateLevel;
                if(su->config.isPreinstantiable 
                   && 
                   su->status.numPIComp < su->config.numComponents)
                    ++su->status.numPIComp;
            }

            if ( csiRef->haState == CL_AMS_HA_STATE_ACTIVE )
            {
                error = _clAmsSACSISet(
                                &comp->config.entity.name,
                                &proxy->config.entity.name,
                                invocation,
                                CL_AMS_HA_STATE_ACTIVE,
                                *csiDescriptor);
            }
            else
            {
                AMS_CALL ( clAmsPeCompAssignCSICallback(
                                comp,
                                invocation,
                                CL_OK,
                                CL_AMS_ENTITY_SWITCHOVER_IMMEDIATE) );
            }

#endif

            clAmsFreeMemory(csiDescriptor->csiAttributeList.attribute);
            clAmsFreeMemory(csiDescriptor);

            if ( error )
            {
                AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
                    ("CSI assignment to Component [%s] with HA state [%s] returned Error [0x%x]\n",
                     comp->config.entity.name.value,
                     CL_AMS_STRING_H_STATE(CL_AMS_HA_STATE_ACTIVE),
                     error) );

                return clAmsPeCompAssignCSIError(comp, error);
            }

#ifdef AMS_CPM_INTEGRATION
 
            _clAmsSAMarshalPGTrackDispatch(
                            csi,
                            comp,
                            CL_AMS_PG_ADDED);

#endif

            break;
        }

        case CL_AMS_COMP_PROPERTY_NON_PROXIED_NON_PREINSTANTIABLE:
        {
            /*
             * Invoke the instantiation command for the component. No callback
             * is expected from the component.
             */
            ClAmsSUT *su = (ClAmsSUT*)comp->config.parentSU.ptr;

            comp->status.instantiateCount++;

            AMS_CALL ( clAmsEntityTimerStart(
                            (ClAmsEntityT*) comp,
                            CL_AMS_COMP_TIMER_INSTANTIATE) );

#ifdef AMS_CPM_INTEGRATION

            if(su)
            {
                if(su->status.instantiateLevel < comp->config.instantiateLevel)
                    su->status.instantiateLevel = comp->config.instantiateLevel;
            }
            ++comp->status.instantiateCookie;
            error = _clAmsSAComponentInstantiate(
                            &comp->config.entity.name,
                            &comp->config.entity.name,
                            comp->status.instantiateCookie);

#endif

            if ( error )
            {
                AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
                    ("Component [%s] instantiate returned Error [0x%x]\n",
                     comp->config.entity.name.value, error)); 

                AMS_CALL ( clAmsEntityTimerStop(
                                (ClAmsEntityT*) comp,
                                CL_AMS_COMP_TIMER_INSTANTIATE) ); 

                return clAmsPeCompInstantiateError(comp, error);
            }

            return clAmsPeCompInstantiateCallback(comp, CL_OK);
        }

        default:
        {
            /*
             * Error, unknown component type
             */

            AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_ERROR,
                ("Error: Invalid Property [%d] for Component [%s]. Exiting..\n",
                 comp->config.property,
                 comp->config.entity.name.value));

            return CL_AMS_RC(CL_AMS_ERR_BAD_CONFIG);
        }
    }

    return CL_OK;
}

/*
 * clAmsPeCompInstantiateDelayTimeout
 * ----------------------------------
 * This fn is called by the timer code when a component needs to be
 * instantiated after a delay.
 */

ClRcT
clAmsPeCompInstantiateDelayTimeout(
        CL_IN ClAmsEntityTimerT  *timer) 
{
    ClAmsCompT *comp;

    AMS_CHECKPTR ( !timer );
    AMS_CHECK_COMP ( comp = (ClAmsCompT *) timer->entity );

    AMS_FUNC_ENTER ( ("Component [%s]\n",comp->config.entity.name.value) );

    AMS_CALL ( clAmsEntityTimerStop(
                    (ClAmsEntityT *) comp, 
                    CL_AMS_COMP_TIMER_INSTANTIATEDELAY) );

    comp->status.instantiateDelayCount++;

    return clAmsPeCompInstantiate2(comp); 
}


/*
 * clAmsPeCompInstantiateCallback
 * ------------------------------
 * This fn is invoked to indicate an acknowledgement of a previous instantiate
 * request. 
 *
 * (1) For SA aware components, the callback is due to a component register fn. 
 * (2) For non proxied, non preinstantiable components, the callback is invoked
 *     directly after the instantiate command is called. 
 * (3) For proxied preinstantiable components, the callback is invoked when the 
 *     proxy registers the component.
 * (4) For proxied non-preinstantiable components, a csi set response results
 *     in an invocation of this fn.
 */

ClRcT
clAmsPeCompInstantiateCallback(
        CL_IN       ClAmsCompT          *comp,
        CL_IN       ClRcT               error)
{
    ClAmsSUT *su;

    ClAmsPresenceStateT compPresenceState;


    AMS_CHECK_COMP ( comp );
    AMS_CHECK_SU ( su = (ClAmsSUT *) comp->config.parentSU.ptr );

    AMS_FUNC_ENTER ( ("Component [%s]\n", comp->config.entity.name.value) );

    /*
     * If component instantiate timer is no longer running, this means the 
     * timeout happened before this callback and this has already been treated 
     * as an error. So ignore the callback.
     */

    /* Will not work for proxied components
    if ( ! clAmsEntityTimerIsRunning(
                (ClAmsEntityT *) comp,
                CL_AMS_COMP_TIMER_INSTANTIATE) )
    {
        AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
            ("Component [%s] instantiate timer has been cleared. Ignoring callback..\n",
            comp->config.entity.name.value));

        return CL_OK;
    }
    */

    /*
     * Is component in appropriate state?
     */

    if ( (comp->status.presenceState != CL_AMS_PRESENCE_STATE_INSTANTIATING ) &&
         (comp->status.presenceState != CL_AMS_PRESENCE_STATE_RESTARTING) )
    {
        AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
            ("Component [%s] is [%s]. \n",
            comp->config.entity.name.value,
            CL_AMS_STRING_P_STATE(comp->status.presenceState)));

        if ((!error) && (comp->status.presenceState != CL_AMS_PRESENCE_STATE_INSTANTIATED ))
        {
            /* Call clAmsPeCompInstantiateError to cleanup the component
             *  which will try to instantiate it again */
          	return clAmsPeCompInstantiateError(comp, CL_AMS_RC(CL_ERR_INVALID_STATE));
        }

        return CL_OK;
    }

    /*
     * Future: Some errors may indicate that no retry is to be attempted.
     * SAF_CLC_NO_RETRY. Modify the switch below, so that if this error
     * is returned, call timeout but make sure it won't try to reinstantiate.
     */

    switch ( comp->config.property )
    {
        case CL_AMS_COMP_PROPERTY_SA_AWARE:
        {
            AMS_CALL ( clAmsEntityTimerStop(
                            (ClAmsEntityT *) comp,
                            CL_AMS_COMP_TIMER_INSTANTIATE) );

            if ( error )
            {
                return clAmsPeCompInstantiateError(comp, error);
            }

            break;
        }

        case CL_AMS_COMP_PROPERTY_PROXIED_PREINSTANTIABLE:
        {
            AMS_CALL ( clAmsEntityTimerStop(
                            (ClAmsEntityT *) comp, 
                            CL_AMS_COMP_TIMER_PROXIEDCOMPINSTANTIATE) );

            if ( error )
            {
                return clAmsPeCompInstantiateError(comp, error);
            }

            break;
        }

        case CL_AMS_COMP_PROPERTY_PROXIED_NON_PREINSTANTIABLE:
        {
            /*
             * The following is just a catchall. Since instantiate callback
             * is called from csi assign callback, any error in the csi
             * set will take the code path to csi assign error and then to
             * component instantiate error. It won't come here.
             */

            if ( error )
            {
                return clAmsPeCompInstantiateError(comp, error);
            }

            break;
        }

        case CL_AMS_COMP_PROPERTY_NON_PROXIED_NON_PREINSTANTIABLE:
        {
            AMS_CALL ( clAmsEntityTimerStop(
                            (ClAmsEntityT *) comp,
                            CL_AMS_COMP_TIMER_INSTANTIATE) );

            if ( error )
            {
                return clAmsPeCompInstantiateError(comp, error);
            }

            break;
        }

        default:
        {
            AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_ERROR,
                ("Error: Invalid Property [%d] for Component [%s]. Exiting..\n",
                 comp->config.property,
                 comp->config.entity.name.value));

            return CL_AMS_RC(CL_AMS_ERR_BAD_CONFIG);
        }
    }

    /*
     * Fall through to here implies that instantiation has succeeded. Pass on 
     * the callback to SU Instantiate or Comp Restart.
     */

    AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
        ("Component [%s] is now instantiated\n",
         comp->config.entity.name.value));

    comp->status.instantiateCount = 0;
    comp->status.instantiateDelayCount = 0;

    compPresenceState = comp->status.presenceState;
    CL_AMS_SET_P_STATE(comp, CL_AMS_PRESENCE_STATE_INSTANTIATED);

    // WORKQUEUE

    if ( compPresenceState == CL_AMS_PRESENCE_STATE_RESTARTING )
    {
        /*
         * A component that has been instantiated may have been restarted and
         * could have assigned CSIs. If this happens, the CSIs need to be
         * assigned back to the component.
         */

        // PROXY: The following if condition may change when other component types
        // are added and verified.

        if ( (comp->config.property == CL_AMS_COMP_PROPERTY_SA_AWARE) ||
             (comp->config.property == CL_AMS_COMP_PROPERTY_PROXIED_PREINSTANTIABLE) )
        {
            AMS_CALL ( clAmsPeCompRestartCallback_Step2(comp, CL_OK) );
        }
    }
    else
    {
        /*
         * The count is incremented when the component is first instantiated, left
         * unchanged through restart cycles and then finally decremented when we
         * know for sure that the component is being terminated.
         */

        su->status.numInstantiatedComp ++;

        AMS_CALL ( clAmsPeSUInstantiateCallback(su, CL_OK) );
    }

    return CL_OK;
}

/*
 * clAmsPeCompInstantiateTimeout
 * -----------------------------
 * This event informs the AMS PE that an instantiate attempt has timed out 
 * (ie failed) for a proxied or nonproxied component.
 */

ClRcT
clAmsPeCompInstantiateTimeout(
        CL_IN ClAmsEntityTimerT  *timer) 
{
    ClAmsCompT *comp;

    AMS_CHECKPTR ( !timer );
    AMS_CHECK_COMP ( comp = (ClAmsCompT *) timer->entity );

#ifdef AMS_EMULATE_RMD_CALLS

#ifdef AMS_TEST_INSTANTIATE_FAILURE_DURING_RESTART
    ClAmsSUT *su = (ClAmsSUT *) comp->config.parentSU.ptr;

    if ( (comp->status.presenceState == CL_AMS_PRESENCE_STATE_INSTANTIATING) &&
         (su ->status.presenceState == CL_AMS_PRESENCE_STATE_INSTANTIATING) )
#endif

    return clAmsPeCompInstantiateCallback(comp, CL_OK);

#endif

    AMS_FUNC_ENTER ( ("Component [%s]\n",comp->config.entity.name.value) );

    if ( (comp->status.presenceState != CL_AMS_PRESENCE_STATE_INSTANTIATING) &&
         (comp->status.presenceState != CL_AMS_PRESENCE_STATE_RESTARTING) )
    {
        AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
            ("Component [%s] is [%s]. Ignoring instantiate timeout..\n",
            comp->config.entity.name.value,
            CL_AMS_STRING_P_STATE(comp->status.presenceState)));

        return CL_OK;
    }

    return clAmsPeCompInstantiateError(comp, CL_AMS_RC(CL_ERR_TIMEOUT));
}

/*
 * clAmsPeCompInstantiateError
 * ---------------------------
 * This fn is called when an error takes place at any time in the instantiation
 * of a component. The next action is to cleanup, which will try to instantiate
 * the component again.
 */

ClRcT
clAmsPeCompInstantiateError(
        CL_IN ClAmsCompT *comp,
        CL_IN ClRcT error)
{
    AMS_FUNC_ENTER ( ("Component [%s]\n",comp->config.entity.name.value) );

    AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_ERROR,
        ("Component [%s] instantiate error [0x%x]. Will cleanup\n",
         comp->config.entity.name.value, error)); 

    AMS_CALL ( clAmsEntityTimerStop(
                    (ClAmsEntityT *) comp, 
                    CL_AMS_COMP_TIMER_INSTANTIATE) );

    return clAmsPeCompCleanup(comp);
}

/*
 * clAmsPeCompProxiedCompInstantiateTimeout
 * ----------------------------------------
 * This fn is called as a result of a timeout of the proxied preinstantiable
 * component instantiate timer. 
 */

ClRcT
clAmsPeCompProxiedCompInstantiateTimeout(
        CL_IN ClAmsEntityTimerT  *timer) 
{
    ClAmsCompT *comp;

    AMS_CHECKPTR ( !timer );
    AMS_CHECK_COMP ( comp = (ClAmsCompT *) timer->entity );

#ifdef AMS_EMULATE_RMD_CALLS
    return clAmsPeCompInstantiateCallback(comp, CL_OK);
#endif

    AMS_FUNC_ENTER ( ("Component [%s]\n",comp->config.entity.name.value) );

    if ( (comp->status.presenceState != CL_AMS_PRESENCE_STATE_INSTANTIATING) &&
         (comp->status.presenceState != CL_AMS_PRESENCE_STATE_RESTARTING) )
    {
        AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("Component [%s] is [%s]. Ignoring instantiate timeout..\n",
            comp->config.entity.name.value,
            CL_AMS_STRING_P_STATE(comp->status.presenceState)));

        return CL_OK;
    }

    return clAmsPeCompProxiedCompInstantiateError(comp, CL_AMS_RC(CL_ERR_TIMEOUT));
}

/*
 * clAmsPeCompProxiedCompInstantiateError
 * --------------------------------------
 * This fn is called when an error takes place at any time in the instantiation
 * of a proxied component. The next action is to cleanup, which will try to 
 * instantiate the component again.
 */

ClRcT
clAmsPeCompProxiedCompInstantiateError(
        CL_IN ClAmsCompT *comp,
        CL_IN ClRcT error)
{
    AMS_FUNC_ENTER ( ("Component [%s]\n",comp->config.entity.name.value) );

    AMS_CALL ( clAmsEntityTimerStop(
                    (ClAmsEntityT *) comp, 
                    CL_AMS_COMP_TIMER_PROXIEDCOMPINSTANTIATE) );

    AMS_CALL ( clAmsInvocationDeleteAll(comp) );

    return clAmsPeCompInstantiateError(comp, error);
}

/*
 * clAmsPeCompShutdown
 * --------------------
 * Terminate or cleanup a component, it is assumed that any work assignment to this
 * component has been appropriately removed before this fn is called.
 * This function is invoked from su terminate and hence the terminate callback would be called
 * if the component is cleaned up if its unreachable
 */

ClRcT
clAmsPeCompShutdown(
                    CL_IN       ClAmsCompT        *comp, 
                    CL_OUT      ClBoolT *pResponsePending)
{
    ClAmsSUT *su;
    ClAmsNodeT *node;
    ClRcT error = CL_OK;
    ClUint32T switchoverMode = CL_AMS_ENTITY_SWITCHOVER_IMMEDIATE;
    ClAmsEntityTimerTypeT timerType = CL_AMS_COMP_TIMER_TERMINATE;
    ClRcT (*compShutdown)(ClNameT *, ClNameT *) = _clAmsSAComponentTerminate;
    ClRcT (*compShutdownCallback)(ClAmsCompT *, ClRcT ) = NULL;
    ClRcT (*compTimerStop)(ClAmsEntityT *, ClAmsEntityTimerTypeT) = NULL;

    if(pResponsePending)
        *pResponsePending = CL_FALSE;

    AMS_CHECK_COMP ( comp );
    AMS_CHECK_SU ( su = (ClAmsSUT *) comp->config.parentSU.ptr );
    AMS_CHECK_NODE ( node = (ClAmsNodeT *) su->config.parentNode.ptr );

    AMS_FUNC_ENTER ( ("Component [%s]\n",comp->config.entity.name.value) );

    /*
     * Precheck: Is component in appropriate state to be terminated?
     */

    if ( comp->status.presenceState == CL_AMS_PRESENCE_STATE_UNINSTANTIATED )
    {
        if(CL_FALSE == su->config.isPreinstantiable)
        {
            CL_AMS_RESET_EPOCH(comp);
            /*
             * Fake a call to compTerminate callback as comp. is already
             * dead to get the SU terminate stats right incase of a shutdown.
             */
            CL_AMS_SET_P_STATE(comp,CL_AMS_PRESENCE_STATE_TERMINATING);
        }

        return clAmsPeCompTerminateCallback(comp, CL_OK);
    }

    if ( (comp->status.presenceState != CL_AMS_PRESENCE_STATE_INSTANTIATED) &&
         (comp->status.presenceState != CL_AMS_PRESENCE_STATE_INSTANTIATING) &&
         (comp->status.presenceState != CL_AMS_PRESENCE_STATE_RESTARTING) )
    {
        AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
            ("Component [%s] is [%s]. Ignoring terminate request..\n",
            comp->config.entity.name.value,
            CL_AMS_STRING_P_STATE(comp->status.presenceState)));

        return CL_OK;
    }

    CL_AMS_RESET_EPOCH(comp);

    AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG,CL_LOG_SEV_INFO,
        ("Terminating Component [%s] in Presence State [%s]\n",
         comp->config.entity.name.value,
         CL_AMS_STRING_P_STATE(comp->status.presenceState)) ); 

    if ( (comp->status.presenceState == CL_AMS_PRESENCE_STATE_INSTANTIATING) ||
         (comp->status.presenceState == CL_AMS_PRESENCE_STATE_RESTARTING) )
    {
        AMS_CALL (clAmsEntityTimerStop((ClAmsEntityT *) comp, CL_AMS_COMP_TIMER_INSTANTIATE));
        AMS_CALL (clAmsEntityTimerStop((ClAmsEntityT *) comp, CL_AMS_COMP_TIMER_INSTANTIATEDELAY));
    }

    if ( comp->status.presenceState != CL_AMS_PRESENCE_STATE_RESTARTING )
    {
        CL_AMS_SET_P_STATE(comp, CL_AMS_PRESENCE_STATE_TERMINATING);
    }
    
    if ( node->status.isClusterMember == CL_AMS_NODE_IS_NOT_CLUSTER_MEMBER
         &&
         node->config.isASPAware
         )
    {
        return clAmsPeCompTerminateCallback(comp, CL_OK);
    }

    if(!node->config.isASPAware 
       &&
       (comp->config.property == CL_AMS_COMP_PROPERTY_SA_AWARE))
    {
        return clAmsPeCompTerminateCallback(comp, CL_OK);
    }

    clAmsPeCompComputeSwitchoverMode(comp, &switchoverMode);

    if(switchoverMode == CL_AMS_ENTITY_SWITCHOVER_FAST)
    {
        timerType = CL_AMS_COMP_TIMER_CLEANUP;
        compShutdown = _clAmsSAComponentCleanup;
        compShutdownCallback = clAmsPeCompTerminateCallback;
        compTimerStop = clAmsEntityTimerStop;
    }
    /*
     * Terminate the component taking its property into account.
     */

    switch ( comp->config.property )
    {
        case CL_AMS_COMP_PROPERTY_SA_AWARE:
        {
            /*
             * Invoke the terminate callback for the component which results in
             * a terminate call to the SA aware component.
             */

            AMS_CALL ( clAmsEntityTimerStart(
                            (ClAmsEntityT*) comp,
                            timerType) ); 
            
#ifdef AMS_CPM_INTEGRATION
            error = (*compShutdown)(
                                    &comp->config.entity.name,
                                    &comp->config.entity.name);
#endif
            if(compTimerStop)
                AMS_CALL ( (*compTimerStop)(
                                            (ClAmsEntityT*)comp,
                                            timerType) );
            if ( error )
            {
                if(pResponsePending)
                    *pResponsePending = CL_TRUE;

                return clAmsPeCompTerminateError(comp, error);
            }
            else
            {
                if(su->status.numPIComp > 0)
                    --su->status.numPIComp;

                if(compShutdownCallback)
                {
                    return (*compShutdownCallback)(comp, CL_OK);
                }
                else
                {
                    if(pResponsePending)
                        *pResponsePending = CL_TRUE;
                }
            }

            break;
        }

        case CL_AMS_COMP_PROPERTY_PROXIED_PREINSTANTIABLE:
        {
            /*
             * The proxy for the component is asked to terminate the 
             * component. When the component is stopped, its proxy will 
             * send a response to the AMF.
             */

            if ( !comp->status.proxyComp )
            {
                AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
                    ("Component [%s] has no proxy. Marking component terminated\n",
                     comp->config.entity.name.value));

                return clAmsPeCompTerminateCallback(comp, CL_OK);
            }

            AMS_CALL ( clAmsEntityTimerStart(
                            (ClAmsEntityT*) comp,
                            CL_AMS_COMP_TIMER_TERMINATE) );
            
#ifdef AMS_CPM_INTEGRATION

            ClAmsCompT *proxy = (ClAmsCompT *) comp->status.proxyComp;

            error = _clAmsSAComponentTerminate(
                                    &comp->config.entity.name,
                                    &proxy->config.entity.name);
#endif

            if(pResponsePending)
                *pResponsePending = CL_TRUE;

            if ( error )
            {
                return clAmsPeCompTerminateError(comp, error);
            }

            if(su->status.numPIComp > 0)
                --su->status.numPIComp;

            break;
        }

        case CL_AMS_COMP_PROPERTY_PROXIED_NON_PREINSTANTIABLE:
        {
            ClAmsCompCSIRefT *csiRef = NULL;
            ClAmsCSIT *csi = NULL;
            ClAmsCSIDescriptorT *csiDescriptor = NULL;
            ClNameT activeCompName = {0};
            ClInvocationT invocation = 0;

            if ( !comp->status.proxyComp )
            {
                AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
                    ("Component [%s] has no proxy. Marking component terminated\n",
                     comp->config.entity.name.value));

                return clAmsPeCompTerminateCallback(comp, CL_OK);
            }

            csiRef = (ClAmsCompCSIRefT *)clAmsEntityListGetFirst(&comp->status.csiList);

            if ( !csiRef )
            {
                AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
                    ("Component [%s] has no CSI assignment. Nothing to terminate\n",
                     comp->config.entity.name.value));

                return CL_OK;
            }

            csi = (ClAmsCSIT *) csiRef->entityRef.ptr;

            activeCompName.length = 0;

            AMS_CALL ( clAmsCSIMarshalCSIDescriptor(
                            &csiDescriptor,
                            csi,
                            CL_AMS_CSI_FLAG_TARGET_ONE,
                            csiRef->haState,
                            activeCompName,
                            CL_AMS_CSI_QUIESCED,
                            0) );
            
            AMS_CALL ( clAmsInvocationCreate(
                            CL_AMS_CSI_RMV_CALLBACK,
                            comp,
                            csi,
                            &invocation) );

            AMS_CALL ( clAmsEntityTimerStart(
                            (ClAmsEntityT *) comp,
                            CL_AMS_COMP_TIMER_CSIREMOVE) );

#ifdef AMS_CPM_INTEGRATION
 
            ClAmsCompT *proxy = (ClAmsCompT *) comp->status.proxyComp;

            error = _clAmsSACSIRemove(
                            &comp->config.entity.name,
                            &proxy->config.entity.name,
                            invocation,
                            *csiDescriptor);

#endif
            
            if(pResponsePending)
                *pResponsePending = CL_TRUE;

            clAmsFreeMemory(csiDescriptor->csiAttributeList.attribute);
            clAmsFreeMemory(csiDescriptor);

            if ( error )
            {
                AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_ERROR,
                    ("Component [%s] terminate returned Error [0x%x]\n",
                     comp->config.entity.name.value, error)); 
                
                return clAmsPeCompRemoveCSIError(
                            comp, 
                            error);
            }
            else
            {
                if(su->status.numPIComp > 0)
                    --su->status.numPIComp;
            }

#ifdef AMS_CPM_INTEGRATION
 
            _clAmsSAMarshalPGTrackDispatch(
                            csi,
                            comp,
                            CL_AMS_PG_REMOVED);

#endif

            break;
        }

        case CL_AMS_COMP_PROPERTY_NON_PROXIED_NON_PREINSTANTIABLE:
        {
            /*
             * Invoke the terminate command for the component. No callback
             * is expected from the component, hence we call it here.
             */

            AMS_CALL ( clAmsEntityTimerStart(
                            (ClAmsEntityT*) comp,
                            timerType) );
            
#ifdef AMS_CPM_INTEGRATION

            error = (*compShutdown)(
                                    &comp->config.entity.name,
                                    &comp->config.entity.name);

#endif
            if(compTimerStop)
                AMS_CALL ( (*compTimerStop) ((ClAmsEntityT*)comp,
                                             timerType) );

            if ( error )
            {
                if(pResponsePending)
                    *pResponsePending = CL_TRUE;

                return clAmsPeCompTerminateError(comp, error);
            }

            if(su->status.numPIComp > 0)
                --su->status.numPIComp;

            return clAmsPeCompTerminateCallback(comp, CL_OK);
        }
        break;

        default:
        {
            /*
             * Error, unknown component type
             */

            AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_ERROR,
                ("Error: Invalid Property [%d] for Component [%s]. Exiting..\n",
                 comp->config.property,
                 comp->config.entity.name.value));

            return CL_AMS_RC(CL_AMS_ERR_BAD_CONFIG);
        }
    }

    return CL_OK;
}

/*
 * clAmsPeCompTerminate
 * --------------------
 * Terminate a component, it is assumed that any work assignment to this
 * component has been appropriately removed before this fn is called.
 */

ClRcT
clAmsPeCompTerminate(
        CL_IN       ClAmsCompT        *comp)
{
    ClAmsSUT *su;
    ClAmsNodeT *node;
    ClRcT error = CL_OK;

    AMS_CHECK_COMP ( comp );
    AMS_CHECK_SU ( su = (ClAmsSUT *) comp->config.parentSU.ptr );
    AMS_CHECK_NODE ( node = (ClAmsNodeT *) su->config.parentNode.ptr );

    AMS_FUNC_ENTER ( ("Component [%s]\n",comp->config.entity.name.value) );

    /*
     * Precheck: Is component in appropriate state to be terminated?
     */

    if ( comp->status.presenceState == CL_AMS_PRESENCE_STATE_UNINSTANTIATED )
    {
        if(CL_FALSE == su->config.isPreinstantiable)
        {
            CL_AMS_RESET_EPOCH(comp);
            /*
             * Fake a call to compTerminate callback as comp. is already
             * dead to get the SU terminate stats right incase of a shutdown.
             */
            CL_AMS_SET_P_STATE(comp,CL_AMS_PRESENCE_STATE_TERMINATING);
        }
        return clAmsPeCompTerminateCallback(comp, CL_OK);
    }

    if ( (comp->status.presenceState != CL_AMS_PRESENCE_STATE_INSTANTIATED) &&
         (comp->status.presenceState != CL_AMS_PRESENCE_STATE_INSTANTIATING) &&
         (comp->status.presenceState != CL_AMS_PRESENCE_STATE_RESTARTING) )
    {
        AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
            ("Component [%s] is [%s]. Ignoring terminate request..\n",
            comp->config.entity.name.value,
            CL_AMS_STRING_P_STATE(comp->status.presenceState)));

        return CL_OK;
    }

    CL_AMS_RESET_EPOCH(comp);

    AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
        ("Terminating Component [%s] in Presence State [%s]\n",
         comp->config.entity.name.value,
         CL_AMS_STRING_P_STATE(comp->status.presenceState)) ); 

    if ( (comp->status.presenceState == CL_AMS_PRESENCE_STATE_INSTANTIATING) ||
         (comp->status.presenceState == CL_AMS_PRESENCE_STATE_RESTARTING) )
    {
        AMS_CALL (clAmsEntityTimerStop((ClAmsEntityT *) comp, CL_AMS_COMP_TIMER_INSTANTIATE));
        AMS_CALL (clAmsEntityTimerStop((ClAmsEntityT *) comp, CL_AMS_COMP_TIMER_INSTANTIATEDELAY));
    }

    if ( comp->status.presenceState != CL_AMS_PRESENCE_STATE_RESTARTING )
    {
        CL_AMS_SET_P_STATE(comp, CL_AMS_PRESENCE_STATE_TERMINATING);
    }
    
    if ( node->status.isClusterMember == CL_AMS_NODE_IS_NOT_CLUSTER_MEMBER
         &&
         node->config.isASPAware
         )
    {
        return clAmsPeCompTerminateCallback(comp, CL_OK);
    }

    if(!node->config.isASPAware 
       &&
       (comp->config.property == CL_AMS_COMP_PROPERTY_SA_AWARE))
    {
        return clAmsPeCompTerminateCallback(comp, CL_OK);
    }

    /*
     * Terminate the component taking its property into account.
     */

    switch ( comp->config.property )
    {
        case CL_AMS_COMP_PROPERTY_SA_AWARE:
        {
            /*
             * Invoke the terminate callback for the component which results in
             * a terminate call to the SA aware component.
             */

            AMS_CALL ( clAmsEntityTimerStart(
                            (ClAmsEntityT*) comp,
                            CL_AMS_COMP_TIMER_TERMINATE) ); 
            
#ifdef AMS_CPM_INTEGRATION

            error = _clAmsSAComponentTerminate(
                            &comp->config.entity.name,
                            &comp->config.entity.name);

#endif

            if ( error )
            {
                return clAmsPeCompTerminateError(comp, error);
            }

            break;
        }

        case CL_AMS_COMP_PROPERTY_PROXIED_PREINSTANTIABLE:
        {
            /*
             * The proxy for the component is asked to terminate the 
             * component. When the component is stopped, its proxy will 
             * send a response to the AMF.
             */

            if ( !comp->status.proxyComp )
            {
                AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
                    ("Component [%s] has no proxy. Marking component terminated\n",
                     comp->config.entity.name.value));

                return clAmsPeCompTerminateCallback(comp, CL_OK);
            }

            AMS_CALL ( clAmsEntityTimerStart(
                            (ClAmsEntityT*) comp,
                            CL_AMS_COMP_TIMER_TERMINATE) );

#ifdef AMS_CPM_INTEGRATION

            ClAmsCompT *proxy = (ClAmsCompT *) comp->status.proxyComp;

            error = _clAmsSAComponentTerminate(
                            &comp->config.entity.name,
                            &proxy->config.entity.name);

#endif

            if ( error )
            {
                return clAmsPeCompTerminateError(comp, error);
            }

            break;
        }

        case CL_AMS_COMP_PROPERTY_PROXIED_NON_PREINSTANTIABLE:
        {
            if ( !comp->status.proxyComp )
            {
                AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
                    ("Component [%s] has no proxy. Marking component terminated\n",
                     comp->config.entity.name.value));

                return clAmsPeCompTerminateCallback(comp, CL_OK);
            }

            ClAmsCompCSIRefT *csiRef;
            ClAmsCSIT *csi;

            csiRef = (ClAmsCompCSIRefT *)clAmsEntityListGetFirst(&comp->status.csiList);

            if ( !csiRef )
            {
                AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
                    ("Component [%s] has no CSI assignment. Nothing to terminate\n",
                     comp->config.entity.name.value));

                return CL_OK;
            }

            csi = (ClAmsCSIT *) csiRef->entityRef.ptr;

            ClAmsCSIDescriptorT *csiDescriptor;
            ClNameT activeCompName = {0};
            ClInvocationT invocation = 0;

            activeCompName.length = 0;

            AMS_CALL ( clAmsCSIMarshalCSIDescriptor(
                            &csiDescriptor,
                            csi,
                            CL_AMS_CSI_FLAG_TARGET_ONE,
                            csiRef->haState,
                            activeCompName,
                            CL_AMS_CSI_QUIESCED,
                            0) );
            
            AMS_CALL ( clAmsInvocationCreate(
                            CL_AMS_CSI_RMV_CALLBACK,
                            comp,
                            csi,
                            &invocation) );

            AMS_CALL ( clAmsEntityTimerStart(
                            (ClAmsEntityT *) comp,
                            CL_AMS_COMP_TIMER_CSIREMOVE) );

#ifdef AMS_CPM_INTEGRATION
 
            ClAmsCompT *proxy = (ClAmsCompT *) comp->status.proxyComp;

            error = _clAmsSACSIRemove(
                            &comp->config.entity.name,
                            &proxy->config.entity.name,
                            invocation,
                            *csiDescriptor);

#endif

            clAmsFreeMemory(csiDescriptor->csiAttributeList.attribute);
            clAmsFreeMemory(csiDescriptor);

            if ( error )
            {
                AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_ERROR,
                    ("Component [%s] terminate returned Error [0x%x]\n",
                     comp->config.entity.name.value, error)); 

                return clAmsPeCompRemoveCSIError(
                            comp, 
                            error);
            }

#ifdef AMS_CPM_INTEGRATION
 
            _clAmsSAMarshalPGTrackDispatch(
                            csi,
                            comp,
                            CL_AMS_PG_REMOVED);

#endif

            break;
        }

        case CL_AMS_COMP_PROPERTY_NON_PROXIED_NON_PREINSTANTIABLE:
        {
            /*
             * Invoke the terminate command for the component. No callback
             * is expected from the component, hence we call it here.
             */

            AMS_CALL ( clAmsEntityTimerStart(
                            (ClAmsEntityT*) comp,
                            CL_AMS_COMP_TIMER_TERMINATE) ); 
            
#ifdef AMS_CPM_INTEGRATION

            error = _clAmsSAComponentTerminate(
                            &comp->config.entity.name,
                            &comp->config.entity.name);

#endif

            if ( error )
            {
                return clAmsPeCompTerminateError(comp, error);
            }

            return clAmsPeCompTerminateCallback(comp, CL_OK);
        }

        default:
        {
            /*
             * Error, unknown component type
             */

            AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_ERROR,
                ("Error: Invalid Property [%d] for Component [%s]. Exiting..\n",
                 comp->config.property,
                 comp->config.entity.name.value));

            return CL_AMS_RC(CL_AMS_ERR_BAD_CONFIG);
        }
    }

    return CL_OK;
}

/*
 * clAmsPeCompTerminateCallback
 * ----------------------------
 * This event informs the AMS PE about the status of a previous
 * attempt at terminating a component.
 */

ClRcT
clAmsPeCompTerminateCallback(
        CL_IN       ClAmsCompT          *comp,
        CL_IN       ClRcT               error)
{
    ClAmsSUT *su;
    ClRcT rc = CL_OK;

    AMS_CHECK_COMP ( comp );
    AMS_CHECK_SU ( su = (ClAmsSUT *) comp->config.parentSU.ptr );

    AMS_FUNC_ENTER ( ("Component [%s]\n",comp->config.entity.name.value) );

    /*
     * If component terminate timer is no longer running, this means the 
     * timeout happened before this callback and this has already been 
     * treated as an error. So ignore the callback.
     */

    /* will not work for proxied np components
    if ( ! clAmsEntityTimerIsRunning(
                (ClAmsEntityT *) comp,
                CL_AMS_COMP_TIMER_TERMINATE) )
    {
        AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
            ("Component [%s] terminate timer has been cleared. Ignoring callback..\n",
            comp->config.entity.name.value));

        return CL_OK;
    }
    */

    /*
     * Process terminate callback (component unregister) based on presence state
     */

    AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
        ("Terminate Callback (Unregister) for Component [%s] in Presence State [%s]\n",
        comp->config.entity.name.value,
        CL_AMS_STRING_P_STATE(comp->status.presenceState)) );

    AMS_CALL ( clAmsEntityTimerStop(
                    (ClAmsEntityT *) comp,
                    CL_AMS_COMP_TIMER_TERMINATE) );

    AMS_CALL ( clAmsInvocationDeleteAll(comp) );

    if(comp->status.presenceState != CL_AMS_PRESENCE_STATE_RESTARTING)
    {
        ClAmsCompT *proxyComp = NULL;
        if( (proxyComp = (ClAmsCompT*)comp->status.proxyComp) )
        {
            if(proxyComp->status.presenceState != CL_AMS_PRESENCE_STATE_INSTANTIATED
               ||
               proxyComp->status.operState == CL_AMS_OPER_STATE_DISABLED
               ||
               proxyComp->status.readinessState != CL_AMS_READINESS_STATE_INSERVICE)
            {
                clLogDebug("COMP", "TERMINATE", "Resetting proxy [%s] component for proxied [%s]",
                           comp->status.proxyComp->name.value,
                           comp->config.entity.name.value);
                comp->status.proxyComp = NULL;
            }
        }
    }

    if ( comp->status.presenceState == CL_AMS_PRESENCE_STATE_RESTARTING )
    {
         if ( clAmsPeSUIsInstantiable((ClAmsSUT *)comp->config.parentSU.ptr) != CL_OK )
         {
            CL_AMS_SET_P_STATE(comp, CL_AMS_PRESENCE_STATE_TERMINATING);
         }
    }

    /*
     * Is component in appropriate state?
     */

    switch ( comp->status.presenceState )
    {
        case CL_AMS_PRESENCE_STATE_TERMINATING:
        {
            if ( error )
            {
                return clAmsPeCompTerminateError(comp, error);
            }

            if ( su->status.numInstantiatedComp )
            {
                su->status.numInstantiatedComp --;
            }

            CL_AMS_SET_P_STATE(comp, CL_AMS_PRESENCE_STATE_UNINSTANTIATED);

            comp->status.instantiateCount = 0;
            comp->status.instantiateDelayCount = 0;

            break;
        }

        case CL_AMS_PRESENCE_STATE_RESTARTING:
        {
            if ( error )
            {
                return clAmsPeCompTerminateError(comp, error);
            }

            break;
        }

        case CL_AMS_PRESENCE_STATE_INSTANTIATING:
        case CL_AMS_PRESENCE_STATE_INSTANTIATED:
        {
            /*
             * Unsolicited unregister is treated as a fault for SA Aware
             * components. For proxied components, it is interpreted to
             * mean that the proxy is no longer proxying the proxied.
             */

            if ( comp->config.property == CL_AMS_COMP_PROPERTY_SA_AWARE )
            {
                ClAmsLocalRecoveryT recovery = CL_AMS_RECOVERY_NO_RECOMMENDATION;
                ClUint32T escalation = clAmsPeEntityComputeFaultEscalation((ClAmsEntityT*) comp);
                clAmsFaultQueueAdd((ClAmsEntityT*)comp);
                rc = clAmsPeCompFaultReport(comp, &recovery, &escalation);
                clAmsFaultQueueDelete((ClAmsEntityT*)comp);
                return rc;
            }
            else
            {
                // TODO Mark the component as having no proxy and 
                // search for a new proxy.
            }

            break;
        }

        default:
        {
            return CL_OK;
        }
    }

    /*
     * Fall through to here means component was/is now terminated.
     */

    AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
        ("Component [%s] is now terminated\n",
         comp->config.entity.name.value));

    // WORKQUEUE

    if ( comp->status.presenceState == CL_AMS_PRESENCE_STATE_RESTARTING )
    {
        return clAmsPeCompRestartCallback_Step1(comp, CL_OK);
    }

    /* Calling terminal if su->status.numInstantiatedComp == 0 */
    if ( (su->status.presenceState == CL_AMS_PRESENCE_STATE_TERMINATING) ||
         (su->status.presenceState == CL_AMS_PRESENCE_STATE_RESTARTING) ||
         !su->status.numInstantiatedComp)
    {
        AMS_CALL ( clAmsPeSUTerminateCallback(su, CL_OK) );
    }

    return CL_OK;
}

/*
 * clAmsPeCompTerminateTimeout
 * ---------------------------
 * This event informs the AMS PE that a terminate attempt has timed out 
 */

ClRcT
clAmsPeCompTerminateTimeout(
        CL_IN ClAmsEntityTimerT  *timer) 
{
    ClAmsCompT *comp;

    AMS_CHECKPTR ( !timer );
    AMS_CHECK_COMP ( comp = (ClAmsCompT *) timer->entity );

#ifdef AMS_EMULATE_RMD_CALLS
    return clAmsPeCompTerminateCallback(comp, CL_OK);
#endif

    AMS_FUNC_ENTER ( ("Component [%s]\n",comp->config.entity.name.value) );

    if ( (comp->status.presenceState != CL_AMS_PRESENCE_STATE_TERMINATING) &&
         (comp->status.presenceState != CL_AMS_PRESENCE_STATE_RESTARTING) )
    {
        AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
            ("Component [%s] is not in terminating state. Ignoring timeout..\n",
            comp->config.entity.name.value));

        return CL_OK;
    }

    return clAmsPeCompTerminateError(comp, CL_AMS_RC(CL_ERR_TIMEOUT) );
}

/*
 * clAmsPeCompTerminateError
 * -------------------------
 * This function is invoked when an error takes place at any state in the
 * termination of a component. The next action is to do a cleanup.
 */

ClRcT
clAmsPeCompTerminateError(
        CL_IN ClAmsCompT *comp,
        CL_IN ClRcT error)
{
    ClAmsSUT *su =  NULL;

    AMS_FUNC_ENTER ( ("Component [%s]\n",comp->config.entity.name.value) );

    /* comp should always have a SU but its not necessary at this level so why require it? */
    /* AMS_CHECK_SU ( su = (ClAmsSUT*)comp->config.parentSU.ptr ); */

    AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_ERROR,("Component [%s] terminate error [0x%x]. Will cleanup\n",comp->config.entity.name.value,error));

    clAmsEntityTimerStop((ClAmsEntityT *) comp,CL_AMS_COMP_TIMER_TERMINATE);

    clAmsInvocationDeleteAll(comp);

    /*
     * If SU is in the middle of terminating, then cleanup all the components
     * as components higher up in the instantiation wont be cleaned up
     * if lower level comps fail to respond to terminate.
     */

    if(su && (su->status.presenceState == CL_AMS_PRESENCE_STATE_TERMINATING))
    {
        return clAmsPeSUCleanup(su);
    }
    return clAmsPeCompCleanup(comp);
}

/*
 * clAmsPeCompCleanup
 * ------------------
 * This fn is called to clean up the resources used by a component when
 * the component instantiation or termination fails. The cleanup action
 * is synchronous for saf aware and npnp components and asynchronous for
 * proxied components.
 */

ClRcT
clAmsPeCompCleanup(
        CL_IN       ClAmsCompT        *comp)
{
    ClAmsSUT *su;
    ClAmsNodeT *node;
    ClRcT error = CL_OK;

    AMS_CHECK_COMP ( comp );
    AMS_CHECK_SU ( su = (ClAmsSUT *) comp->config.parentSU.ptr );
    AMS_CHECK_NODE ( node = (ClAmsNodeT *) su->config.parentNode.ptr );

    AMS_FUNC_ENTER ( ("Component [%s]\n", comp->config.entity.name.value) );

    /*
     * Ensure we are not in the middle of a cleanup for this component.
     */

    if ( comp->status.timers.cleanup.count )
    {
        AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_ERROR,
            ("Component [%s]'s cleanup timer is running. Ignoring cleanup request..\n",
             comp->config.entity.name.value));

        return CL_OK;
    }

    AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG,CL_LOG_SEV_INFO,
        ("Cleaning up Component [%s] in Presence State [%s]\n",
         comp->config.entity.name.value,
         CL_AMS_STRING_P_STATE(comp->status.presenceState)) ); 

    CL_AMS_RESET_EPOCH(comp);

    if ( node->status.isClusterMember != CL_AMS_NODE_IS_CLUSTER_MEMBER
         &&
         node->config.isASPAware)
    {
        if(node->status.isClusterMember == CL_AMS_NODE_IS_NOT_CLUSTER_MEMBER
           ||
           clAmsPeCheckNodeHasLeft(node))
        {
            return clAmsPeCompCleanupCallback(comp, CL_OK);
        }
    }

    if(!node->config.isASPAware 
       &&
       comp->config.property == CL_AMS_COMP_PROPERTY_SA_AWARE)
    {
        return clAmsPeCompCleanupCallback(comp, CL_OK);
    }
       

    /*
     * Invoke appropriate cleanup action depending on component type.
     */

    switch ( comp->config.property )
    {
        case CL_AMS_COMP_PROPERTY_SA_AWARE:
        {
            /*
             * Invoke the cleanup command for the component. We expect
             * no callback in this case, only a timeout if the cleanup
             * command takes too long.
             */

            AMS_CALL ( clAmsEntityTimerStart(
                            (ClAmsEntityT*) comp,
                            CL_AMS_COMP_TIMER_CLEANUP) ); 
            
#ifdef AMS_CPM_INTEGRATION

            error = _clAmsSAComponentCleanup(
                            &comp->config.entity.name,
                            &comp->config.entity.name);

            if(CL_COMP_UNREACHABLE_ERROR(error))
                error = CL_OK;

            if ( error )
            {
                AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
                    ("Component [%s] cleanup returned Error [0x%x]\n",
                     comp->config.entity.name.value, error) ) 

                return clAmsPeCompCleanupError(comp, error);
            }
#endif

            return clAmsPeCompCleanupCallback(comp, CL_OK);
        }

        case CL_AMS_COMP_PROPERTY_NON_PROXIED_NON_PREINSTANTIABLE:
        {
            /*
             * Invoke the cleanup command for the component. We expect
             * no callback in this case, only a timeout if the cleanup
             * command takes too long.
             */

            AMS_CALL ( clAmsEntityTimerStart(
                            (ClAmsEntityT*) comp,
                            CL_AMS_COMP_TIMER_CLEANUP) ); 
            
#ifdef AMS_CPM_INTEGRATION

            error = _clAmsSAComponentCleanup(
                        &comp->config.entity.name,
                        &comp->config.entity.name);

#endif
            if(CL_COMP_UNREACHABLE_ERROR(error))
                error = CL_OK;

            if ( error )
            {
                AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
                    ("Component [%s] cleanup returned Error [0x%x]\n",
                     comp->config.entity.name.value, error) ) 

                return clAmsPeCompCleanupError(comp, error);
            }

            return clAmsPeCompCleanupCallback(comp, CL_OK);
        }

        case CL_AMS_COMP_PROPERTY_PROXIED_PREINSTANTIABLE:
        case CL_AMS_COMP_PROPERTY_PROXIED_NON_PREINSTANTIABLE:
        {
            /*
             * The proxy for the component is asked to cleanup the 
             * component. When the component is stopped, its proxy will 
             * send a response to the AMF, so a callback is expected. 
             */

            if ( !comp->status.proxyComp )
            {
                AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
                    ("Component [%s] has no proxy. Marking component cleaned up\n",
                     comp->config.entity.name.value));

                return clAmsPeCompProxiedCompCleanupCallback(comp, CL_OK);
            }

            if( comp->status.presenceState == CL_AMS_PRESENCE_STATE_UNINSTANTIATED)
            {
                AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
                    ("Component [%s] already uninstantiated. Marking component cleaned up\n",
                     comp->config.entity.name.value));
                return clAmsPeCompProxiedCompCleanupCallback(comp, CL_OK);
            }

            AMS_CALL ( clAmsEntityTimerStart(
                            (ClAmsEntityT*) comp,
                            CL_AMS_COMP_TIMER_PROXIEDCOMPCLEANUP) );

#ifdef AMS_CPM_INTEGRATION
            ClAmsCompT *proxy = (ClAmsCompT *) comp->status.proxyComp;

            error = _clAmsSAComponentCleanup(
                            &comp->config.entity.name,
                            &proxy->config.entity.name);
#endif

            if(CL_COMP_UNREACHABLE_ERROR(error))
            {
                return clAmsPeCompProxiedCompCleanupCallback(comp, CL_OK);
            }

            if ( error )
            {
                AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
                    ("Component [%s] cleanup returned Error [0x%x]\n",
                     comp->config.entity.name.value, error) ) 

                return clAmsPeCompProxiedCompCleanupError(comp, error);
            }

            return CL_OK;
        }

        default:
        {
            /*
             * Error, unknown component type
             */

            AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_ERROR,
                ("Error: Invalid Property [%d] for Component [%s]. Exiting..\n",
                 comp->config.property,
                 comp->config.entity.name.value));

            return CL_AMS_RC(CL_AMS_ERR_BAD_CONFIG);
        }
    }

    return CL_OK;
}

/*
 * clAmsPeCompCleanupCallback
 * --------------------------
 * This fn implements the second part of the cleanup process and is directly
 * called from comp cleanup in the case of sa aware and non-proxied components
 * and due to an async callback in the case of proxied components.
 */

ClRcT
clAmsPeCompCleanupCallback(
        CL_IN   ClAmsCompT *comp,
        CL_IN   ClRcT error)
{
    ClAmsSUT *su;

    AMS_CHECK_COMP ( comp );
    AMS_CHECK_SU ( su = (ClAmsSUT *) comp->config.parentSU.ptr );

    AMS_FUNC_ENTER ( ("Component [%s]\n", comp->config.entity.name.value) );

#ifdef AMS_EMULATE_RMD_CALLS

    // for negative testing, uncomment below

    //error = !CL_OK;

#endif
    
    if(comp->status.presenceState != CL_AMS_PRESENCE_STATE_RESTARTING)
    {
        ClAmsCompT *proxyComp = NULL;
        if( (proxyComp = (ClAmsCompT*)comp->status.proxyComp) )
        {
            if(proxyComp->status.presenceState != CL_AMS_PRESENCE_STATE_INSTANTIATED
               ||
               proxyComp->status.operState == CL_AMS_OPER_STATE_DISABLED
               ||
               proxyComp->status.readinessState != CL_AMS_READINESS_STATE_INSERVICE)
            {
                clLogDebug("COMP", "CLEANUP", "Resetting proxy [%s] component for proxied [%s]",
                           comp->status.proxyComp->name.value, comp->config.entity.name.value);
                comp->status.proxyComp = NULL;
            }
        }
    }

    if ( error )
    {
        return clAmsPeCompCleanupError(comp, error);
    }

    AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
        ("Component [%s] is now terminated by cleanup action\n",
         comp->config.entity.name.value));

    AMS_CALL ( clAmsEntityTimerStop(
                    (ClAmsEntityT *) comp,
                    CL_AMS_COMP_TIMER_CLEANUP) );

    /*
     * The count is incremented when the component is first instantiated, left
     * unchanged through restart cycles and then finally decremented when we
     * know for sure that the component is being terminated.
     */

    if ( comp->status.presenceState == CL_AMS_PRESENCE_STATE_RESTARTING )
    {
         if ( clAmsPeSUIsInstantiable((ClAmsSUT *)comp->config.parentSU.ptr) != CL_OK )
         {
            CL_AMS_SET_P_STATE(comp, CL_AMS_PRESENCE_STATE_TERMINATING);
         }
    }

    if ( (comp->status.presenceState == CL_AMS_PRESENCE_STATE_INSTANTIATED) ||
         (comp->status.presenceState == CL_AMS_PRESENCE_STATE_TERMINATING) )
    {
        if ( su->status.numInstantiatedComp )
        {
            su->status.numInstantiatedComp --;
        }
    }

    AMS_ENTITY_LOG(su, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
        ("SU [%s] now has [%d] instantiated + restarting components\n",
         su->config.entity.name.value,
         su->status.numInstantiatedComp));

    // WORKQUEUE

    if ( comp->status.presenceState == CL_AMS_PRESENCE_STATE_RESTARTING )
    {
        return clAmsPeCompRestartCallback_Step1(comp, CL_OK);
    }

    if ( (su->status.presenceState == CL_AMS_PRESENCE_STATE_INSTANTIATED)  ||
         (su->status.presenceState == CL_AMS_PRESENCE_STATE_INSTANTIATING) ||
         (su->status.presenceState == CL_AMS_PRESENCE_STATE_RESTARTING) )
    {
        if ( comp->status.presenceState == CL_AMS_PRESENCE_STATE_INSTANTIATING )
        {
            return clAmsPeCompInstantiate(comp);
        }
    }

    CL_AMS_SET_P_STATE(comp, CL_AMS_PRESENCE_STATE_UNINSTANTIATED);

    comp->status.instantiateCount = 0;
    comp->status.instantiateDelayCount = 0;

    if ( su->status.presenceState != CL_AMS_PRESENCE_STATE_UNINSTANTIATED && !su->status.numInstantiatedComp)
    {
        AMS_CALL ( clAmsPeSUCleanupCallback(su, CL_OK) );
    }

    return CL_OK;
}

/*
 * clAmsPeCompCleanupError
 * -----------------------
 * This fn is called as a result of an error in the clean up of a component. 
 */

ClRcT
clAmsPeCompCleanupError(
        CL_IN   ClAmsCompT *comp,
        CL_IN   ClRcT error)
{
    ClRcT rc = CL_OK;
    ClAmsSUT *su;
    ClAmsLocalRecoveryT recovery;
    ClUint32T escalation = 0;

    AMS_CHECK_COMP ( comp );
    AMS_CHECK_SU ( su = (ClAmsSUT *) comp->config.parentSU.ptr );

    AMS_FUNC_ENTER ( ("Component [%s]\n", comp->config.entity.name.value) );

    AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_ERROR,
        ("Error [0x%x] in cleaning up Component [%s]\n",
         error,
         comp->config.entity.name.value));

    AMS_CALL ( clAmsEntityTimerStop(
                    (ClAmsEntityT *) comp,
                    CL_AMS_COMP_TIMER_CLEANUP) );

    /*
     * The count is incremented when the component is first instantiated, left
     * unchanged through restart cycles and then finally decremented when we
     * know for sure that the component is being terminated.
     */

    if ( (comp->status.presenceState == CL_AMS_PRESENCE_STATE_INSTANTIATED) ||
         (comp->status.presenceState == CL_AMS_PRESENCE_STATE_TERMINATING)  ||
         (comp->status.presenceState == CL_AMS_PRESENCE_STATE_RESTARTING)   )
    {
        if ( su->status.numInstantiatedComp )
        {
            su->status.numInstantiatedComp --;
        }
    }

    AMS_ENTITY_LOG(su, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
        ("SU [%s] now has [%d] instantiated + restarting components\n",
         su->config.entity.name.value,
         su->status.numInstantiatedComp));

    CL_AMS_SET_P_STATE(comp, CL_AMS_PRESENCE_STATE_TERMINATION_FAILED);
    CL_AMS_SET_O_STATE(comp, CL_AMS_OPER_STATE_DISABLED);

    /*
     * If the comp cleanup is due to an SU event, report it back.
     */

    if ( su->status.presenceState != CL_AMS_PRESENCE_STATE_UNINSTANTIATED )
    {
        AMS_CALL ( clAmsPeSUCleanupError(su, error) );
    }

    /*
     * Report a fault on the component. The recovery action cannot be restart
     * so pick the lowest possible escalated recovery above restart.
     */

    recovery = comp->config.nodeRebootCleanupFail ? 
                        CL_AMS_RECOVERY_NODE_FAILOVER: 
                        CL_AMS_RECOVERY_COMP_FAILOVER;

    /*
     * Don't raise the fault again if we are already inside a fault
    */
    if(recovery != comp->status.recovery)
    {
        escalation = clAmsPeEntityComputeFaultEscalation((ClAmsEntityT*) comp);

        clAmsFaultQueueAdd((ClAmsEntityT*)comp);

        rc = clAmsPeCompFaultReport(comp, &recovery, &escalation);

        clAmsFaultQueueDelete((ClAmsEntityT*)comp);
    }

    return rc;
}

/*
 * clAmsPeCompCleanupTimeout
 * -------------------------
 * This event informs the AMS PE that a previous cleanup attempt has timed out,
 * ie, failed.
 */

ClRcT
clAmsPeCompCleanupTimeout(
        CL_IN ClAmsEntityTimerT  *timer) 
{
    ClAmsCompT *comp;
    ClAmsSUT *su;

    AMS_CHECKPTR ( !timer );
    AMS_CHECK_COMP ( comp = (ClAmsCompT *) timer->entity );
    AMS_CHECK_SU ( su = (ClAmsSUT *) comp->config.parentSU.ptr );

    AMS_FUNC_ENTER ( ("Component [%s]\n", comp->config.entity.name.value) );

    AMS_CALL ( clAmsEntityTimerStop(
                    (ClAmsEntityT *) comp,
                    CL_AMS_COMP_TIMER_CLEANUP) );

    return clAmsPeCompCleanupError(comp, CL_AMS_RC(CL_ERR_TIMEOUT) );
}

/*
 * clAmsPeCompProxiedCompCleanupCallback
 * -------------------------------------
 * This fn implements the second part of the cleanup process for proxied
 * components.  It is called due to an async callback in the case of proxied
 * components.
 */

ClRcT
clAmsPeCompProxiedCompCleanupCallback(
        CL_IN   ClAmsCompT *comp,
        CL_IN   ClRcT error)
{
    AMS_FUNC_ENTER ( ("Component [%s]\n", comp->config.entity.name.value) );

    if ( error )
    {
        return clAmsPeCompProxiedCompCleanupError(comp, error);
    }

    AMS_CALL ( clAmsEntityTimerStop(
                    (ClAmsEntityT *) comp,
                    CL_AMS_COMP_TIMER_PROXIEDCOMPCLEANUP) );

    return clAmsPeCompCleanupCallback(comp, CL_OK);
}

/*
 * clAmsPeCompProxiedCompCleanupError
 * ----------------------------------
 * This fn is called as a result of an error for component cleanup, mainly
 * for proxied components. Cleanup for sa aware components is a sync action.
 */

ClRcT
clAmsPeCompProxiedCompCleanupError(
        CL_IN   ClAmsCompT *comp,
        CL_IN   ClRcT error)
{
    AMS_FUNC_ENTER ( ("Component [%s]\n", comp->config.entity.name.value) );

    AMS_CALL ( clAmsEntityTimerStop(
                    (ClAmsEntityT *) comp,
                    CL_AMS_COMP_TIMER_PROXIEDCOMPCLEANUP) );

    /*
     * Future: We may want to add a step here to run a cleanup CLI as
     * for SAF aware components and only if that fails then declare
     * an error.
     */

    return clAmsPeCompCleanupError(comp, error);
}

/*
 * clAmsPeCompProxiedCompCleanupTimeout
 * ------------------------------------
 * This fn is called as a result of a timeout of the proxied component cleanup
 * timer. 
 */

ClRcT
clAmsPeCompProxiedCompCleanupTimeout(
        CL_IN ClAmsEntityTimerT  *timer) 
{
    ClAmsCompT *comp;

    AMS_CHECKPTR ( !timer );
    AMS_CHECK_COMP ( comp = (ClAmsCompT *) timer->entity );

#ifdef AMS_EMULATE_RMD_CALLS
    return clAmsPeCompProxiedCompCleanupCallback(comp, CL_OK);
#endif

    AMS_FUNC_ENTER ( ("Component [%s]\n", comp->config.entity.name.value) );

    AMS_CALL ( clAmsEntityTimerStop(
                    (ClAmsEntityT *) comp,
                    CL_AMS_COMP_TIMER_PROXIEDCOMPCLEANUP) );

    AMS_CALL ( clAmsInvocationDeleteAll((ClAmsCompT *) comp) );

    return clAmsPeCompCleanupError(comp, CL_AMS_RC(CL_ERR_TIMEOUT) );
}

/*-----------------------------------------------------------------------------
 * Component work assignment functions
 *---------------------------------------------------------------------------*/

/*
 * clAmsPeCompRemoveWork
 * ---------------------
 * Remove all CSI assigned to a component.
 */

ClRcT
clAmsPeCompRemoveWork(
                      ClAmsCompT *comp,
                      ClUint32T switchoverMode)
{
    ClAmsEntityRefT *entityRef;
    ClAmsSUT *su;
    ClAmsNodeT *node;

    AMS_CHECK_COMP ( comp );
    AMS_CHECK_SU ( su = (ClAmsSUT *) comp->config.parentSU.ptr );
    AMS_CHECK_NODE ( node = (ClAmsNodeT *) su->config.parentNode.ptr );

    AMS_FUNC_ENTER ( ("Component [%s]\n", comp->config.entity.name.value) );

    /*
     * If the component is on a node that is not present (due to a critical
     * fault), then the switchoverMode is forced to be fast and the net result 
     * is only to reset internal state without communicating with the component. 
     * This path is typically followed when a fault is reported on a component 
     * and the recovery is a SU/comp failover.
     */

    switchoverMode &= ~CL_AMS_ENTITY_SWITCHOVER_FAST;

    clAmsPeCompComputeSwitchoverMode(comp, &switchoverMode);

    AMS_ENTITY_LOG (comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
                    ("Component [%s] Remove Work Request with switchover mode %d",
                     comp->config.entity.name.value, switchoverMode));

    entityRef = clAmsEntityListGetFirst(&comp->status.csiList);

    while ( entityRef != (ClAmsEntityRefT *) NULL )
    {
        ClAmsCSIT *csi = NULL;
        ClAmsCSIT *csiNext = NULL;

        csi = (ClAmsCSIT *) entityRef->ptr;
        entityRef = clAmsEntityListGetNext(&comp->status.csiList, entityRef);
        
        if(entityRef)
        {
            csiNext = (ClAmsCSIT*)entityRef->ptr;
        }

        if(csi->config.csiDependentsList.numEntities > 0 
           &&
           clAmsPeCheckCSIDependentsAssigned(su, csi))
            continue;

        AMS_CALL ( clAmsPeCompRemoveCSI(comp, csi, switchoverMode) );
        /*
         * The csi reference could have been deleted behind our back.
         * Double check before accessing.
         */
        if(csiNext)
        {
            entityRef = NULL;
            if( clAmsEntityListFindEntityRef2(
                                              &comp->status.csiList, 
                                              &csiNext->config.entity,
                                              0, 
                                              (ClAmsEntityRefT **)&entityRef) != CL_OK)
            {
                entityRef = clAmsEntityListGetFirst(&comp->status.csiList);
            }
        }

    }

    return CL_OK;
}

/*
 * clAmsPeCompSwitchoverWork
 * -------------------------
 * This fn switches over all the CSI assigned to a component matching the 
 * ha state. Active CSIs are quiesced and then reassigned in the callback, 
 * while all others are removed. 
 */

ClRcT
clAmsPeCompSwitchoverWork(
                          CL_IN       ClAmsCompT *comp,
                          CL_IN       ClUint32T switchoverMode,
                          CL_IN       ClAmsHAStateT haState)
{
    ClAmsEntityRefT *entityRef;
    ClRcT (*csiFn)(ClAmsCompT *, ClAmsCSIT *, ClUint32T);
    ClAmsSUT *su;
    ClAmsNodeT *node;

    AMS_CHECK_COMP ( comp );
    AMS_CHECK_SU ( su = (ClAmsSUT *) comp->config.parentSU.ptr );
    AMS_CHECK_NODE ( node = (ClAmsNodeT *) su->config.parentNode.ptr );

    AMS_FUNC_ENTER ( ("Component [%s]\n", comp->config.entity.name.value) );

    /*
     * If the component is on a node that is not present (due to a critical
     * fault), then the switchoverMode is forced to be fast and the net result 
     * is only to reset internal state without communicating with the component. 
     * This path is typically followed when a fault is reported on a component 
     * and the recovery is a SU/comp failover.
     */

    clAmsPeCompComputeSwitchoverMode(comp, &switchoverMode);

    /*
     * Decide which path to follow depending on the HA state of the CSIs
     * to be switched over.
     */

    if ( haState == CL_AMS_HA_STATE_ACTIVE )
    {
        if ( switchoverMode == CL_AMS_ENTITY_SWITCHOVER_GRACEFUL )
        {
            csiFn = clAmsPeCompQuiesceCSIGracefully;
        }
        else if ( switchoverMode == CL_AMS_ENTITY_SWITCHOVER_IMMEDIATE )
        {
            csiFn = clAmsPeCompQuiesceCSIImmediately;
        }
        else 
        {
            // For now, will change later to removeCSI once FAST and PRETEND
            // modes are separated

            csiFn = clAmsPeCompQuiesceCSIImmediately;
        }
    }
    else if ( haState == CL_AMS_HA_STATE_QUIESCING)
    {
        /*
         * Repeating the quiescing as we arent sure if the last one had 
         * completed.Anyway duplicate assignments are ignored from client side.
         */
        csiFn = clAmsPeCompQuiesceCSIGracefully;
    }
    else if ( haState == CL_AMS_HA_STATE_QUIESCED )
    {
        // For now, we repeat the quiesced operation as we do not know if the
        // previous quiesced operation had succeeded. This can result in a 
        // component getting duplicate quiesced requests in some failure
        // scenarios.

        csiFn = clAmsPeCompQuiesceCSIImmediately;
    }
    else
    {
        csiFn = clAmsPeCompRemoveCSI;
    }

    entityRef = clAmsEntityListGetFirst(&comp->status.csiList);

    AMS_ENTITY_LOG (comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
                    ("Component [%s] Switchover Request for CSIs with HA state [%s] \n",
                     comp->config.entity.name.value,
                     CL_AMS_STRING_H_STATE(haState)) );

    while ( entityRef != (ClAmsEntityRefT *) NULL )
    {
        ClAmsCompCSIRefT *csiRef = (ClAmsCompCSIRefT *) entityRef;
        ClAmsCSIT *csi = (ClAmsCSIT *) entityRef->ptr;
        ClAmsCSIT *csiNext = NULL;
        
        /*
         * csiFn will change csiList, so get next entry in list upfront.
         */

        entityRef = clAmsEntityListGetNext(&comp->status.csiList, entityRef);
        if(entityRef)
        {
            csiNext = (ClAmsCSIT*)entityRef->ptr;
        }

        if ( csiRef->haState == haState )
        {
            if(csi->config.csiDependentsList.numEntities > 0 
               &&
               clAmsPeCheckCSIDependentsAssigned(su, csi))
            {
                continue;
            }

            AMS_CALL ( csiFn(comp, csi, switchoverMode) );

            /*
             * The csi reference could have been deleted behind our back.
             * Double check before accessing.
             */
            if(csiNext)
            {
                entityRef = NULL;
                if( clAmsEntityListFindEntityRef2(
                                                  &comp->status.csiList, 
                                                  &csiNext->config.entity,
                                                  0, 
                                                  (ClAmsEntityRefT **)&entityRef) != CL_OK)
                {
                    entityRef = clAmsEntityListGetFirst(&comp->status.csiList);
                }
            }
        }
    }

    return CL_OK;
}

/*
 * clAmsPeCompSwitchoverCallback
 * -----------------------------
 * This fn is called when all the CSIs in a component have switched over.
 */

ClRcT
clAmsPeCompSwitchoverCallback(
        CL_IN       ClAmsCompT          *comp,
        CL_IN       ClRcT               error,
        CL_IN       ClUint32T           switchoverMode)
{
    ClAmsSUT *su;

    AMS_CHECK_COMP ( comp );
    AMS_CHECK_SU ( su = (ClAmsSUT *) comp->config.parentSU.ptr );

    AMS_FUNC_ENTER ( ("Component [%s]\n", comp->config.entity.name.value) );

    if ( comp->status.numStandbyCSIs || comp->status.numActiveCSIs )
    {
        AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
            ("Component [%s] has [%d] assignments. Ignoring switchover callback ..\n",
             comp->config.entity.name.value,
             comp->status.numActiveCSIs + comp->status.numStandbyCSIs));

        return CL_OK;
    }

    AMS_CALL ( clAmsPeSUSwitchoverCallback(su, error, switchoverMode) );

    return CL_OK;
}

/*
 * clAmsPeCompReassignWork
 * -----------------------
 * This fn reassigns the CSIs assigned to a component to their standby
 * components.
 */

ClRcT
clAmsPeCompReassignWork(
                        ClAmsCompT *comp,
                        ClAmsSUT **activeSU,
                        ClListHeadT *siList,
                        ClBoolT *allCSIsReassigned)
{
    ClAmsEntityRefT *entityRef;
    ClAmsSUT *su;
    ClAmsSGT *sg;
    ClBoolT dependency = CL_FALSE;
    ClUint32T numAssigned = 0;

    AMS_CHECKPTR (!activeSU);
    AMS_CHECK_COMP ( comp );
    AMS_CHECK_SU ( su = (ClAmsSUT *) comp->config.parentSU.ptr );
    AMS_CHECK_SG ( sg = (ClAmsSGT *) su->config.parentSG.ptr );

    AMS_FUNC_ENTER ( ("Component [%s]\n", comp->config.entity.name.value) );

    /*
     * Reassign the CSIs on the csiList to their standby components. If the
     * redundancy model is 2N, then the component capability model requires
     * that all CSIs be switched over at once.
     */

    /*
     * Check if the CSI has dependencies for 2N as its a TARGET_ALL
     * case for it by default.
     */

    if(sg->config.redundancyModel == CL_AMS_SG_REDUNDANCY_MODEL_TWO_N)
    {
        for(entityRef = clAmsEntityListGetFirst(&comp->status.csiList);
            entityRef != NULL;
            entityRef = clAmsEntityListGetNext(&comp->status.csiList, entityRef))
        {
            ClAmsCSIT *csi = (ClAmsCSIT*)entityRef->ptr;
            ClAmsSIT *si = (ClAmsSIT*)csi->config.parentSI.ptr;
            AMS_CHECKPTR(!csi);
            AMS_CHECKPTR(!si);
            if(csi->config.csiDependentsList.numEntities > 0 
               ||
               csi->config.csiDependenciesList.numEntities > 0)
            {
                dependency = CL_TRUE;
                break;
            }
            if(si->config.siDependentsList.numEntities > 0
               ||
               si->config.siDependenciesList.numEntities > 0)
            {
                dependency = CL_TRUE;
                break;
            }
        }
    }

    entityRef = clAmsEntityListGetFirst(&comp->status.csiList);

    while ( entityRef != (ClAmsEntityRefT *) NULL )
    {
        ClAmsCSIT *csi = (ClAmsCSIT *) entityRef->ptr;
        ClAmsSIT  *si  = NULL;

        AMS_CHECKPTR(!csi);

        si = (ClAmsSIT*)csi->config.parentSI.ptr;

        AMS_CHECKPTR(!si);

        entityRef = clAmsEntityListGetNext(&comp->status.csiList, entityRef);

        if(clAmsPeSIReassignMatch(si, siList))
        {
            ++numAssigned;
            dependency = CL_TRUE;
            continue;
        }

        /*
         * Verify that CSI (really SI) is unlocked.
         */

        ClAmsAdminStateT adminState;

        clAmsPeCSIComputeAdminState(csi, &adminState);

        if ( adminState != CL_AMS_ADMIN_STATE_UNLOCKED )
        {
            continue;
        }

        /*
         * Find component for this CSI with lowest standby rank.
         */

        ClAmsCompT *standby = NULL;
        ClUint32T  standbyRank = 0xFFFFFFFF;

        ClAmsEntityRefT *eRef;

        for ( eRef = clAmsEntityListGetFirst(&csi->status.pgList);
              eRef != (ClAmsEntityRefT *) NULL;
              eRef = clAmsEntityListGetNext(&csi->status.pgList, eRef) )
        {
            ClAmsCSICompRefT    *standbyRef = (ClAmsCSICompRefT *) eRef;
            ClAmsCompT          *c = (ClAmsCompT *) eRef->ptr;
            ClAmsSUT            *cSU = NULL;
            ClAmsNodeT          *cNode = NULL;

            AMS_CHECK_SU(cSU = (ClAmsSUT*)c->config.parentSU.ptr);
            AMS_CHECK_NODE(cNode = (ClAmsNodeT*)cSU->config.parentNode.ptr);

            if ( standbyRef->haState != CL_AMS_HA_STATE_STANDBY )
            {
                /*
                 * If there is an existing active and it isn't the one
                 * getting switched over for the CSI, 
                 * then skip reassigning standby that is properly assigned
                 */
                if(standbyRef->haState == CL_AMS_HA_STATE_ACTIVE)
                {
                    if(cSU != su
                       &&
                       (c->config.capabilityModel != CL_AMS_COMP_CAP_X_ACTIVE_AND_Y_STANDBY)
                       && 
                       (sg->config.redundancyModel != CL_AMS_SG_REDUNDANCY_MODEL_CUSTOM))
                    {
                        *activeSU = cSU;
                        standby = NULL;
                        clLogNotice("COMP", "REASSIGN", "Component [%s] part of SU [%s] "
                                    "is already assigned active. Skipping reassignment "
                                    "for SU [%s]", c->config.entity.name.value,
                                    cSU->config.entity.name.value,
                                    su->config.entity.name.value);
                    }
                }
                continue;
            }

            if(*activeSU && cSU != *activeSU) continue;

            clAmsPeCompUpdateReadinessState(c);

            if ( c->status.readinessState != CL_AMS_READINESS_STATE_INSERVICE )
            {
                continue;
            }

            if(cNode->status.isClusterMember != CL_AMS_NODE_IS_CLUSTER_MEMBER)
            {
                continue;
            }

            if ( standbyRef->rank < standbyRank )
            {
                standbyRank = standbyRef->rank;
                standby = c;
            }
        }

        if ( standby == NULL )
        {
            AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
                           ("CSI [%s] assigned to Component [%s] has no in-service standby Component. Work reassignment not possible..\n", 
                            csi->config.entity.name.value,
                            comp->config.entity.name.value));

            continue;
        }
        
        /*
         * Check if this standby has pending removes. If yes, skip
         * reassignment as on re-evaluation, it could become active if rules
         * are satisfied.
         */

        if( (!(*activeSU))
            && 
           clAmsInvocationPendingForComp(standby, CL_AMS_CSI_RMV_CALLBACK))
        {
            AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
                           ("Component [%.*s] has a pending csi remove. Skipping reassignment\n",
                            standby->config.entity.name.length-1, standby->config.entity.name.value));
            continue;
        }

        if(!(*activeSU))
            *activeSU = (ClAmsSUT*)standby->config.parentSU.ptr;

        /*
         * If all CSIs need to be assigned at once, then take a different path,
         * else reassign each CSI separately.
         */
        
        if ( sg->config.redundancyModel == CL_AMS_SG_REDUNDANCY_MODEL_TWO_N 
             &&
             !dependency)
        {
            ++numAssigned;
            AMS_CALL ( clAmsPeCompReassignAllCSI(standby,
                                                 comp,
                                                 CL_AMS_HA_STATE_ACTIVE) );
            break;
        }
        else
        {
            /*
             * Assign dependencies first.
             */
            if(csi->config.csiDependenciesList.numEntities > 0 )
            {
                if(!dependency) dependency = CL_TRUE;
                continue;
            }
            ++numAssigned;
            AMS_CALL ( clAmsPeCompReassignCSI(standby,
                                              comp,
                                              csi,
                                              CL_AMS_HA_STATE_ACTIVE) );
        }
    }

    if(allCSIsReassigned)
    {
        *allCSIsReassigned = !dependency;
    }
    return CL_OK;
}


ClRcT
clAmsPeReplayCSI(ClAmsCompT *comp,ClAmsInvocationT *invocationData, ClBoolT scFailover)
{
    ClRcT rc  = CL_OK;
    ClAmsNodeT *node = NULL;
    ClAmsSUT *su = NULL;
    ClAmsCSIT *csi = NULL;
    ClAmsCompCSIRefT *csiRef = NULL;
    ClAmsEntityTimerTypeT type = CL_AMS_COMP_TIMER_CSISET;
    ClUint32T switchoverMode = CL_AMS_ENTITY_SWITCHOVER_IMMEDIATE;
    ClRcT error = CL_OK;

    AMS_CHECK_COMP ( comp );

    AMS_CHECKPTR( !invocationData );

    AMS_FUNC_ENTER ( ("Component [%s]\n", comp->config.entity.name.value) );

    AMS_CHECK_CSI(csi = invocationData->csi);

    AMS_CHECK_SU( su = (ClAmsSUT*)comp->config.parentSU.ptr);

    AMS_CHECK_NODE( node = (ClAmsNodeT*)su->config.parentNode.ptr);

    if(invocationData->cmd != CL_AMS_CSI_SET_CALLBACK &&
       invocationData->cmd != CL_AMS_CSI_QUIESCING_CALLBACK)
    {
        AMS_ENTITY_LOG(comp,CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_ERROR,
                       ("Invocation data does not match CSI command\n"));
        return CL_OK;
    }

    clAmsPeCompComputeSwitchoverMode(comp, &switchoverMode);

    rc = clAmsEntityListFindEntityRef2(
                                       &comp->status.csiList, 
                                       &csi->config.entity,
                                       0, 
                                       (ClAmsEntityRefT **)&csiRef);

    if(CL_OK != rc)
    {
        AMS_ENTITY_LOG(comp,CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_ERROR,
                       ("Component [%s] does not have CSI [%s] assigned\n",
                        comp->config.entity.name.value,csi->config.entity.name.value));
        return CL_OK;
    }

    if(csiRef->pendingOp != invocationData->cmd)
    {
        AMS_ENTITY_LOG(comp,CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_ERROR,
                       ("CSI [%s],pending op [%#x] does not match pending invocation cmd [%#x]."
                        " [%s] replay",
                        csi->config.entity.name.value, csiRef->pendingOp, invocationData->cmd,
                        csiRef->pendingOp ? "Skipping" : "Going ahead with the csi set"));
        if(csiRef->pendingOp)
            return CL_OK;
    }

    AMS_ENTITY_LOG (comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_INFO,
                    ("Re-Assigning CSI [%s] to Component [%s] with mode [%d], HA State [%s], Invocation [%#llx]",
                     csi->config.entity.name.value,
                     comp->config.entity.name.value,
                     switchoverMode,
                     CL_AMS_STRING_H_STATE(csiRef->haState),
                     invocationData->invocation));

    if(invocationData->cmd == CL_AMS_CSI_QUIESCING_CALLBACK)
    {
        type = CL_AMS_COMP_TIMER_QUIESCINGCOMPLETE;
    }

    if(scFailover) 
        switchoverMode |= CL_AMS_ENTITY_SWITCHOVER_CONTROLLER;

    /*
     * Now make the actual call to replay the CSI to the component. This 
     * takes the component's property into account.
     */

    switch ( comp->config.property )
    {
    case CL_AMS_COMP_PROPERTY_SA_AWARE:
        {
            ClAmsCSIDescriptorT *csiDescriptor = NULL;

            /*
             * First stop the pending timer for this CSI set if any
             */
            AMS_CALL ( clAmsEntityTimerStop
                       ((ClAmsEntityT*)comp,
                        type) );

            AMS_CALL ( clAmsCSIMarshalCSIDescriptor(
                                                    &csiDescriptor,
                                                    csi,
                                                    CL_AMS_CSI_FLAG_TARGET_ONE,
                                                    csiRef->haState,
                                                    comp->config.entity.name,
                                                    csiRef->tdescriptor,
                                                    csiRef->rank) );

            AMS_CALL ( clAmsEntityTimerStart(
                                             (ClAmsEntityT *) comp,
                                             type) );
#ifdef AMS_CPM_INTEGRATION
            
            if(! (switchoverMode & CL_AMS_ENTITY_SWITCHOVER_FAST) )
            {
                error = _clAmsSACSISetNoCkpt(
                                             &comp->config.entity.name,
                                             &comp->config.entity.name,
                                             invocationData->invocation,
                                             csiRef->haState,
                                             *csiDescriptor);
            }

#endif

            clAmsFreeMemory(csiDescriptor->csiAttributeList.attribute);
            clAmsFreeMemory(csiDescriptor);

            if ( error )
            {
                AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_ERROR,
                               ("CSI assignment to Component [%s] with HA state [%s] returned Error [0x%x]\n",
                                comp->config.entity.name.value,
                                CL_AMS_STRING_H_STATE(csiRef->haState),
                                error) ); 
                
                if(invocationData->cmd == CL_AMS_CSI_SET_CALLBACK)
                    return clAmsPeCompAssignCSIError(comp, error);
                else
                    return clAmsPeCompQuiesceCSIGracefullyError(comp, error);
            }

            break;
        }

    case CL_AMS_COMP_PROPERTY_PROXIED_PREINSTANTIABLE:
        {
            ClAmsCSIDescriptorT *csiDescriptor = NULL;

            AMS_CALL ( clAmsEntityTimerStop(
                                            (ClAmsEntityT *) comp,
                                            type)
                       );

            if ( clAmsPeCompIsProxyReady(comp) == CL_FALSE )
            {
                AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
                               ("Component [%s] has no proxy. Defering CSI assignment\n",
                                comp->config.entity.name.value));

                return CL_OK;
            }

            AMS_CALL ( clAmsCSIMarshalCSIDescriptor(
                                                    &csiDescriptor,
                                                    csi,
                                                    CL_AMS_CSI_FLAG_TARGET_ONE,
                                                    csiRef->haState,
                                                    comp->config.entity.name,
                                                    csiRef->tdescriptor,
                                                    csiRef->rank) );

            AMS_CALL ( clAmsEntityTimerStart(
                                             (ClAmsEntityT *) comp,
                                             type ) );

#ifdef AMS_CPM_INTEGRATION
 
            ClAmsCompT *proxy = (ClAmsCompT *) comp->status.proxyComp;

            if(!(switchoverMode & CL_AMS_ENTITY_SWITCHOVER_FAST))
            {
                error = _clAmsSACSISetNoCkpt(
                                             &comp->config.entity.name,
                                             &proxy->config.entity.name,
                                             invocationData->invocation,
                                             csiRef->haState,
                                             *csiDescriptor);
            }
#endif

            clAmsFreeMemory(csiDescriptor->csiAttributeList.attribute);
            clAmsFreeMemory(csiDescriptor);

            if ( error )
            {
                AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
                               ("CSI assignment to Component [%s] with HA state [%s] returned Error [0x%x]\n",
                                comp->config.entity.name.value,
                                CL_AMS_STRING_H_STATE(csiRef->haState),
                                error) ); 

                if(invocationData->cmd == CL_AMS_CSI_SET_CALLBACK)
                    return clAmsPeCompAssignCSIError(comp, error);
                else
                    return clAmsPeCompQuiesceCSIGracefullyError(comp, error);
            }

            break;
        }

    case CL_AMS_COMP_PROPERTY_PROXIED_NON_PREINSTANTIABLE:
    case CL_AMS_COMP_PROPERTY_NON_PROXIED_NON_PREINSTANTIABLE:
        {
            /*
             * If its an active assignment, then the non-preinstantiable
             * component needs to be instantiated. For any other ha state,
             * (just standby, based on earlier checks) just pretend it 
             * succeeded, so SU assignment and rest of logic works.
             */

            AMS_CALL ( clAmsEntityTimerStop(
                                            (ClAmsEntityT *) comp,
                                            type)
                       );

            if ( clAmsPeCompIsProxyReady(comp) == CL_FALSE )
            {
                AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
                               ("Component [%s] has no proxy. Defering CSI assignment\n",
                                comp->config.entity.name.value));

                return CL_OK;
            }
            
            if(!(switchoverMode & CL_AMS_ENTITY_SWITCHOVER_FAST))
            {
                if ( csiRef->haState == CL_AMS_HA_STATE_ACTIVE )
                {
                    AMS_CALL ( clAmsPeCompInstantiate(comp) );
                }
                else
                {
                    goto csiCallback;
                }
            }
            break;
        }

    default:
        {
            AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_ERROR,
                           ("Error: Invalid Property [%d] for Component [%s]. Exiting..\n",
                            comp->config.property,
                            comp->config.entity.name.value));

            return CL_AMS_RC(CL_AMS_ERR_BAD_CONFIG);
        }
    }

    if( (switchoverMode & CL_AMS_ENTITY_SWITCHOVER_FAST) )
    {
        csiCallback:

        if(invocationData->cmd == CL_AMS_CSI_SET_CALLBACK)
        {
            AMS_CALL (clAmsPeCompAssignCSICallback(comp, 
                                                   invocationData->invocation,
                                                   CL_OK,
                                                   switchoverMode) );
        }
        else
        {
            AMS_CALL (clAmsPeCompQuiescingCompleteCallback(comp,
                                                           invocationData->invocation,
                                                           CL_OK,
                                                           switchoverMode));
        }
        
    }

    return CL_OK;
}

/*-----------------------------------------------------------------------------
 * Component CSI assignment functions
 *---------------------------------------------------------------------------*/

/*
 * clAmsPeCompAssignCSI
 * --------------------
 * This fn is called to assign a CSI to a component. The haState can be active or
 * standby. The function handles the case when a csi assignment is being made a
 * second time to a component, either due to a standby->active change or due to
 * a standby rank change.
 */

ClRcT
clAmsPeCompAssignCSIExtended(
                             CL_IN   ClAmsCompT *comp,
                             CL_IN   ClAmsCSIT *csi,
                             CL_IN   ClAmsHAStateT haState,
                             ClBoolT reassignCSI)
{
    ClAmsEntityRefT *entityRef;
    ClRcT rc;
    ClAmsCompCSIRefT *csiRef;
    ClAmsCSICompRefT *compRef;
    ClAmsCompT *activeComp;
    ClAmsCompT *lastActiveComp = NULL;
    ClUint32T standbyRank;
    ClAmsCSIFlagsT csiFlags;
    ClRcT error = CL_OK;

    AMS_CHECK_COMP ( comp );
    AMS_CHECK_CSI ( csi );

    AMS_FUNC_ENTER ( ("Component [%s]\n", comp->config.entity.name.value) );

    /*
     * Step 1: Verify if this CSI is already assigned to this Component.  If not, 
     * then create an appropriate csiRef. If yes, just reuse the existing csiRef. 
     * The second case happens when a CSI is reassigned to a component during 
     * restart or after a switchover.
     */

    rc = clAmsEntityListFindEntityRef2(
                                       &comp->status.csiList, 
                                       &csi->config.entity,
                                       0, 
                                       (ClAmsEntityRefT **)&csiRef);

    if ( CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST )
    {
        /*
         * This is a new assignment. 
         *
         * If this is an active assignment, then the rank of this assignment is 
         * 0 and the component is the active comp. 
         *
         * If this is a standby assignment, find the component with the active
         * CSI assignment. If no such component is found, print out a warning.
         * Also, compute the standby rank. It is assumed that the ranks are
         * always contiguous and start at 1.
         */

        AMS_ENTITY_LOG (comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
                        ("Assigning new CSI [%s] to Component [%s] with HA State [%s]\n",
                         csi->config.entity.name.value,
                         comp->config.entity.name.value,
                         CL_AMS_STRING_H_STATE(haState)));

        if ( haState == CL_AMS_HA_STATE_ACTIVE )
        {
            activeComp = comp;
            standbyRank = 0;
        }
        else if ( haState == CL_AMS_HA_STATE_STANDBY )
        {
            ClAmsCompT *standbyComp = NULL;
            activeComp = NULL;
            standbyRank = 1;

            for ( entityRef = clAmsEntityListGetFirst(&csi->status.pgList);
                  entityRef != (ClAmsEntityRefT *) NULL;
                  entityRef = clAmsEntityListGetNext(&csi->status.pgList, entityRef) )
            {
                ClAmsCSICompRefT *cRef = (ClAmsCSICompRefT *) entityRef;

                if ( cRef->haState == CL_AMS_HA_STATE_ACTIVE )
                {
                    activeComp = (ClAmsCompT *) entityRef->ptr;
                }

                if ( cRef->haState == CL_AMS_HA_STATE_STANDBY )
                {
                    standbyComp = (ClAmsCompT*)entityRef->ptr;
                    standbyRank++;
                }
            }

            if ( !activeComp )
            {
                AMS_ENTITY_LOG (comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_LOG_SEV_WARNING,
                                ("Warning: Assigning CSI [%s] with HA state [Standby] to Component [%s] but no active assignment found.\n",
                                 csi->config.entity.name.value,
                                 comp->config.entity.name.value));

                activeComp = standbyComp;
            }
        }
        else
        {
            return CL_AMS_RC(CL_ERR_INVALID_STATE);
        }

        if(!activeComp)
            return CL_OK;

        /*
         * Add a new csiRef to this component, update the si and su counts in the
         * process and then set up the csiRef appropriately.
         */

        AMS_CALL ( clAmsPeCSITransitionHAState(
                                               csi,
                                               comp,
                                               CL_AMS_HA_STATE_NONE,
                                               haState));

        AMS_CALL ( clAmsEntityListFindEntityRef2(
                                                 &comp->status.csiList, 
                                                 &csi->config.entity,
                                                 0, 
                                                 (ClAmsEntityRefT **)&csiRef) );

        csiRef->activeComp = (ClAmsEntityT *) activeComp;
        csiRef->rank = standbyRank;
        csiRef->tdescriptor = CL_AMS_CSI_NEW_ASSIGN;

        AMS_CALL ( clAmsEntityListFindEntityRef2(
                                                 &csi->status.pgList, 
                                                 &comp->config.entity,
                                                 0, 
                                                 (ClAmsEntityRefT **)&compRef) );

        compRef->rank = standbyRank;
        lastActiveComp = activeComp;
        csiFlags = CL_AMS_CSI_FLAG_ADD_ONE;
    }
    else if ( rc == CL_OK )
    {
        ClAmsSUT *su = NULL;
        ClAmsNodeT *node = NULL;
        ClBoolT activeSwap = CL_FALSE;

        /*
         * This is a reassignment of the CSI to this comp with possibly updated
         * states, transition descriptors and rank. 
         *
         * If this is a standby assignment, find the comp with the active CSI
         * assignment. If no such comp is found, print out a warning. Also,
         * compute the new standby rank of this CSI by counting the number
         * of CSI assignments with lower standby ranks.
         *
         * Note: We will announce this change via pgtrack even if the component
         * is restarting and the csi with same HA state is assigned back to the
         * component. This could be changed in the future if necessary.
         */

        AMS_ENTITY_LOG (comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
                        ("Re-Assigning CSI [%s] to Component [%s] with HA State [%s]\n",
                         csi->config.entity.name.value,
                         comp->config.entity.name.value,
                         CL_AMS_STRING_H_STATE(haState)));

        if ( haState == CL_AMS_HA_STATE_ACTIVE )
        {
            activeComp = comp;
            lastActiveComp = (ClAmsCompT*)csiRef->activeComp;
            if(!lastActiveComp) lastActiveComp = comp;
            standbyRank = 0;
        }
        else if ( haState == CL_AMS_HA_STATE_STANDBY )
        {
            ClAmsCompT *standbyComp = NULL;
            activeComp = NULL;
            standbyComp = NULL;
            standbyRank = 1;

            for ( entityRef = clAmsEntityListGetFirst(&csi->status.pgList);
                  entityRef != (ClAmsEntityRefT *) NULL;
                  entityRef = clAmsEntityListGetNext(&csi->status.pgList, entityRef) )
            {
                ClAmsCSICompRefT *cRef = (ClAmsCSICompRefT *) entityRef;

                if ( cRef->haState == CL_AMS_HA_STATE_ACTIVE )
                {
                    activeComp = (ClAmsCompT *) entityRef->ptr;
                }

                if ( cRef->haState == CL_AMS_HA_STATE_STANDBY )
                {
                    if ( cRef->rank < csiRef->rank )
                    {
                        standbyComp = (ClAmsCompT*)entityRef->ptr;
                        standbyRank++;
                    }
                    else if(!standbyComp) standbyComp = (ClAmsCompT*)entityRef->ptr;
                }
            }

            if ( !activeComp )
            {
                AMS_ENTITY_LOG (comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
                                ("Warning: Assigning CSI [%s] with HA state [Standby] to Component [%s] but no active assignment found.\n",
                                 csi->config.entity.name.value,
                                 comp->config.entity.name.value));
                activeSwap = CL_TRUE;
                activeComp = standbyComp;
            }
            lastActiveComp = activeComp;
            if(!lastActiveComp)
            {
                clLogWarning("COMP", "CSI-ASSIGN", "No active component found for component [%s]. "
                             "Using it as the active", comp->config.entity.name.value);
                lastActiveComp = comp;
                activeSwap = CL_TRUE;
            }
        }
        else
        {
            return CL_AMS_RC(CL_ERR_INVALID_STATE);
        }

        AMS_CALL ( clAmsPeCSITransitionHAState(
                                               csi,
                                               comp,
                                               csiRef->haState,
                                               haState) );

        csiRef->activeComp = (ClAmsEntityT *) activeComp;
        csiRef->rank = standbyRank;

        if(activeSwap)
            csiRef->tdescriptor = CL_AMS_CSI_NOT_QUIESCED;
        else
        {
            /*
             * Check for CSI reassignments on comp. restarts.
             * as those should be flagged as new assign.
             */
            if(reassignCSI)
            {
                csiRef->tdescriptor = CL_AMS_CSI_NEW_ASSIGN;
            }
            else
            {
                csiRef->tdescriptor = ( lastActiveComp->status.operState ==
                                        CL_AMS_OPER_STATE_DISABLED ) ?
                    CL_AMS_CSI_NOT_QUIESCED :
                    CL_AMS_CSI_QUIESCED;
            }
        }

        if(!activeSwap)
        {
            su = (ClAmsSUT*)lastActiveComp->config.parentSU.ptr;
            if(su) node = (ClAmsNodeT*)su->config.parentNode.ptr;
            if(node && node->status.operState == CL_AMS_OPER_STATE_DISABLED)
            {
                csiRef->tdescriptor = CL_AMS_CSI_NOT_QUIESCED;
            }
            else
            {
                /*
                 * Check for pending swap ops
                 */
                su = (ClAmsSUT*)comp->config.parentSU.ptr;
                if(su && 
                   clAmsEntityOpPending(&su->config.entity, &su->status.entity, 
                                        CL_AMS_ENTITY_OP_SWAP_ACTIVE_MPLUSN))
                    csiRef->tdescriptor = CL_AMS_CSI_NOT_QUIESCED;
            }
        }

        AMS_CALL ( clAmsEntityListFindEntityRef2(
                                                 &csi->status.pgList, 
                                                 &comp->config.entity,
                                                 0, 
                                                 (ClAmsEntityRefT **)&compRef) );

        compRef->rank = standbyRank;

        csiFlags = CL_AMS_CSI_FLAG_TARGET_ONE;
    }
    else
    {
        /*
         * Got an error while searching for CSI assignment.
         */

        AMS_ENTITY_LOG (comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_ERROR,
                        ("Error [0x%x] : Assigning CSI [%s] to Component [%s] with HA State [%s]\n",
                         rc,
                         csi->config.entity.name.value,
                         comp->config.entity.name.value,
                         CL_AMS_STRING_H_STATE(haState)));

        return rc;
    }

    /*
     * Step 2: Now make the actual call to assign the CSI to the component. This 
     * takes the component's property into account.
     */

          
    switch ( comp->config.property )
    {
    case CL_AMS_COMP_PROPERTY_SA_AWARE:
        {
            ClAmsCSIDescriptorT *csiDescriptor = NULL;
            ClInvocationT invocation = 0;
            if (!lastActiveComp)
            {
                AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_LOG_SEV_ERROR,
                               ("Refusing to execute CSI assignment to Component [%s] with HA state [%s] since there is no Active assignment.",
                                comp->config.entity.name.value,
                                CL_AMS_STRING_H_STATE(haState)) );
                return CL_AMS_RC(CL_ERR_INVALID_STATE);
            }

            AMS_CALL ( clAmsCSIMarshalCSIDescriptorExtended(
                                                    &csiDescriptor,
                                                    csi,
                                                    csiFlags,
                                                    csiRef->haState,
                                                    lastActiveComp->config.entity.name,
                                                    csiRef->tdescriptor,
                                                    csiRef->rank,
                                                    reassignCSI) );

            AMS_CALL ( clAmsInvocationCreateExtended(
                                                     CL_AMS_CSI_SET_CALLBACK,
                                                     comp,
                                                     csi,
                                                     reassignCSI,
                                                     &invocation) );

            AMS_CALL ( clAmsEntityTimerStart(
                                             (ClAmsEntityT *) comp,
                                             CL_AMS_COMP_TIMER_CSISET) );

#ifdef AMS_CPM_INTEGRATION

            error = _clAmsSACSISet(
                                   &comp->config.entity.name,
                                   &comp->config.entity.name,
                                   invocation,
                                   haState,
                                   *csiDescriptor);

#endif

            clAmsFreeMemory(csiDescriptor->csiAttributeList.attribute);
            clAmsFreeMemory(csiDescriptor);

            if ( error )
            {
                AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
                               ("CSI assignment to Component [%s] with HA state [%s] returned Error [0x%x]\n",
                                comp->config.entity.name.value,
                                CL_AMS_STRING_H_STATE(haState),
                                error) ); 

                return clAmsPeCompAssignCSIError(comp, error);
            }

            break;
        }

    case CL_AMS_COMP_PROPERTY_PROXIED_PREINSTANTIABLE:
        {
            if ( clAmsPeCompIsProxyReady(comp) == CL_FALSE )
            {
                AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
                               ("Component [%s] has no proxy. Defering CSI assignment\n",
                                comp->config.entity.name.value));

                return CL_OK;
            }

            ClAmsCSIDescriptorT *csiDescriptor = NULL;
            ClInvocationT invocation = 0;

            AMS_CALL ( clAmsCSIMarshalCSIDescriptorExtended(
                                                    &csiDescriptor,
                                                    csi,
                                                    csiFlags,
                                                    csiRef->haState,
                                                    lastActiveComp->config.entity.name,
                                                    csiRef->tdescriptor,
                                                    csiRef->rank,
                                                    reassignCSI) );

            AMS_CALL ( clAmsInvocationCreateExtended(
                                                     CL_AMS_CSI_SET_CALLBACK,
                                                     comp,
                                                     csi,
                                                     reassignCSI,
                                                     &invocation) );

            AMS_CALL ( clAmsEntityTimerStart(
                                             (ClAmsEntityT *) comp,
                                             CL_AMS_COMP_TIMER_CSISET) );

#ifdef AMS_CPM_INTEGRATION
 
            ClAmsCompT *proxy = (ClAmsCompT *) comp->status.proxyComp;

            error = _clAmsSACSISet(
                                   &comp->config.entity.name,
                                   &proxy->config.entity.name,
                                   invocation,
                                   haState,
                                   *csiDescriptor);

#endif

            clAmsFreeMemory(csiDescriptor->csiAttributeList.attribute);
            clAmsFreeMemory(csiDescriptor);

            if ( error )
            {
                AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
                               ("CSI assignment to Component [%s] with HA state [%s] returned Error [0x%x]\n",
                                comp->config.entity.name.value,
                                CL_AMS_STRING_H_STATE(haState),
                                error) ); 

                return clAmsPeCompAssignCSIError(comp, error);
            }

            break;
        }

    case CL_AMS_COMP_PROPERTY_PROXIED_NON_PREINSTANTIABLE:
    case CL_AMS_COMP_PROPERTY_NON_PROXIED_NON_PREINSTANTIABLE:
        {
            /*
             * If its an active assignment, then the non-preinstantiable
             * component needs to be instantiated. For any other ha state,
             * (just standby, based on earlier checks) just pretend it 
             * succeeded, so SU assignment and rest of logic works.
             */

            if ( haState == CL_AMS_HA_STATE_ACTIVE )
            {
                AMS_CALL ( clAmsPeCompInstantiate(comp) );
            }
            else
            {
                ClInvocationT invocation = 0;

                AMS_CALL ( clAmsInvocationCreateExtended(
                                                         CL_AMS_CSI_SET_CALLBACK,
                                                         comp,
                                                         csi,
                                                         reassignCSI,
                                                         &invocation) );

                AMS_CALL ( clAmsPeCompAssignCSICallback(
                                                        comp,
                                                        invocation,
                                                        CL_OK,
                                                        CL_AMS_ENTITY_SWITCHOVER_IMMEDIATE));
            }

            break;
        }

    default:
        {
            AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_ERROR,
                           ("Error: Invalid Property [%d] for Component [%s]. Exiting..\n",
                            comp->config.property,
                            comp->config.entity.name.value));

            return CL_AMS_RC(CL_AMS_ERR_BAD_CONFIG);
        }
    }

    return CL_OK;
}

ClRcT
clAmsPeCompAssignCSI(
        CL_IN   ClAmsCompT *comp,
        CL_IN   ClAmsCSIT *csi,
        CL_IN   ClAmsHAStateT haState)
{
    return clAmsPeCompAssignCSIExtended(comp, csi, haState, CL_FALSE);
}


/*
 * clAmsPeCompAssignCSIAgain
 * -------------------------
 * This fn is called to assign CSIs again that were already assigned
 * to the same component. It is used during component restart and also
 * when a proxied component is to be assigned pending CSIs that could
 * not be assigned because a proxy was not available. The fn iterates 
 * over the CSIs assigned to a component as per AMS state and calls 
 * the appropriate assignment function.
 */

ClRcT
clAmsPeCompAssignCSIAgain(
        CL_IN   ClAmsCompT *comp)
{
    ClAmsEntityRefT *entityRef;
    ClUint32T switchoverMode = CL_AMS_ENTITY_SWITCHOVER_GRACEFUL;

    AMS_CHECK_COMP ( comp );
 
    AMS_FUNC_ENTER ( ("Component [%s]\n",comp->config.entity.name.value) );

    AMS_ENTITY_LOG (comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
             ("Replaying [%d] CSI assignments for Component [%s]\n",
              comp->status.csiList.numEntities,
              comp->config.entity.name.value) );

    for ( entityRef = clAmsEntityListGetFirst(&comp->status.csiList);
          entityRef != (ClAmsEntityRefT *) NULL;
          entityRef = clAmsEntityListGetNext(&comp->status.csiList,entityRef) )
    {
        ClAmsCompCSIRefT *csiRef = (ClAmsCompCSIRefT *) entityRef;
        ClAmsCSIT *csi = (ClAmsCSIT *) entityRef->ptr;
        
        if ( (csiRef->haState == CL_AMS_HA_STATE_ACTIVE) ||
             (csiRef->haState == CL_AMS_HA_STATE_STANDBY) )
        {
            AMS_CALL ( clAmsPeCompAssignCSIExtended(comp, csi, csiRef->haState, CL_TRUE) );
        }
        else
        if ( csiRef->haState == CL_AMS_HA_STATE_QUIESCING )
        {
            AMS_CALL ( clAmsPeCompQuiesceCSIGracefullyExtended(comp, csi, switchoverMode, CL_TRUE) );
        }
        else
        if ( csiRef->haState == CL_AMS_HA_STATE_QUIESCED )
        {
            AMS_CALL ( clAmsPeCompQuiesceCSIImmediatelyExtended(comp, csi, switchoverMode, CL_TRUE) );
        }
        else
        {
            AMS_CALL ( clAmsPeCompRemoveCSI(comp, csi, switchoverMode) );
        }
    }

    return CL_OK;
}

/*
 * clAmsPeCompReassignCSI
 * ----------------------
 * This fn is called to reassign a CSI from oldcomp to comp with haState. 
 * Typically it would only be called to assign a standby CSI as active.
 */

ClRcT
clAmsPeCompReassignCSI(
        CL_IN ClAmsCompT *comp,
        CL_IN ClAmsCompT *oldcomp,
        CL_IN ClAmsCSIT *csi,
        CL_IN ClAmsHAStateT haState)
{
    AMS_CHECK_COMP ( comp );
    AMS_CHECK_COMP ( oldcomp );
    AMS_CHECK_CSI ( csi );

    AMS_FUNC_ENTER ( ("Component [%s]\n",comp->config.entity.name.value) );

    AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
        ("Assigning CSI [%s] with HA state [%s] from Component [%s] to Component [%s]\n",
         csi->config.entity.name.value,
         CL_AMS_STRING_H_STATE(haState),
         oldcomp->config.entity.name.value,
         comp->config.entity.name.value));

    AMS_CALL ( clAmsPeCompAssignCSI(comp, csi, haState) );

   return CL_OK; 
}

/*
 * clAmsPeCompReassignAllCSI
 * -------------------------
 * Reassign all the CSIs assigned to 'comp' to the haState. oldcomp is the
 * previous component which had the haState.
 */

ClRcT
clAmsPeCompReassignAllCSI(
        CL_IN ClAmsCompT *comp,
        CL_IN ClAmsCompT *oldcomp,
        CL_IN ClAmsHAStateT haState)
{
    ClAmsEntityRefT *entityRef;
    ClAmsCompT *activeComp;
    ClAmsCompT *lastActiveComp = NULL;
    ClUint32T standbyRank;
    ClAmsCSIFlagsT csiFlags;
    ClAmsCSITransitionDescriptorT tdescriptor;
    ClRcT error = CL_OK;
    ClBoolT activeSwap = CL_FALSE;
    ClAmsSUT *su = NULL;
    ClAmsNodeT *node = NULL;

    AMS_CHECK_COMP ( comp );
    AMS_CHECK_COMP ( oldcomp );

    AMS_FUNC_ENTER ( ("Component [%s]\n",comp->config.entity.name.value) );

    AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
        ("Assigning CSI [All] with HA state [%s] from Component [%s] to Component [%s]\n",
         CL_AMS_STRING_H_STATE(haState),
         oldcomp->config.entity.name.value,
         comp->config.entity.name.value));

    if ( comp->status.csiList.numEntities == 0 )
    {
        return CL_OK;
    }

    /*
     * Step 1: Find activeComp, rank and the transition descriptor. This applies
     * to all CSIs.
     */

    if ( haState == CL_AMS_HA_STATE_ACTIVE )
    {
        activeComp = comp;
        lastActiveComp = oldcomp;
        standbyRank = 0;
    }
    else if ( haState == CL_AMS_HA_STATE_STANDBY )
    {
        ClAmsEntityRefT *eRef;
        ClAmsCompCSIRefT *csiRef;
        ClAmsCompT *standbyComp = NULL;
        ClAmsCSIT *csi;
        
        activeComp = NULL;
        standbyRank = 1;

        eRef = clAmsEntityListGetFirst(&comp->status.csiList);
        AMS_CHECKPTR ( !eRef );
        csiRef = (ClAmsCompCSIRefT *) eRef;
        csi = (ClAmsCSIT *) eRef->ptr;

        for ( eRef = clAmsEntityListGetFirst(&csi->status.pgList);
              eRef != (ClAmsEntityRefT *) NULL;
              eRef = clAmsEntityListGetNext(&csi->status.pgList, eRef) )
        {
            ClAmsCSICompRefT *cRef = (ClAmsCSICompRefT *) eRef;

            if ( cRef->haState == CL_AMS_HA_STATE_ACTIVE )
            {
                activeComp = (ClAmsCompT *) eRef->ptr;
            }

            if ( cRef->haState == CL_AMS_HA_STATE_STANDBY )
            {
                if ( cRef->rank < csiRef->rank )
                {
                    standbyComp = (ClAmsCompT*)eRef->ptr;
                    standbyRank++;
                }
                else if(!standbyComp) standbyComp = (ClAmsCompT*)eRef->ptr;
            }
        }

        if ( !activeComp )
        {
            AMS_ENTITY_LOG (comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
                 ("Warning: Assigning CSI [%s] with HA state [Standby] to Component [%s] but no active assignment found.\n",
                  csi->config.entity.name.value,
                  comp->config.entity.name.value));

            activeComp = standbyComp; /*expecting a swap state*/
            activeSwap = CL_TRUE;
        }
        lastActiveComp = activeComp;
        if(!lastActiveComp)
        {
            lastActiveComp = comp;
            activeSwap = CL_TRUE;
            clLogWarning("COMP", "CSI-ASSIGN-ALL", "No active component found for component [%s]. "
                         "Using it as the active", comp->config.entity.name.value);
        }
    }
    else
    {
        return CL_AMS_RC(CL_ERR_INVALID_STATE);
    }

    if(activeSwap)
      tdescriptor = CL_AMS_CSI_NOT_QUIESCED;
    else
      tdescriptor = ( lastActiveComp->status.operState == CL_AMS_OPER_STATE_DISABLED ) ?
                                                        CL_AMS_CSI_NOT_QUIESCED :
                                                        CL_AMS_CSI_QUIESCED;

    if(!activeSwap)
    { 
        su = (ClAmsSUT*)lastActiveComp->config.parentSU.ptr;
        if(su) node = (ClAmsNodeT*)su->config.parentNode.ptr;
        if(node && node->status.operState == CL_AMS_OPER_STATE_DISABLED)
        {
            tdescriptor = CL_AMS_CSI_NOT_QUIESCED;
        }
        else
        {
            su = (ClAmsSUT*)comp->config.parentSU.ptr;
            if(su && 
               clAmsEntityOpPending(&su->config.entity, &su->status.entity,
                                    CL_AMS_ENTITY_OP_SWAP_ACTIVE_MPLUSN))
                tdescriptor = CL_AMS_CSI_NOT_QUIESCED;
        }
    }
            
    csiFlags = CL_AMS_CSI_FLAG_TARGET_ALL;

    /*
     * Step 2: For all the CSI assigned to comp, update their activeComp, rank
     * and transition descriptor.
     */

    for ( entityRef = clAmsEntityListGetFirst(&comp->status.csiList);
          entityRef != (ClAmsEntityRefT *) NULL;
          entityRef = clAmsEntityListGetNext(&comp->status.csiList,entityRef) )
    {
        ClAmsCompCSIRefT *csiRef = (ClAmsCompCSIRefT *) entityRef;
        ClAmsCSIT *csi = (ClAmsCSIT *) entityRef->ptr;
        ClAmsCSICompRefT *compRef;

        AMS_CALL ( clAmsPeCSITransitionHAState(
                        csi,
                        comp,
                        csiRef->haState,
                        haState) );

        csiRef->activeComp = (ClAmsEntityT *) activeComp;
        csiRef->rank = standbyRank;
        csiRef->tdescriptor = tdescriptor;

        AMS_CALL ( clAmsEntityListFindEntityRef2(
                        &csi->status.pgList, 
                        &comp->config.entity,
                        0, 
                        (ClAmsEntityRefT **)&compRef) );

        compRef->rank = standbyRank;
    }


    /*
     * Step 3: Now make the actual call to assign all CSIs to the component. This 
     * takes the component's property into account. Note, only one CSI set call is
     * made for all CSI.
     */

    switch ( comp->config.property )
    {
        case CL_AMS_COMP_PROPERTY_SA_AWARE:
        {
            ClAmsCSIDescriptorT *csiDescriptor = NULL;
            ClInvocationT invocation = 0;

            AMS_CALL ( clAmsCSIMarshalCSIDescriptor(
                            &csiDescriptor,
                            NULL,
                            csiFlags,
                            haState,
                            lastActiveComp->config.entity.name,
                            tdescriptor,
                            standbyRank) );

            AMS_CALL ( clAmsInvocationCreate(
                            CL_AMS_CSI_SET_CALLBACK,
                            comp,
                            NULL,
                            &invocation) );

            AMS_CALL ( clAmsEntityTimerStart(
                            (ClAmsEntityT *) comp,
                            CL_AMS_COMP_TIMER_CSISET) );

#ifdef AMS_CPM_INTEGRATION
 
            error = _clAmsSACSISet(
                            &comp->config.entity.name,
                            &comp->config.entity.name,
                            invocation,
                            haState,
                            *csiDescriptor);

#endif

            clAmsFreeMemory(csiDescriptor->csiAttributeList.attribute);
            clAmsFreeMemory(csiDescriptor);

            if ( error )
            {
                AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
                    ("CSI assignment to Component [%s] with HA state [%s] returned Error [0x%x]\n",
                     comp->config.entity.name.value,
                     CL_AMS_STRING_H_STATE(haState),
                     error) ); 

                return clAmsPeCompAssignCSIError(comp, error);
            }

            break;
        }

        case CL_AMS_COMP_PROPERTY_PROXIED_PREINSTANTIABLE:
        {
            if ( clAmsPeCompIsProxyReady(comp) == CL_FALSE )
            {
                AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
                    ("Component [%s] has no proxy. Defering CSI assignment\n",
                     comp->config.entity.name.value));

                return CL_OK;
            }

            ClAmsCSIDescriptorT *csiDescriptor = NULL;
            ClInvocationT invocation = 0;

            AMS_CALL ( clAmsCSIMarshalCSIDescriptor(
                            &csiDescriptor,
                            NULL,
                            csiFlags,
                            haState,
                            lastActiveComp->config.entity.name,
                            tdescriptor,
                            standbyRank) );

            AMS_CALL ( clAmsInvocationCreate(
                            CL_AMS_CSI_SET_CALLBACK,
                            comp,
                            NULL,
                            &invocation) );

            AMS_CALL ( clAmsEntityTimerStart(
                            (ClAmsEntityT *) comp,
                            CL_AMS_COMP_TIMER_CSISET) );

#ifdef AMS_CPM_INTEGRATION
 
            ClAmsCompT *proxy = (ClAmsCompT *) comp->status.proxyComp;

            error = _clAmsSACSISet(
                            &comp->config.entity.name,
                            &proxy->config.entity.name,
                            invocation,
                            haState,
                            *csiDescriptor);

#endif

            clAmsFreeMemory(csiDescriptor->csiAttributeList.attribute);
            clAmsFreeMemory(csiDescriptor);

            if ( error )
            {
                AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
                    ("CSI assignment to Component [%s] with HA state [%s] returned Error [0x%x]\n",
                     comp->config.entity.name.value,
                     CL_AMS_STRING_H_STATE(haState),
                     error) );

                return clAmsPeCompAssignCSIError(comp, error);
            }

            break;
        }

        case CL_AMS_COMP_PROPERTY_PROXIED_NON_PREINSTANTIABLE:
        case CL_AMS_COMP_PROPERTY_NON_PROXIED_NON_PREINSTANTIABLE:
        {
            /*
             * If its an active assignment, then the non-preinstantiable
             * component needs to be instantiated. For any other ha state,
             * (just standby, based on earlier checks) just pretend it 
             * succeeded, so SU assignment and rest of logic works.
             */

            if ( haState == CL_AMS_HA_STATE_ACTIVE )
            {
                AMS_CALL ( clAmsPeCompInstantiate(comp) );
            }
            else
            {
                ClInvocationT invocation = 0;

                AMS_CALL ( clAmsInvocationCreate(
                                CL_AMS_CSI_SET_CALLBACK,
                                comp,
                                NULL,
                                &invocation) );

                AMS_CALL ( clAmsPeCompAssignCSICallback(
                                comp,
                                invocation,
                                CL_OK,
                                CL_AMS_ENTITY_SWITCHOVER_IMMEDIATE));
            }

            break;
        }

        default:
        {
            AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_ERROR,
                ("Error: Invalid Property [%d] for Component [%s]. Exiting..\n",
                 comp->config.property,
                 comp->config.entity.name.value));

            return CL_AMS_RC(CL_AMS_ERR_BAD_CONFIG);
        }
    }

    return CL_OK; 
}

/*
 * clAmsPeCompAssignCSICallback
 * ----------------------------
 * An acknowledgement from a SA client that a CSI assignment has been received
 * results in a callback to this fn. The callback can happen for any of the
 * following cases: assign new csi to a component, reassign a csi again to
 * a component as active, standby or quiesced, reassign a standby csi again 
 * but with different rank.
 */

static ClRcT
amsPeCompAssignCSICallback(
        CL_IN       ClAmsCompT          *comp,
        CL_IN       ClInvocationT       invocation,
        CL_IN       ClRcT               error,
        CL_IN       ClUint32T           switchoverMode)
{
    ClAmsCSIT *affectedcsi;
    ClAmsEntityListT *csiList;
    ClAmsEntityRefT *entityRef;
    ClAmsSUT *su;
    ClAmsHAStateT haState = CL_AMS_HA_STATE_NONE;
    ClAmsInvocationT invocationData = {0};
    ClUint32T invocationsPendingForComp;
    ClBoolT invokeCSIDependency = CL_TRUE;

    AMS_CHECK_COMP ( comp );
    AMS_CHECK_SU   ( su = (ClAmsSUT *) comp->config.parentSU.ptr );

    AMS_FUNC_ENTER ( ("Component [%s]\n", comp->config.entity.name.value) );

    /*
     * If CSI set timer is no longer running, this means the timout happened 
     * before this callback and this has already been treated as an error. So 
     * ignore the callback.
     */

    /* will not work for nonpi components
    if ( ! clAmsEntityTimerIsRunning(
                (ClAmsEntityT *) comp,
                CL_AMS_COMP_TIMER_CSISET) )
    {
        AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
            ("Component [%s] CSI set timer has been cleared. Ignoring callback..\n",
            comp->config.entity.name.value));

        return CL_OK;
    }
    */

    /*
     * We can get a separate invocation callback for each CSI, so stop the
     * timer only if all invocations have been responded.
     */

    AMS_CALL ( clAmsEntityTimerStopIfCountZero(
                    (ClAmsEntityT *) comp,
                    CL_AMS_COMP_TIMER_CSISET) );

    /*
     * Get and verify the invocation data for the invocation id returned by the
     * component. 
     */

    if ( clAmsInvocationGetAndDelete(invocation, &invocationData) )
    {
        AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_ERROR,
                ("Error: Response from Component [%s] with Invocation [0x%x] is unknown. Ignoring response..\n",
                 comp->config.entity.name.value, invocation));

        return CL_OK;
    }
    
    if ( invocationData.cmd != CL_AMS_CSI_SET_CALLBACK )
    {
        AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_ERROR,
                ("Error: Response from Component [%s] with Invocation [0x%x] does not match command. Ignoring response..\n",
                 comp->config.entity.name.value, invocation));

        return CL_OK;
    }

    /*
     * Is this csi set for one CSI or all CSI? If for one CSI, create a dummy 
     * list containing only that CSI, so rest of the logic in the function 
     * can be written once for a list of CSIs.
     */

    affectedcsi = invocationData.csi;

    if ( affectedcsi )
    {
        ClAmsCompCSIRefT *csiRef;

        csiList = clHeapAllocate(sizeof (ClAmsEntityListT));

        AMS_CALL ( clAmsEntityListInstantiate(csiList,
                                              CL_AMS_ENTITY_TYPE_CSI));

        AMS_CALL ( clAmsEntityListFindEntityRef2(
                        &comp->status.csiList, 
                        &affectedcsi->config.entity,
                        0, 
                        (ClAmsEntityRefT **)&csiRef) );

        AMS_CALL ( clAmsCompAddCSIRefToCSIList(csiList,
                                               affectedcsi,
                                               csiRef->haState,
                                               CL_AMS_CSI_NEW_ASSIGN));
    }
    else
    {
        csiList = &comp->status.csiList;
    }

    // WORKQUEUE

    for ( entityRef = clAmsEntityListGetFirst(csiList);
          entityRef != (ClAmsEntityRefT *) NULL;
          entityRef = clAmsEntityListGetNext(csiList, entityRef) )
    {
        ClAmsCSIT *csi;
        ClAmsSIT *si;
        ClAmsCompCSIRefT *csiRef;
        ClAmsSUSIRefT *siRef;
        ClUint32T invocationsPendingForSI;

        AMS_CHECK_CSI ( csi = (ClAmsCSIT *) entityRef->ptr );
        AMS_CHECK_SI ( si = (ClAmsSIT *) csi->config.parentSI.ptr );

        AMS_CALL ( clAmsEntityListFindEntityRef2(
                        &comp->status.csiList,
                        &csi->config.entity,
                        0,
                        (ClAmsEntityRefT **)&csiRef) );

        AMS_CALL ( clAmsEntityListFindEntityRef2(
                        &su->status.siList,
                        &si->config.entity,
                        0,
                        (ClAmsEntityRefT **)&siRef) );

        AMS_ENTITY_LOG (comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
                ("Confirming CSI [%s] assigned to Component [%s] has HA State [%s]\n",
                 csi->config.entity.name.value,
                 comp->config.entity.name.value,
                 CL_AMS_STRING_H_STATE(csiRef->haState)));

        /*
         * If this is a proxy CSI then components proxied by this CSI need to
         * be updated depending on the ha state of this assignment. The ha
         * state is checked in the fn called.
         */

        if ( csi->config.isProxyCSI )
        {
            AMS_CALL ( amsPeCompUpdateProxiedComponents(comp, csiRef, invocationData.reassignCSI) );
        }

        /*
         * Depending on the type of HA state assigned to this CSI, we may have
         * additional work to do.
         */

        invocationsPendingForSI = clAmsInvocationsPendingForSI(si, su);

        if(haState == CL_AMS_HA_STATE_NONE)
            haState = csiRef->haState;

        if ( csiRef->haState == CL_AMS_HA_STATE_ACTIVE )
        {

#ifdef AMS_CPM_INTEGRATION

            _clAmsSAMarshalPGTrackDispatch(
                            csi,
                            comp,
                            CL_AMS_PG_ADDED);

#endif

            /*
             * If this component is a proxied non-preinstantiable component, then 
             * the csi set was actually a request to the proxy for instantiation. 
             * Pass this on as an instantiate callback. Note, its okay to invoke
             * this here within the csiList loop because the non preinstantiable 
             * will have only one CSI ever assigned to it.
             */

            if ( comp->config.property == CL_AMS_COMP_PROPERTY_PROXIED_NON_PREINSTANTIABLE )
            {
                AMS_CALL ( clAmsPeCompInstantiateCallback(comp, CL_OK) );
            }

            if ( (siRef->numActiveCSIs == si->config.numCSIs) )
            {
                invokeCSIDependency = CL_FALSE;
                if(!invocationsPendingForSI)
                {
                    CL_AMS_NOTIFICATION_PUBLISH(su,
                                                (ClAmsEntityRefT *)siRef,
                                                siRef->haState,
                                                CL_AMS_NOTIFICATION_SI_PARTIALLY_ASSIGNED,
                                                switchoverMode);
                    AMS_CALL ( clAmsPeSUAssignSICallback(su, si, CL_OK) );
                }
            }
            
        }

        if ( csiRef->haState == CL_AMS_HA_STATE_STANDBY )
        {
#ifdef AMS_CPM_INTEGRATION

            _clAmsSAMarshalPGTrackDispatch(
                            csi,
                            comp,
                            CL_AMS_PG_ADDED);

#endif

            if ( (siRef->numStandbyCSIs == si->config.numCSIs) )
            {
                invokeCSIDependency = CL_FALSE;
                if(!invocationsPendingForSI)
                {
                    CL_AMS_NOTIFICATION_PUBLISH(su,
                                                (ClAmsEntityRefT *)siRef,
                                                siRef->haState,
                                                CL_AMS_NOTIFICATION_SI_FULLY_ASSIGNED,
                                                switchoverMode);
                    AMS_CALL ( clAmsPeSUAssignSICallback(su, si, CL_OK) );
                }
            }
        }

        if ( csiRef->haState == CL_AMS_HA_STATE_QUIESCED )
        {
#ifdef AMS_CPM_INTEGRATION

            _clAmsSAMarshalPGTrackDispatch(
                            csi,
                            comp,
                            CL_AMS_PG_STATE_CHANGE);

#endif

            if ( (siRef->numQuiescedCSIs == si->config.numCSIs) )
            {
                invokeCSIDependency = CL_FALSE;
                if(!invocationsPendingForSI)
                {
                    AMS_CALL ( clAmsPeSUQuiesceSIImmediatelyCallback(
                                                                     su, si, CL_OK, switchoverMode) );
                }
            }
        }

        /*
         * Callbacks for SI
         */

        if ( !si->status.numActiveAssignments && 
             !si->status.numStandbyAssignments &&
             !invocationsPendingForSI )
        {
            AMS_CALL ( clAmsPeSISwitchoverCallback(si, CL_OK, switchoverMode) );
        }
    }


    if(error != CL_OK)
    {
        if((haState == CL_AMS_HA_STATE_ACTIVE)
            ||
           (haState == CL_AMS_HA_STATE_STANDBY))
            invokeCSIDependency = CL_FALSE;
    }
            
    /*
     * Callbacks for Component
     */
    if(invokeCSIDependency)
    {
        clAmsPeSUAssignSIDependencyCallback(su, affectedcsi, haState, switchoverMode, 
                                            invocationData.reassignCSI);
    }

    invocationsPendingForComp = clAmsInvocationsPendingForComp(comp);

    if ( !comp->status.numStandbyCSIs && 
         !comp->status.numActiveCSIs  &&
         !invocationsPendingForComp )
    {
        AMS_CALL ( clAmsPeCompSwitchoverCallback(comp, CL_OK, switchoverMode) );
    }

    if ( (comp->status.recovery == CL_AMS_RECOVERY_COMP_RESTART) &&
         !invocationsPendingForComp)
    {
        AMS_CALL ( clAmsPeCompFaultCallback(comp, CL_OK) );
    }

    /*
     * If dummy csi list was created, delete it
     */

    if ( affectedcsi )
    {
        AMS_CALL ( clAmsCompDeleteCSIRefFromCSIList(csiList, affectedcsi) );
        AMS_CALL ( clAmsEntityListTerminate(csiList) );
        clAmsFreeMemory (csiList);
    }

    return CL_OK;
}

ClRcT
clAmsPeCompAssignCSICallback(
        CL_IN       ClAmsCompT          *comp,
        CL_IN       ClInvocationT       invocation,
        CL_IN       ClRcT               error,
        CL_IN       ClUint32T           switchoverMode)
{
    AMS_FUNC_ENTER ( ("Component [%s]\n", comp->config.entity.name.value) );

    /*
     * Handle error conditions upfront.
     */

    if ( error )
    {
        return clAmsPeCompAssignCSIError(comp, error);
    }

    return amsPeCompAssignCSICallback(comp, invocation, error, switchoverMode);
}

/*
 * clAmsPeCompAssignCSITimeout
 * ---------------------------
 * This fn is called as a result of either a timeout indicating that no responses 
 * have been received from a component for _all_ pending CSI assign invocations
 * or a response was received but with an error.
 */

ClRcT
clAmsPeCompAssignCSITimeout(
        CL_IN ClAmsEntityTimerT  *timer) 
{
    ClAmsCompT *comp;

    AMS_CHECKPTR ( !timer );
    AMS_CHECK_COMP ( comp = (ClAmsCompT *) timer->entity );

#ifdef AMS_EMULATE_RMD_CALLS

    return clAmsInvocationListWalk(
                comp,
                amsPeCompAssignCSICallback,
                CL_AMS_RC(CL_ERR_TIMEOUT),
                CL_AMS_ENTITY_SWITCHOVER_IMMEDIATE,
                CL_AMS_CSI_SET_CALLBACK);

#endif

    AMS_FUNC_ENTER ( ("Component [%s]\n",comp->config.entity.name.value) );

    AMS_ENTITY_LOG (comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("CSI Assign Timer popped for Component [%s]\n",
             comp->config.entity.name.value));

    /*
     * Stop the pending healthchecks if any for the component on timeout as the 
     * component is presumably locked up in the dispatcher.
     */
    cpmCompHealthcheckStop(&comp->config.entity.name);

    return clAmsPeCompAssignCSIError(comp, CL_AMS_RC(CL_ERR_TIMEOUT));
}

/*
 * clAmsPeCompProcessPendingSetCSIs
 * --------------------------------
 * Process pending CSI set invocations. This is called if an error or timeout
 * prevents a normal CSI callback.
 */

ClRcT
clAmsPeCompProcessPendingSetCSIs(
        CL_IN       ClAmsCompT          *comp,
        CL_IN       ClInvocationT       invocation,
        CL_IN       ClRcT               error,
        CL_IN       ClUint32T           switchoverMode)
{
    ClAmsCSIT *affectedcsi;
    ClAmsEntityListT *csiList;
    ClAmsEntityRefT *entityRef;
    ClAmsSUT *su;
    ClAmsInvocationT invocationData = {0};
    ClUint32T invocationsPendingForComp;
    ClAmsHAStateT haState = CL_AMS_HA_STATE_NONE;
    ClBoolT invokeCSIDependency = CL_TRUE;

    AMS_CHECK_COMP ( comp );
    AMS_CHECK_SU   ( su = (ClAmsSUT *) comp->config.parentSU.ptr );

    AMS_FUNC_ENTER ( ("Component [%s]\n", comp->config.entity.name.value) );

    /*
     * Get and verify the invocation data for the invocation id returned by the
     * component. 
     */

    if ( clAmsInvocationGetAndDelete(invocation, &invocationData) )
    {
        AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_ERROR,
                ("Error: Invocation [0x%x] is unknown. Possible internal error. Ignoring ..\n",
                 invocation));

        return CL_OK;
    }
    
    if ( invocationData.cmd != CL_AMS_CSI_SET_CALLBACK )
    {
        AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_ERROR,
                ("Error: Invocation [0x%x] does not match command. Possible internal error. Ignoring ..\n",
                 invocation));

        return CL_OK;
    }

    /*
     * Is this csi set for one CSI or all CSI? If for one CSI, create a dummy 
     * list containing only that CSI, so rest of the logic in the function 
     * can be written once for a list of CSIs.
     */

    affectedcsi = invocationData.csi;

    if ( affectedcsi )
    {
        ClAmsCompCSIRefT *csiRef;

        csiList = clHeapAllocate(sizeof (ClAmsEntityListT));

        AMS_CALL ( clAmsEntityListInstantiate(csiList,
                                              CL_AMS_ENTITY_TYPE_CSI));

        AMS_CALL ( clAmsEntityListFindEntityRef2(
                        &comp->status.csiList, 
                        &affectedcsi->config.entity,
                        0, 
                        (ClAmsEntityRefT **)&csiRef) );

        AMS_CALL ( clAmsCompAddCSIRefToCSIList(csiList,
                                               affectedcsi,
                                               csiRef->haState,
                                               CL_AMS_CSI_NEW_ASSIGN));
    }
    else
    {
        csiList = &comp->status.csiList;
    }

    // WORKQUEUE

    for ( entityRef = clAmsEntityListGetFirst(csiList);
          entityRef != (ClAmsEntityRefT *) NULL;
          entityRef = clAmsEntityListGetNext(csiList, entityRef) )
    {
        ClAmsCSIT *csi;
        ClAmsSIT *si;
        ClAmsCompCSIRefT *csiRef;
        ClAmsSUSIRefT *siRef;
        ClUint32T invocationsPendingForSI;

        AMS_CHECK_CSI ( csi = (ClAmsCSIT *) entityRef->ptr );
        AMS_CHECK_SI ( si = (ClAmsSIT *) csi->config.parentSI.ptr );

        AMS_CALL ( clAmsEntityListFindEntityRef2(
                        &comp->status.csiList,
                        &csi->config.entity,
                        0,
                        (ClAmsEntityRefT **)&csiRef) );

        AMS_CALL ( clAmsEntityListFindEntityRef2(
                        &su->status.siList,
                        &si->config.entity,
                        0,
                        (ClAmsEntityRefT **)&siRef) );

        /*
         * Depending on the type of HA state assigned to this CSI, we may have
         * additional work to do.
         */

        invocationsPendingForSI = clAmsInvocationsPendingForSI(si, su);

        if(haState == CL_AMS_HA_STATE_NONE)
            haState = csiRef->haState;

        if ( csiRef->haState == CL_AMS_HA_STATE_ACTIVE )
        {
            if ( siRef->numActiveCSIs == si->config.numCSIs )
            {
                invokeCSIDependency = CL_FALSE;
                AMS_CALL ( clAmsPeSUAssignSIError(su, si, error) );
            }
        }

        if ( csiRef->haState == CL_AMS_HA_STATE_STANDBY )
        {
            if ( siRef->numStandbyCSIs == si->config.numCSIs )
            {
                invokeCSIDependency = CL_FALSE;
                AMS_CALL ( clAmsPeSUAssignSIError(su, si, error) );
            }
        }

        if ( csiRef->haState == CL_AMS_HA_STATE_QUIESCED )
        {
            AMS_ENTITY_LOG (comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
                    ("Self Confirming CSI [%s] assigned to Component [%s] has HA State [%s]\n",
                     csi->config.entity.name.value,
                     comp->config.entity.name.value,
                     CL_AMS_STRING_H_STATE(csiRef->haState)));

#ifdef AMS_CPM_INTEGRATION

            _clAmsSAMarshalPGTrackDispatch(
                            csi,
                            comp,
                            CL_AMS_PG_STATE_CHANGE);

#endif

            /*
             * If this is a proxy CSI then components proxied by this CSI need to
             * be updated depending on the ha state of this assignment. The ha
             * state is checked in the fn called.
             */

            if ( csi->config.isProxyCSI )
            {
                AMS_CALL ( clAmsPeCompUpdateProxiedComponents(comp, csiRef) );
            }

            if ( (siRef->numQuiescedCSIs == si->config.numCSIs) &&
                 !invocationsPendingForSI)
            {
                invokeCSIDependency = CL_FALSE;
                AMS_CALL ( clAmsPeSUQuiesceSIImmediatelyCallback(
                                                su, si, CL_OK, switchoverMode) );
            }
        }

        /*
         * Callbacks for SI
         */

        if ( !si->status.numActiveAssignments && 
             !si->status.numStandbyAssignments &&
             !invocationsPendingForSI )
        {
            AMS_CALL ( clAmsPeSISwitchoverCallback(si, CL_OK, switchoverMode) );
        }
    }

    /*
     * Callbacks for Component
     */
    /*
     * Callbacks for Component
     */
    if(error != CL_OK)
    {
        if((haState == CL_AMS_HA_STATE_ACTIVE)
           ||
           (haState == CL_AMS_HA_STATE_STANDBY))
            invokeCSIDependency = CL_FALSE;
    }

    if(invokeCSIDependency)
    {
        clAmsPeSUAssignSIDependencyCallback(su, affectedcsi, haState, switchoverMode, 
                                            invocationData.reassignCSI);
    }

    invocationsPendingForComp = clAmsInvocationsPendingForComp(comp);

    if ( !comp->status.numStandbyCSIs && 
         !comp->status.numActiveCSIs  &&
         !invocationsPendingForComp )
    {
        AMS_CALL ( clAmsPeCompSwitchoverCallback(comp, CL_OK, switchoverMode) );
    }

    if ( (comp->status.recovery == CL_AMS_RECOVERY_COMP_RESTART) &&
         !invocationsPendingForComp)
    {
        AMS_CALL ( clAmsPeCompFaultCallback(comp, error) );
    }

    /*
     * If dummy csi list was created, delete it
     */

    if ( affectedcsi )
    {
        AMS_CALL ( clAmsCompDeleteCSIRefFromCSIList(csiList, affectedcsi) );
        AMS_CALL ( clAmsEntityListTerminate(csiList) );
        clAmsFreeMemory (csiList);
    }

    return CL_OK;
}

/*
 * clAmsPeCompAssignCSIError
 * -------------------------
 * This fn is called when an error is detected while trying to assign a CSI to
 * a component.
 */

ClRcT
clAmsPeCompAssignCSIError(
        CL_IN ClAmsCompT  *comp,
        CL_IN ClRcT error) 
{
    ClAmsLocalRecoveryT recovery;
    ClUint32T escalation;
    ClRcT rc = CL_OK;

    AMS_CHECK_COMP ( comp );

    AMS_FUNC_ENTER ( ("Component [%s]\n",comp->config.entity.name.value) );

    AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_ERROR,
        ("CSI assignment to Component [%s] returned Error [0x%x]\n",
         comp->config.entity.name.value, error) );

    AMS_CALL ( clAmsEntityTimerStop(
                    (ClAmsEntityT *) comp,
                    CL_AMS_COMP_TIMER_CSISET) );

    /*
     * Go through the pending invocations for this component and reinvoke
     * CSI quiesced complete, but with a switchover mode of fast. This 
     * will cause all CSIs waiting for quiesced state to be confirmed as 
     * well as any necessary actions taken as if the CSI quiescing had 
     * succeeded.
     */
    CL_AMS_SET_O_STATE(comp, CL_AMS_OPER_STATE_DISABLED);

    clAmsFaultQueueAdd((ClAmsEntityT*)comp);
    
    clAmsInvocationListWalk(
                comp,
                clAmsPeCompProcessPendingSetCSIs,
                error,
                CL_AMS_ENTITY_SWITCHOVER_FAST,
                CL_AMS_CSI_SET_CALLBACK);

    if ( comp->config.property == CL_AMS_COMP_PROPERTY_PROXIED_NON_PREINSTANTIABLE )
    {
        // XXX Restarting can mean instantiating. For now this is okay as the
        // action is same in both cases. This should not matter once restarting
        // is changed to a separate flag.

        if ( comp->status.presenceState == CL_AMS_PRESENCE_STATE_INSTANTIATING )
        {
            AMS_CALL ( clAmsPeCompInstantiateError(comp, error) );
            clAmsFaultQueueDelete((ClAmsEntityT*)comp);
            return CL_OK;
        }

        if ( (comp->status.presenceState == CL_AMS_PRESENCE_STATE_TERMINATING) ||
             (comp->status.presenceState == CL_AMS_PRESENCE_STATE_RESTARTING) )
        {
            AMS_CALL ( clAmsPeCompTerminateError(comp, error) );
            clAmsFaultQueueDelete((ClAmsEntityT*)comp);
            return CL_OK;
        }
    }

    /*
     * Report a fault on the component
     */

    recovery = CL_AMS_RECOVERY_NO_RECOMMENDATION;
    escalation = clAmsPeEntityComputeFaultEscalation((ClAmsEntityT*) comp);
    rc = clAmsPeCompFaultReport(comp, &recovery, &escalation);
    clAmsFaultQueueDelete((ClAmsEntityT*)comp);
    return rc;
}

/*
 * clAmsPeCompQuiesceCSIGracefully
 * -------------------------------
 * Given a component and a CSI, this fn informs the component to mark the 
 * CSI as quiescing.
 */

ClRcT
clAmsPeCompQuiesceCSIGracefullyExtended(
        CL_IN       ClAmsCompT *comp,
        CL_IN       ClAmsCSIT *csi,
        CL_IN       ClUint32T switchoverMode,
        CL_IN       ClBoolT reassignCSI)
{
    ClAmsCompCSIRefT *csiRef;
    ClRcT rc;
    ClRcT error = CL_OK;

    AMS_CHECK_COMP ( comp );
    AMS_CHECK_CSI ( csi );
 
    AMS_FUNC_ENTER ( ("Component [%s]\n", comp->config.entity.name.value) );

    /*
     * Step 1: Verify that this CSI is already assigned to the component and
     * has active HA state.
     */

    rc = clAmsEntityListFindEntityRef2(
                    &comp->status.csiList, 
                    &csi->config.entity,
                    0, 
                    (ClAmsEntityRefT **)&csiRef);

    if ( CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST )
    {
        AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
            ("Component [%s] is not assigned CSI [%s]. Ignoring [Quiesce CSI Gracefully] request..\n",
            comp->config.entity.name.value,
            csi->config.entity.name.value));

        return CL_OK;
    }

    if ( csiRef->haState == CL_AMS_HA_STATE_ACTIVE )
    {
        AMS_ENTITY_LOG (comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
                 ("Marking CSI [%s] assigned to Component [%s] as [Quiescing]\n",
                  csi->config.entity.name.value,
                  comp->config.entity.name.value));
    }
    else if ( csiRef->haState == CL_AMS_HA_STATE_QUIESCING )
    {
        AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
            ("(Replay) Marking CSI [%s] assigned to Component [%s] as [Quiescing]\n",
             csi->config.entity.name.value,
             comp->config.entity.name.value));
    }
    else
    {
        AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
            ("CSI [%s] assigned to Component [%s] is not Active or Quiescing. Ignoring..\n",
             csi->config.entity.name.value,
             comp->config.entity.name.value));

        return CL_OK;
    }

    /*
     * Update the Component, CSI and SIs view of this CSI assignment.
     */

    AMS_CALL ( clAmsPeCSITransitionHAStateExtended(
                    csi,
                    comp,
                    csiRef->haState,
                    CL_AMS_HA_STATE_QUIESCING,
                    switchoverMode) );

    /*
     * Step 2: Now make the actual call to quiece the CSI. This takes the
     * component's property into account.
     */

    ClInvocationT invocation = 0;

    switch ( comp->config.property )
    {
        case CL_AMS_COMP_PROPERTY_SA_AWARE:
        {
            ClAmsCSIDescriptorT *csiDescriptor;
            ClNameT activeCompName = {0};

            activeCompName.length = 0;

            AMS_CALL (clAmsCSIMarshalCSIDescriptor(
                            &csiDescriptor,
                            csi,
                            CL_AMS_CSI_FLAG_TARGET_ONE,
                            csiRef->haState,
                            activeCompName,
                            CL_AMS_CSI_NOT_QUIESCED,
                            0));

            AMS_CALL ( clAmsInvocationCreateExtended(
                        CL_AMS_CSI_QUIESCING_CALLBACK,
                        comp,
                        csi,
                        reassignCSI,
                        &invocation) );

            AMS_CALL ( clAmsEntityTimerStart(
                            (ClAmsEntityT *) comp,
                            CL_AMS_COMP_TIMER_QUIESCINGCOMPLETE) );

#ifdef AMS_CPM_INTEGRATION

            if ( !(switchoverMode & CL_AMS_ENTITY_SWITCHOVER_FAST ))
            {
                error = _clAmsSACSISet(
                                &comp->config.entity.name,
                                &comp->config.entity.name,
                                invocation,
                                CL_AMS_HA_STATE_QUIESCING,
                                *csiDescriptor);
            }

#endif

            clAmsFreeMemory(csiDescriptor->csiAttributeList.attribute);
            clAmsFreeMemory(csiDescriptor);

            if ( error )
            {
                AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
                    ("CSI assignment to Component [%s] with HA state [%s] returned Error [0x%x]\n",
                     comp->config.entity.name.value,
                     CL_AMS_STRING_H_STATE(CL_AMS_HA_STATE_QUIESCING),
                     error) );

                return clAmsPeCompQuiesceCSIGracefullyError(comp, error);
            }

            break;
        }

        case CL_AMS_COMP_PROPERTY_PROXIED_PREINSTANTIABLE:
        {
            if ( clAmsPeCompIsProxyReady(comp) == CL_FALSE )
            {
                AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
                    ("Component [%s] has no proxy. Marking CSI quiescing\n",
                     comp->config.entity.name.value));

                switchoverMode = CL_AMS_ENTITY_SWITCHOVER_FAST;
            }

            ClAmsCSIDescriptorT *csiDescriptor;
            ClNameT activeCompName = {0};

            activeCompName.length = 0;

            AMS_CALL (clAmsCSIMarshalCSIDescriptor(
                            &csiDescriptor,
                            csi,
                            CL_AMS_CSI_FLAG_TARGET_ONE,
                            csiRef->haState,
                            activeCompName,
                            CL_AMS_CSI_NOT_QUIESCED,
                            0));

            AMS_CALL ( clAmsInvocationCreateExtended(
                        CL_AMS_CSI_QUIESCING_CALLBACK,
                        comp,
                        csi,
                        reassignCSI,
                        &invocation) );

            AMS_CALL ( clAmsEntityTimerStart(
                        (ClAmsEntityT *) comp,
                        CL_AMS_COMP_TIMER_QUIESCINGCOMPLETE) );

#ifdef AMS_CPM_INTEGRATION
 
            ClAmsCompT *proxy = (ClAmsCompT *) comp->status.proxyComp;

            if ( !(switchoverMode & CL_AMS_ENTITY_SWITCHOVER_FAST ))
            {
                error = _clAmsSACSISet(
                                &comp->config.entity.name,
                                &proxy->config.entity.name,
                                invocation,
                                CL_AMS_HA_STATE_QUIESCING,
                                *csiDescriptor);
            }

#endif

            clAmsFreeMemory(csiDescriptor->csiAttributeList.attribute);
            clAmsFreeMemory(csiDescriptor);

            if ( error )
            {
                AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
                    ("CSI assignment to Component [%s] with HA state [%s] returned Error [0x%x]\n",
                     comp->config.entity.name.value,
                     CL_AMS_STRING_H_STATE(CL_AMS_HA_STATE_QUIESCING),
                     error) );

                return clAmsPeCompQuiesceCSIGracefullyError(comp, error);
            }

            break;
        }

        case CL_AMS_COMP_PROPERTY_PROXIED_NON_PREINSTANTIABLE:
        case CL_AMS_COMP_PROPERTY_NON_PROXIED_NON_PREINSTANTIABLE:
        {

            AMS_CALL ( clAmsInvocationCreateExtended(
                                             CL_AMS_CSI_QUIESCING_CALLBACK,
                                             comp,
                                             csi,
                                             reassignCSI,
                                             &invocation) );

            AMS_CALL ( clAmsEntityTimerStart(
                        (ClAmsEntityT *) comp,
                        CL_AMS_COMP_TIMER_QUIESCINGCOMPLETE) );

            if ( !(switchoverMode & CL_AMS_ENTITY_SWITCHOVER_FAST ))
            {
                AMS_CALL ( clAmsPeCompQuiescingCompleteCallback(
                                                    comp,
                                                    invocation,
                                                    CL_OK,
                                                    switchoverMode));

                AMS_CALL ( clAmsPeCompTerminate(comp) );
            }

            break;
        }

        default:
        {
            AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_ERROR,
                ("Error: Invalid Property [%d] for Component [%s]. Exiting..\n",
                 comp->config.property,
                 comp->config.entity.name.value));

            return CL_AMS_RC(CL_AMS_ERR_BAD_CONFIG);
        }
    }

    if ( (switchoverMode & CL_AMS_ENTITY_SWITCHOVER_FAST ))
    {
        AMS_CALL ( clAmsPeCompQuiescingCompleteCallback(
                        comp,
                        invocation,
                        CL_OK,
                        switchoverMode) );
    }

    return CL_OK;
}

ClRcT
clAmsPeCompQuiesceCSIGracefully(
        CL_IN       ClAmsCompT *comp,
        CL_IN       ClAmsCSIT *csi,
        CL_IN       ClUint32T switchoverMode)
{
    return clAmsPeCompQuiesceCSIGracefullyExtended(comp, csi, switchoverMode, CL_FALSE);
}

/*
 * clAmsPeCompQuiesceCSIImmediately
 * --------------------------------
 * Given a component and a CSI, this fn informs the component to mark the 
 * CSI as quiescing.
 */

ClRcT
clAmsPeCompQuiesceCSIImmediatelyExtended(
        CL_IN       ClAmsCompT *comp,
        CL_IN       ClAmsCSIT *csi,
        CL_IN       ClUint32T switchoverMode,
        CL_IN       ClBoolT reassignCSI)
{
    ClAmsCompCSIRefT *csiRef;
    ClRcT rc;
    ClRcT error = CL_OK;

    AMS_CHECK_COMP ( comp );
    AMS_CHECK_CSI ( csi );
 
    AMS_FUNC_ENTER ( ("Component [%s]\n", comp->config.entity.name.value) );

    /*
     * Step 1: Verify that this CSI is already assigned to the component and
     * has active or quiescing HA state.
     */

    rc = clAmsEntityListFindEntityRef2(
                    &comp->status.csiList, 
                    &csi->config.entity,
                    0, 
                    (ClAmsEntityRefT **)&csiRef);

    if ( CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST )
    {
        AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
            ("Component [%s] is not assigned CSI [%s]. Ignoring [Quiesce CSI Immediately] request..\n",
            comp->config.entity.name.value,
            csi->config.entity.name.value));

        return CL_OK;
    }

    if ( (csiRef->haState == CL_AMS_HA_STATE_ACTIVE) || 
         (csiRef->haState == CL_AMS_HA_STATE_QUIESCING) )
    {
        AMS_ENTITY_LOG (comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
                 ("Marking CSI [%s] assigned to Component [%s] as [Quiesced]\n",
                  csi->config.entity.name.value,
                  comp->config.entity.name.value));

    }
    else if ( csiRef->haState == CL_AMS_HA_STATE_QUIESCED )
    {
        AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("(Replay) Marking CSI [%s] assigned to Component [%s] as [Quiesced]\n",
             csi->config.entity.name.value,
             comp->config.entity.name.value));
    }
    else
    {
        AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("CSI [%s] assigned to Component [%s] is not Active, Quiescing or Quiesced. Ignoring..\n",
             csi->config.entity.name.value,
             comp->config.entity.name.value));

        return CL_OK;
    }

    /*
     * Update the Component, CSI and SIs view of this CSI assignment.
     */

    AMS_CALL ( clAmsPeCSITransitionHAStateExtended(
                    csi,
                    comp,
                    csiRef->haState,
                    CL_AMS_HA_STATE_QUIESCED,
                    switchoverMode) );

    /*
     * Step 2: Now make the actual call to quiesce the CSI. This takes the 
     * component's property into account.
     */

    ClInvocationT invocation = 0;

    switch ( comp->config.property )
    {
        case CL_AMS_COMP_PROPERTY_SA_AWARE:
        {
            ClAmsCSIDescriptorT *csiDescriptor;
            ClNameT activeCompName = {0};

            activeCompName.length = 0;

            AMS_CALL (clAmsCSIMarshalCSIDescriptor(
                            &csiDescriptor,
                            csi,
                            CL_AMS_CSI_FLAG_TARGET_ONE,
                            csiRef->haState,
                            activeCompName,
                            CL_AMS_CSI_QUIESCED,
                            0));

            AMS_CALL ( clAmsInvocationCreateExtended(
                            CL_AMS_CSI_SET_CALLBACK,
                            comp,
                            csi,
                            reassignCSI,
                            &invocation) );

            AMS_CALL ( clAmsEntityTimerStart(
                            (ClAmsEntityT *) comp,
                            CL_AMS_COMP_TIMER_CSISET) );

#ifdef AMS_CPM_INTEGRATION

            if (!( switchoverMode & CL_AMS_ENTITY_SWITCHOVER_FAST ))
            {
                error = _clAmsSACSISet(
                                &comp->config.entity.name,
                                &comp->config.entity.name,
                                invocation,
                                CL_AMS_HA_STATE_QUIESCED,
                                *csiDescriptor);
            }

#endif

            clAmsFreeMemory(csiDescriptor->csiAttributeList.attribute);
            clAmsFreeMemory(csiDescriptor);

            if ( error )
            {
                AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
                    ("CSI assignment to Component [%s] with HA state [%s] returned Error [0x%x]\n",
                     comp->config.entity.name.value,
                     CL_AMS_STRING_H_STATE(CL_AMS_HA_STATE_QUIESCED),
                     error) );

                return clAmsPeCompAssignCSIError(comp, error);
            }

            break;
        }

        case CL_AMS_COMP_PROPERTY_PROXIED_PREINSTANTIABLE:
        {
            if ( clAmsPeCompIsProxyReady(comp) == CL_FALSE )
            {
                AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
                    ("Component [%s] has no proxy. Marking CSI quiesced\n",
                     comp->config.entity.name.value));

                switchoverMode = CL_AMS_ENTITY_SWITCHOVER_FAST;
            }

            ClAmsCSIDescriptorT *csiDescriptor;
            ClNameT activeCompName = {0};

            activeCompName.length = 0;

            AMS_CALL (clAmsCSIMarshalCSIDescriptor(
                            &csiDescriptor,
                            csi,
                            CL_AMS_CSI_FLAG_TARGET_ONE,
                            csiRef->haState,
                            activeCompName,
                            CL_AMS_CSI_QUIESCED,
                            0));

            AMS_CALL ( clAmsInvocationCreateExtended(
                            CL_AMS_CSI_SET_CALLBACK,
                            comp,
                            csi,
                            reassignCSI,
                            &invocation) );

            AMS_CALL ( clAmsEntityTimerStart(
                            (ClAmsEntityT *) comp,
                            CL_AMS_COMP_TIMER_CSISET) );

#ifdef AMS_CPM_INTEGRATION
 
            ClAmsCompT *proxy = (ClAmsCompT *) comp->status.proxyComp;

            if ( !(switchoverMode & CL_AMS_ENTITY_SWITCHOVER_FAST ))
            {
                error = _clAmsSACSISet(
                                &comp->config.entity.name,
                                &proxy->config.entity.name,
                                invocation,
                                CL_AMS_HA_STATE_QUIESCED,
                                *csiDescriptor);
            }

#endif

            clAmsFreeMemory(csiDescriptor->csiAttributeList.attribute);
            clAmsFreeMemory(csiDescriptor);

            if ( error )
            {
                AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
                    ("CSI assignment to Component [%s] with HA state [%s] returned Error [0x%x]\n",
                     comp->config.entity.name.value,
                     CL_AMS_STRING_H_STATE(CL_AMS_HA_STATE_QUIESCED),
                     error) );

                return clAmsPeCompAssignCSIError(comp, error);
            }

            break;
        }

        case CL_AMS_COMP_PROPERTY_PROXIED_NON_PREINSTANTIABLE:
        case CL_AMS_COMP_PROPERTY_NON_PROXIED_NON_PREINSTANTIABLE:
        {

            AMS_CALL ( clAmsInvocationCreateExtended(
                                             CL_AMS_CSI_SET_CALLBACK,
                                             comp,
                                             csi,
                                             reassignCSI,
                                             &invocation) );

            AMS_CALL ( clAmsEntityTimerStart(
                            (ClAmsEntityT *) comp,
                            CL_AMS_COMP_TIMER_CSISET) );

            if (!( switchoverMode & CL_AMS_ENTITY_SWITCHOVER_FAST ))
            {
                AMS_CALL ( clAmsPeCompAssignCSICallback(
                                                    comp,
                                                    invocation,
                                                    CL_OK,
                                                    switchoverMode));
                AMS_CALL ( clAmsPeCompTerminate(comp) );
            }
            break;
        }

        default:
        {
            AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_ERROR,
                ("Error: Invalid Property [%d] for Component [%s]. Exiting..\n",
                 comp->config.property,
                 comp->config.entity.name.value));

            return CL_AMS_RC(CL_AMS_ERR_BAD_CONFIG);
        }
    }

    if ( (switchoverMode & CL_AMS_ENTITY_SWITCHOVER_FAST ))
    {
        AMS_CALL ( clAmsPeCompAssignCSICallback(
                        comp,
                        invocation,
                        CL_OK,
                        switchoverMode) );
    }

    return CL_OK;
}

ClRcT
clAmsPeCompQuiesceCSIImmediately(
        CL_IN       ClAmsCompT *comp,
        CL_IN       ClAmsCSIT *csi,
        CL_IN       ClUint32T switchoverMode)
{
    return clAmsPeCompQuiesceCSIImmediatelyExtended(comp, csi, switchoverMode, CL_FALSE);
}

/*
 * clAmsPeCompQuiescingCSICallback -  If the component responds back with
 * cpm response, then stop the timer as quiescing complete can be delayed.
 */

ClRcT
clAmsPeCompQuiescingCSICallback(
                                ClAmsCompT *comp,
                                ClInvocationT invocation,
                                ClRcT error,
                                ClUint32T switchoverMode)
{
    ClAmsInvocationT invocationData = {0};
    ClRcT rc = CL_OK;

    AMS_CHECK_COMP(comp);

    rc = clAmsInvocationGet(invocation, &invocationData);
    if(rc != CL_OK)
    {
        AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_ERROR,
                ("Error: Quiescing Response from Component [%s] with Invocation [0x%x] is unknown. "
                 "Ignoring response..\n",
                 comp->config.entity.name.value, invocation));

        return CL_OK;
    }

    if ( invocationData.cmd != CL_AMS_CSI_QUIESCING_CALLBACK )
    {
        AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_ERROR,
                ("Error: Quiescing response from Component [%s] with Invocation [0x%x] does not match command."
                 "Ignoring response\n",
                 comp->config.entity.name.value, invocation));

        return CL_OK;
    }

    if ( error )
    {
        AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
            ("CSI quiescing from Component [%s] returned Error [0x%x]\n",
             comp->config.entity.name.value,
             error) );

        return clAmsPeCompQuiescingCompleteTimeout(
                    &comp->status.timers.quiescingComplete);
    }

    AMS_CALL ( clAmsEntityTimerStopIfCountZero(
                    (ClAmsEntityT *) comp,
                    CL_AMS_COMP_TIMER_QUIESCINGCOMPLETE) );

    return CL_OK;
}

/*
 * clAmsPeCompQuiescingCompleteCallback
 * ------------------------------------
 * This fn is called when a response is received from a component in
 * reaction to a CSI set invocation with ha state set to quiescing.
 * The quiescing complete response indicates if the previously assigned
 * active state to one or more CSIs has been successfully quiesced. If 
 * quiescing is successful, this fn then assigns the active state for 
 * the affected CSIs to the first ranked standby and the quiesced CSI 
 * is removed.
 */

static ClRcT
amsPeCompQuiescingCompleteCallback(
        CL_IN       ClAmsCompT          *comp,
        CL_IN       ClInvocationT       invocation,
        CL_IN       ClRcT               error,
        CL_IN       ClUint32T           switchoverMode)
{
    ClAmsCSIT *affectedcsi;
    ClAmsEntityListT *csiList;
    ClAmsEntityRefT *entityRef;
    ClAmsSUT *su;
    ClAmsInvocationT invocationData = {0};
    ClUint32T invocationsPendingForComp;
    ClBoolT invokeCSIDependency = CL_TRUE;

    AMS_CHECK_COMP ( comp );
    AMS_CHECK_SU ( su = (ClAmsSUT *) comp->config.parentSU.ptr );

    AMS_FUNC_ENTER ( ("Component [%s]\n", comp->config.entity.name.value) );

    /*
     * If CSI quiesce timer is no longer running, this means the timeout 
     * happened before this callback and this has already been treated 
     * as an error. So ignore the callback.
     */

    /* will not work for npi components
    if ( ! clAmsEntityTimerIsRunning(
                (ClAmsEntityT *) comp,
                CL_AMS_COMP_TIMER_QUIESCINGCOMPLETE) )
    {
        AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
            ("Component [%s] CSI quiescing timer has been cleared. Ignoring callback..\n",
            comp->config.entity.name.value));

        return CL_OK;
    }
    */

    /*
     * Get and verify the invocation data for the invocation id returned by the
     * component. 
     */

    if ( clAmsInvocationGetAndDelete(invocation, &invocationData) )
    {
        AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_ERROR,
                ("Error: Response from Component [%s] with Invocation [0x%x] is unknown. Ignoring response..\n",
                 comp->config.entity.name.value, invocation));

        return CL_OK;
    }
    
    if ( invocationData.cmd != CL_AMS_CSI_QUIESCING_CALLBACK )
    {
        AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_ERROR,
                ("Error: Response from Component [%s] with Invocation [0x%x] does not match command. Ignoring response..\n",
                 comp->config.entity.name.value, invocation));

        return CL_OK;
    }

    /*
     * We can get a separate quiescing callback for each pending quiesce
     * invocation, so we stop the timer only when all quiescing responses
     * have been received.
     */

    AMS_CALL ( clAmsEntityTimerStopIfCountZero(
                    (ClAmsEntityT *) comp,
                    CL_AMS_COMP_TIMER_QUIESCINGCOMPLETE) );

    /*
     * Is this quiescing complete for one CSI or all CSI? If for one CSI,
     * create a dummy list containing only that CSI, so rest of the logic
     * in the function can be written once for a list of CSIs.
     *
     * The quiescing complete will be for one CSI when a SI change is
     * made but will be for all CSI when a switchover is done.
     */

    affectedcsi = invocationData.csi;

    if ( affectedcsi )
    {
        csiList = clHeapAllocate(sizeof (ClAmsEntityListT));

        AMS_CALL ( clAmsEntityListInstantiate(csiList,
                                              CL_AMS_ENTITY_TYPE_CSI));

        AMS_CALL ( clAmsCompAddCSIRefToCSIList(csiList,
                                               affectedcsi,
                                               CL_AMS_HA_STATE_QUIESCING,
                                               CL_AMS_CSI_NEW_ASSIGN));
    }
    else
    {
        csiList = &comp->status.csiList;
    }

    /*
     * Update CSIs and their respective siRef and SUs.
     */

    for ( entityRef = clAmsEntityListGetFirst(csiList);
          entityRef != (ClAmsEntityRefT *) NULL;
          entityRef = clAmsEntityListGetNext(csiList, entityRef) )
    {
        ClAmsCSIT *csi;
        ClAmsSIT *si;
        ClAmsCompCSIRefT *csiRef;
        ClAmsSUSIRefT *siRef;
        ClUint32T invocationsPendingForSI;

        AMS_CHECK_CSI ( csi = (ClAmsCSIT *) entityRef->ptr );
        AMS_CHECK_SI ( si = (ClAmsSIT *) csi->config.parentSI.ptr );

        AMS_CALL ( clAmsEntityListFindEntityRef2(
                        &comp->status.csiList,
                        &csi->config.entity,
                        0,
                        (ClAmsEntityRefT **)&csiRef) );

        AMS_CALL ( clAmsEntityListFindEntityRef2(
                        &su->status.siList,
                        &si->config.entity,
                        0,
                        (ClAmsEntityRefT **)&siRef) );

        invocationsPendingForSI = clAmsInvocationsPendingForSI(si, su);

        if ( csiRef->haState == CL_AMS_HA_STATE_QUIESCING )
        {
            AMS_ENTITY_LOG (comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
                ("%sConfirming CSI [%s] assigned to Component [%s] has HA State [Quiesced]\n",
                 (switchoverMode & CL_AMS_ENTITY_SWITCHOVER_FAST) ? "Self " : "",
                 csi->config.entity.name.value,
                 comp->config.entity.name.value));

            AMS_CALL ( clAmsPeCSITransitionHAStateExtended(
                            csi,
                            comp,
                            CL_AMS_HA_STATE_QUIESCING,
                            CL_AMS_HA_STATE_QUIESCED,
                            switchoverMode) );

#ifdef AMS_CPM_INTEGRATION

            _clAmsSAMarshalPGTrackDispatch(
                    csi,
                    comp,
                    CL_AMS_PG_STATE_CHANGE);

#endif

            if ( (siRef->numQuiescedCSIs == si->config.numCSIs) )
            {
                invokeCSIDependency = CL_FALSE;
                if(!invocationsPendingForSI)
                {
                    AMS_CALL ( clAmsPeSUQuiesceSIGracefullyCallback(
                                                                    su, si, CL_OK, switchoverMode) );
                }
            }
        }
        else
        {
            if ( csiRef->haState != CL_AMS_HA_STATE_QUIESCED )
            {
                return CL_OK;
            }
        }

        /*
         * If this is a proxy CSI then components proxied by this CSI need to
         * be updated depending on the ha state of this assignment. The ha
         * state is checked in the fn called.
         */

        if ( csi->config.isProxyCSI )
        {
            AMS_CALL ( clAmsPeCompUpdateProxiedComponents(comp, csiRef) );
        }

        /*
         * Callbacks for SI
         */

        if ( !si->status.numActiveAssignments && 
             !si->status.numStandbyAssignments &&
             !invocationsPendingForSI )
        {
            AMS_CALL ( clAmsPeSISwitchoverCallback(si, CL_OK, switchoverMode) );
        }
    }

    /*
     * Callbacks for Component
     */
    if(invokeCSIDependency)
    {
        clAmsPeSUAssignSIDependencyCallback(su, affectedcsi, CL_AMS_HA_STATE_QUIESCING, switchoverMode, 
                                            invocationData.reassignCSI);
    }

    invocationsPendingForComp = clAmsInvocationsPendingForComp(comp);

    if ( !comp->status.numStandbyCSIs && 
         !comp->status.numActiveCSIs  &&
         !invocationsPendingForComp )
    {
        AMS_CALL ( clAmsPeCompSwitchoverCallback(comp, CL_OK, switchoverMode) );
    }

    /*
     * If dummy csi list was created, delete it
     */

    if ( affectedcsi )
    {
        AMS_CALL ( clAmsCompDeleteCSIRefFromCSIList(csiList, affectedcsi) );
        AMS_CALL ( clAmsEntityListTerminate(csiList) );
        clAmsFreeMemory (csiList);
    }

    return CL_OK;
}

ClRcT
clAmsPeCompQuiescingCompleteCallback(
        CL_IN       ClAmsCompT          *comp,
        CL_IN       ClInvocationT       invocation,
        CL_IN       ClRcT               error,
        CL_IN       ClUint32T           switchoverMode)
{
    AMS_FUNC_ENTER ( ("Component [%s]\n", comp->config.entity.name.value) );

    /*
     * If error, treat this as a timeout.
     */

    if ( error )
    {
        AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
            ("CSI quiescing from Component [%s] returned Error [0x%x]\n",
             comp->config.entity.name.value,
             error) );

        return clAmsPeCompQuiescingCompleteTimeout(
                    &comp->status.timers.quiescingComplete);
    }

    return amsPeCompQuiescingCompleteCallback(comp, invocation, error, switchoverMode);
}

/*
 * clAmsPeCompProcessPendingQuiescingCSIs
 * --------------------------------------
 * Process pending CSI invocations that are trying to mark the CSI as quiescing.
 * This is called if an error or timeout prevents a normal quiescing complete
 * callback.
 */

ClRcT
clAmsPeCompProcessPendingQuiescingCSIs(
        CL_IN       ClAmsCompT          *comp,
        CL_IN       ClInvocationT       invocation,
        CL_IN       ClRcT               error,
        CL_IN       ClUint32T           switchoverMode)
{
    ClAmsCSIT *affectedcsi;
    ClAmsEntityListT *csiList;
    ClAmsEntityRefT *entityRef;
    ClAmsSUT *su;
    ClAmsInvocationT invocationData = {0};
    ClUint32T invocationsPendingForComp;

    AMS_CHECK_COMP ( comp );
    AMS_CHECK_SU ( su = (ClAmsSUT *) comp->config.parentSU.ptr );

    AMS_FUNC_ENTER ( ("Component [%s]\n", comp->config.entity.name.value) );

    /*
     * Get and verify the invocation data for the invocation id returned by the
     * component. 
     */

    if ( clAmsInvocationGetAndDelete(invocation, &invocationData) )
    {
        AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_ERROR,
                ("Error: Invocation [0x%x] is unknown. Possible internal error. Ignoring ..\n",
                 invocation));

        return CL_OK;
    }
    
    if ( invocationData.cmd != CL_AMS_CSI_QUIESCING_CALLBACK )
    {
        AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_ERROR,
                ("Error: Invocation [0x%x] does not match command. Possible internal error. Ignoring ..\n",
                 invocation));

        return CL_OK;
    }

    /*
     * Is this quiescing complete for one CSI or all CSI? If for one CSI,
     * create a dummy list containing only that CSI, so rest of the logic
     * in the function can be written once for a list of CSIs.
     *
     * The quiescing complete will be for one CSI when a SI change is
     * made but will be for all CSI when a switchover is done.
     */

    affectedcsi = invocationData.csi;

    if ( affectedcsi )
    {
        csiList = clHeapAllocate(sizeof (ClAmsEntityListT));

        AMS_CALL ( clAmsEntityListInstantiate(csiList,
                                              CL_AMS_ENTITY_TYPE_CSI));

        AMS_CALL ( clAmsCompAddCSIRefToCSIList(csiList,
                                               affectedcsi,
                                               CL_AMS_HA_STATE_QUIESCING,
                                               CL_AMS_CSI_NEW_ASSIGN));
    }
    else
    {
        csiList = &comp->status.csiList;
    }

    /*
     * Update CSIs and their respective siRef and SUs.
     */

    for ( entityRef = clAmsEntityListGetFirst(csiList);
          entityRef != (ClAmsEntityRefT *) NULL;
          entityRef = clAmsEntityListGetNext(csiList, entityRef) )
    {
        ClAmsCSIT *csi;
        ClAmsSIT *si;
        ClAmsCompCSIRefT *csiRef;
        ClAmsSUSIRefT *siRef;
        ClUint32T invocationsPendingForSI;

        AMS_CHECK_CSI ( csi = (ClAmsCSIT *) entityRef->ptr );
        AMS_CHECK_SI ( si = (ClAmsSIT *) csi->config.parentSI.ptr );

        AMS_CALL ( clAmsEntityListFindEntityRef2(
                        &comp->status.csiList,
                        &csi->config.entity,
                        0,
                        (ClAmsEntityRefT **)&csiRef) );

        AMS_CALL ( clAmsEntityListFindEntityRef2(
                        &su->status.siList,
                        &si->config.entity,
                        0,
                        (ClAmsEntityRefT **)&siRef) );

        invocationsPendingForSI = clAmsInvocationsPendingForSI(si, su);

        if ( csiRef->haState == CL_AMS_HA_STATE_QUIESCING )
        {
            AMS_CALL ( clAmsPeCSITransitionHAStateExtended(
                            csi,
                            comp,
                            CL_AMS_HA_STATE_QUIESCING,
                            CL_AMS_HA_STATE_QUIESCED,
                            switchoverMode) );

            AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
                ("Self Confirming CSI [%s] assigned to Component [%s] has HA State [Quiesced]\n",
                 csi->config.entity.name.value,
                 comp->config.entity.name.value) ); 

#ifdef AMS_CPM_INTEGRATION

            _clAmsSAMarshalPGTrackDispatch(
                    csi,
                    comp,
                    CL_AMS_PG_STATE_CHANGE);

#endif

            if ( (siRef->numQuiescedCSIs == si->config.numCSIs) &&
                 !invocationsPendingForSI)
            {
                AMS_CALL ( clAmsPeSUQuiesceSIGracefullyCallback(
                                                su, si, CL_OK, switchoverMode) );
            }
        }
        else
        {
            if ( csiRef->haState != CL_AMS_HA_STATE_QUIESCED )
            {
                return CL_OK;
            }
        }

        /*
         * If this is a proxy CSI then components proxied by this CSI need to
         * be updated depending on the ha state of this assignment. The ha
         * state is checked in the fn called.
         */

        if ( csi->config.isProxyCSI )
        {
            AMS_CALL ( clAmsPeCompUpdateProxiedComponents(comp, csiRef) );
        }

        /*
         * Callbacks for SI
         */

        if ( !si->status.numActiveAssignments && 
             !si->status.numStandbyAssignments &&
             !invocationsPendingForSI )
        {
            AMS_CALL ( clAmsPeSISwitchoverCallback(si, CL_OK, switchoverMode) );
        }
    }

    /*
     * Callbacks for Component
     */

    invocationsPendingForComp = clAmsInvocationsPendingForComp(comp);

    if ( !comp->status.numStandbyCSIs && 
         !comp->status.numActiveCSIs  &&
         !invocationsPendingForComp )
    {
        AMS_CALL ( clAmsPeCompSwitchoverCallback(comp, CL_OK, switchoverMode) );
    }

    /* Not necessary - restart does not include quiescing as action
    if ( (comp->status.recovery == CL_AMS_RECOVERY_COMP_RESTART) &&
         !invocationsPendingForComp)
    {
        AMS_CALL ( clAmsPeCompFaultCallback(comp, CL_AMS_RC(CL_ERR_TIMEOUT)) );
    }
    */

    /*
     * If dummy csi list was created, delete it
     */

    if ( affectedcsi )
    {
        AMS_CALL ( clAmsCompDeleteCSIRefFromCSIList(csiList, affectedcsi) );
        AMS_CALL ( clAmsEntityListTerminate(csiList) );
        clAmsFreeMemory (csiList);
    }

    return CL_OK;
}

/*
 * clAmspeCompQuiescingCompleteTimeout
 * -----------------------------------
 * This fn is called when the quiescing complete timer pops.
 */

ClRcT
clAmsPeCompQuiescingCompleteTimeout(
        CL_IN ClAmsEntityTimerT  *timer) 
{
    ClAmsCompT *comp;

    AMS_CHECKPTR ( !timer );
    AMS_CHECK_COMP ( comp = (ClAmsCompT *) timer->entity );

#ifdef AMS_EMULATE_RMD_CALLS

    return clAmsInvocationListWalk(
                comp,
                amsPeCompQuiescingCompleteCallback,
                CL_AMS_RC(CL_ERR_TIMEOUT),
                CL_AMS_ENTITY_SWITCHOVER_GRACEFUL,
                CL_AMS_CSI_QUIESCING_CALLBACK);

#endif

    AMS_FUNC_ENTER ( ("Component [%s]\n",comp->config.entity.name.value) );

    AMS_ENTITY_LOG (comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("CSI Quiescing Timer popped for Component [%s]\n",
             comp->config.entity.name.value));

    /*
     * Stop the pending healthchecks if any for the component on timeout as the 
     * component is presumably locked up in the dispatcher.
     */
    cpmCompHealthcheckStop(&comp->config.entity.name);

    return clAmsPeCompQuiesceCSIGracefullyError(comp, CL_AMS_RC(CL_ERR_TIMEOUT));
}

/*
 * clAmsPeCompQuiesceCSIGracefullyError
 * ------------------------------------
 * This fn is called when an error is detected while trying to assign a CSI to
 * a component with HA state quiescing.
 */

ClRcT
clAmsPeCompQuiesceCSIGracefullyError(
        CL_IN ClAmsCompT  *comp,
        CL_IN ClRcT error) 
{
    ClRcT rc = CL_OK;
    ClAmsLocalRecoveryT recovery;
    ClUint32T escalation;

    AMS_CHECK_COMP ( comp );

    AMS_FUNC_ENTER ( ("Component [%s]\n",comp->config.entity.name.value) );

    AMS_ENTITY_LOG (comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_ERROR,
            ("Error [0x%x] while trying to quiesce CSI assigned to Component [%s]\n",
             error,
             comp->config.entity.name.value));

    AMS_CALL ( clAmsEntityTimerStop(
                    (ClAmsEntityT *) comp,
                    CL_AMS_COMP_TIMER_QUIESCINGCOMPLETE) );

    /*
     * Go through the pending invocations for this component and reinvoke
     * CSI quiescing complete, but with a switchover mode of fast. This 
     * will cause all CSIs pending quiescing to be quiesced as well as any 
     * necessary actions taken as if the CSI quiescing had succeeded.
     */
    CL_AMS_SET_O_STATE(comp, CL_AMS_OPER_STATE_DISABLED);

    clAmsFaultQueueAdd((ClAmsEntityT*)comp);
    clAmsInvocationListWalk(
                comp,
                clAmsPeCompProcessPendingQuiescingCSIs,
                error,
                CL_AMS_ENTITY_SWITCHOVER_FAST,
                CL_AMS_CSI_QUIESCING_CALLBACK);

    if ( comp->config.property == CL_AMS_COMP_PROPERTY_PROXIED_NON_PREINSTANTIABLE )
    {
        if ( (comp->status.presenceState == CL_AMS_PRESENCE_STATE_TERMINATING) ||
             (comp->status.presenceState == CL_AMS_PRESENCE_STATE_RESTARTING) )
        {
            AMS_CALL ( clAmsPeCompTerminateError(comp, error) );
            clAmsFaultQueueDelete((ClAmsEntityT*)comp);
            return CL_OK;
        }
    }

    /*
     * Report a fault on the component
     */

    recovery = CL_AMS_RECOVERY_NO_RECOMMENDATION;
    escalation = clAmsPeEntityComputeFaultEscalation((ClAmsEntityT*) comp);
    rc = clAmsPeCompFaultReport(comp, &recovery, &escalation);
    clAmsFaultQueueDelete((ClAmsEntityT*)comp);
    return rc;
}

/* clAmsPeCompRemoveCSI
 * --------------------
 * This fn is called to remove a CSI assigned to a component. The CSI can be
 * in active, standby, quiescing or quiesced state.
 */

ClRcT
clAmsPeCompRemoveCSI(
        CL_IN   ClAmsCompT *comp,
        CL_IN   ClAmsCSIT *csi,
        CL_IN   ClUint32T switchoverMode)
{
    ClAmsCompCSIRefT *csiRef;
    ClRcT rc;
    ClRcT error = CL_OK;

    AMS_CHECK_COMP ( comp );
    AMS_CHECK_CSI ( csi );

    AMS_FUNC_ENTER ( ("Component [%s]\n", comp->config.entity.name.value) );

    clAmsPeCompComputeSwitchoverMode(comp, &switchoverMode);
    
    /*
     * Step 1: Verify that this CSI is already assigned to the component.
     */

    rc = clAmsEntityListFindEntityRef2(
                    &comp->status.csiList,
                    &csi->config.entity,
                    0,
                    (ClAmsEntityRefT **)&csiRef);

    if ( CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST )
    {
        AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
            ("Component [%s] is not assigned CSI [%s]. Ignoring remove request..\n",
            comp->config.entity.name.value,
            csi->config.entity.name.value));

        return CL_OK;
    }

    if ( csiRef->pendingOp == CL_AMS_CSI_RMV_CALLBACK )
    {
        AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("CSI [%s] assigned to Component [%s] is already marked for removal\n",
             csi->config.entity.name.value,
             comp->config.entity.name.value));

        return CL_OK;
    }

    AMS_ENTITY_LOG (comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
             ("Marking CSI [%s] assigned to Component [%s] as [Removed]\n",
              csi->config.entity.name.value,
              comp->config.entity.name.value));

    /*
     * Step 2: Now make the actual call to quiesce the CSI. This takes the 
     * component's property into account.
     */

    ClInvocationT invocation = 0;

    switch ( comp->config.property )
    {
        case CL_AMS_COMP_PROPERTY_SA_AWARE:
        {

            ClAmsCSIDescriptorT *csiDescriptor;
            ClNameT activeCompName = {0};

            activeCompName.length = 0;

            AMS_CALL ( clAmsCSIMarshalCSIDescriptor(
                            &csiDescriptor,
                            csi,
                            CL_AMS_CSI_FLAG_TARGET_ONE,
                            csiRef->haState,
                            activeCompName,
                            ( (csiRef->haState == CL_AMS_HA_STATE_QUIESCED) ||
                              (csiRef->haState == CL_AMS_HA_STATE_STANDBY)  ) ? 
                                    CL_AMS_CSI_QUIESCED : CL_AMS_CSI_NOT_QUIESCED,
                            0) );
            
            AMS_CALL ( clAmsInvocationCreate(
                            CL_AMS_CSI_RMV_CALLBACK,
                            comp,
                            csi,
                            &invocation) );

            AMS_CALL ( clAmsEntityTimerStart(
                            (ClAmsEntityT *) comp,
                            CL_AMS_COMP_TIMER_CSIREMOVE) );

#ifdef AMS_CPM_INTEGRATION
            if ( !(switchoverMode & CL_AMS_ENTITY_SWITCHOVER_FAST ))
            {
                error = _clAmsSACSIRemove(
                            &comp->config.entity.name,
                            &comp->config.entity.name,
                            invocation,
                            *csiDescriptor);
            }
#endif

            clAmsFreeMemory(csiDescriptor->csiAttributeList.attribute);
            clAmsFreeMemory(csiDescriptor);
            
            if ( error )
                return clAmsPeCompRemoveCSIError(comp, error);

            break;
        }

        case CL_AMS_COMP_PROPERTY_PROXIED_PREINSTANTIABLE:
        {
            if ( !comp->status.proxyComp )
            {
                AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
                    ("Component [%s] has no proxy. Marking CSI removed\n",
                     comp->config.entity.name.value));

                switchoverMode = CL_AMS_ENTITY_SWITCHOVER_FAST;
            }

            ClAmsCSIDescriptorT *csiDescriptor;
            ClNameT activeCompName = {0};

            activeCompName.length = 0;

            AMS_CALL ( clAmsCSIMarshalCSIDescriptor(
                            &csiDescriptor,
                            csi,
                            CL_AMS_CSI_FLAG_TARGET_ONE,
                            csiRef->haState,
                            activeCompName,
                            ( (csiRef->haState == CL_AMS_HA_STATE_QUIESCED) ||
                              (csiRef->haState == CL_AMS_HA_STATE_STANDBY)  ) ? 
                                    CL_AMS_CSI_QUIESCED : CL_AMS_CSI_NOT_QUIESCED,
                            0) );
            
            AMS_CALL ( clAmsInvocationCreate(
                            CL_AMS_CSI_RMV_CALLBACK,
                            comp,
                            csi,
                            &invocation) );

            AMS_CALL ( clAmsEntityTimerStart(
                        (ClAmsEntityT *) comp,
                        CL_AMS_COMP_TIMER_CSIREMOVE) );

#ifdef AMS_CPM_INTEGRATION
 
            ClAmsCompT *proxy = (ClAmsCompT *) comp->status.proxyComp;

            if (!( switchoverMode & CL_AMS_ENTITY_SWITCHOVER_FAST ))
            {
                error =  _clAmsSACSIRemove(
                                           &comp->config.entity.name,
                                           &proxy->config.entity.name,
                                           invocation,
                                           *csiDescriptor);
            }

#endif

            clAmsFreeMemory(csiDescriptor->csiAttributeList.attribute);
            clAmsFreeMemory(csiDescriptor);

            if ( error )
                return clAmsPeCompRemoveCSIError(comp, error);

            break;
        }

        case CL_AMS_COMP_PROPERTY_PROXIED_NON_PREINSTANTIABLE:
        case CL_AMS_COMP_PROPERTY_NON_PROXIED_NON_PREINSTANTIABLE:
        {

            AMS_CALL ( clAmsInvocationCreate(
                                             CL_AMS_CSI_RMV_CALLBACK,
                                             comp,
                                             csi,
                                             &invocation) );

            AMS_CALL ( clAmsEntityTimerStart(
                                             (ClAmsEntityT *) comp,
                                             CL_AMS_COMP_TIMER_CSIREMOVE) );

            /*
             * If the ha state is active, then terminate the component,
             * otherwise, just pretend the csi remove succeeded.
             */

            if (!( switchoverMode & CL_AMS_ENTITY_SWITCHOVER_FAST ))
            {
                if(comp->status.proxyComp)
                {
                    AMS_CALL ( clAmsPeCompTerminate(comp) );
                }
                else
                {
                    AMS_CALL ( clAmsPeCompRemoveCSICallback(
                                                            comp,
                                                            invocation,
                                                            CL_OK,
                                                            switchoverMode) );
                    AMS_CALL ( clAmsPeCompTerminate(comp) );
                }
            }

            break;
        }

        default:
        {
            AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_ERROR,
                ("Error: Invalid Property [%d] for Component [%s]. Exiting..\n",
                 comp->config.property,
                 comp->config.entity.name.value));

            return CL_AMS_RC(CL_AMS_ERR_BAD_CONFIG);
        }
    }

    if ( (switchoverMode & CL_AMS_ENTITY_SWITCHOVER_FAST ))
    {
        AMS_CALL ( clAmsPeCompRemoveCSICallback(
                        comp,
                        invocation,
                        CL_OK,
                        switchoverMode) );
    }

    return CL_OK;
}


/*
 * clAmsPeCompRemoveCSICallback
 * ----------------------------
 * This fn is called when an indication is received from a client that a CSI
 * has been removed. Note, an appropriate wrapper must fill in switchoverMode.
 */

static ClRcT
amsPeCompRemoveCSICallback(
                           CL_IN       ClAmsCompT          *comp,
                           CL_IN       ClInvocationT       invocation,
                           CL_IN       ClRcT               error,
                           CL_IN       ClUint32T           switchoverMode)
{
    ClAmsCSIT *affectedcsi;
    ClAmsEntityListT *csiList;
    ClAmsEntityRefT *entityRef;
    ClAmsSUT *su;
    ClAmsSGT *sg;
    ClAmsSIT *lastSI = NULL;
    ClAmsInvocationT invocationData = {0};
    ClUint32T invocationsPendingForComp;
    ClBoolT invokeCSIDependency = CL_TRUE;
    ClUint32T quiescedCSIs = 0;

    AMS_CHECK_COMP ( comp );
    AMS_CHECK_SU ( su = (ClAmsSUT *) comp->config.parentSU.ptr );
    AMS_CHECK_SG ( sg = (ClAmsSGT *)su->config.parentSG.ptr );

    AMS_FUNC_ENTER ( ("Component [%s]\n", comp->config.entity.name.value) );

    /*
     * If CSI remove timer is no longer running, this means the timeout 
     * happened before this callback and this has already been treated 
     * as an error. So ignore the callback.
     */

    /* will not work for npi components
       if ( ! clAmsEntityTimerIsRunning(
       (ClAmsEntityT *) comp,
       CL_AMS_COMP_TIMER_CSIREMOVE) )
       {
       AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_TRACE,
       ("Component [%s] CSI remove timer has been cleared. Ignoring callback..\n",
       comp->config.entity.name.value));

       return CL_OK;
       }
    */

    /*
     * We can get a separate csi remove callback for each pending remove
     * invocation, so we stop the timer only when all remove responses
     * have been received.
     */

    AMS_CALL ( clAmsEntityTimerStopIfCountZero(
                                               (ClAmsEntityT *) comp,
                                               CL_AMS_COMP_TIMER_CSIREMOVE) );

    /*
     * Get and verify the invocation data for the invocation id returned by the
     * component. 
     */

    if ( clAmsInvocationGetAndDelete(invocation, &invocationData) )
    {
        AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_ERROR,
                       ("Error: Response from Component [%s] with Invocation [0x%x] is unknown. Ignoring response..\n",
                        comp->config.entity.name.value, invocation));

        return CL_OK;
    }
    
    /*
     * invocationData.cmd may be something other than remove if the component
     * returned an invalid invocation or because this fn was called from CSI
     * remove timeout.
     */

    if ( invocationData.cmd != CL_AMS_CSI_RMV_CALLBACK )
    {
        AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_ERROR,
                       ("Error: Response from Component [%s] with Invocation [0x%x] does not match command. Ignoring response..\n",
                        comp->config.entity.name.value, invocation));

        return CL_OK;
    }

    /*
     * Is this remove for one CSI or all CSI? If for one CSI, create a dummy 
     * list containing only that CSI, so rest of the logic in the function 
     * can be written once for a list of CSIs.
     */

    affectedcsi = invocationData.csi;

    if ( affectedcsi )
    {
        csiList = clHeapAllocate(sizeof (ClAmsEntityListT));

        AMS_CALL ( clAmsEntityListInstantiate(csiList,
                                              CL_AMS_ENTITY_TYPE_CSI));

        AMS_CALL ( clAmsCompAddCSIRefToCSIList(csiList,
                                               affectedcsi,
                                               CL_AMS_HA_STATE_QUIESCING,
                                               CL_AMS_CSI_NEW_ASSIGN));
    }
    else
    {
        csiList = &comp->status.csiList;
    }

    // WORKQUEUE

    for ( entityRef = clAmsEntityListGetFirst(csiList);
          entityRef != (ClAmsEntityRefT *) NULL;
          entityRef = clAmsEntityListGetNext(csiList, entityRef) )
    {
        ClAmsCSIT *csi;
        ClAmsSIT *si;
        ClAmsCompCSIRefT *csiRef;
        ClAmsSUSIRefT *siRef;
        ClUint32T invocationsPendingForSI;
        ClAmsHAStateT currentHaState ;

        AMS_CHECK_CSI ( csi = (ClAmsCSIT *) entityRef->ptr );
        AMS_CHECK_SI ( si = (ClAmsSIT *) csi->config.parentSI.ptr );

        AMS_CALL ( clAmsEntityListFindEntityRef2(
                                                 &comp->status.csiList,
                                                 &csi->config.entity,
                                                 0,
                                                 (ClAmsEntityRefT **)&csiRef) );

        AMS_CALL ( clAmsEntityListFindEntityRef2(
                                                 &su->status.siList,
                                                 &si->config.entity,
                                                 0,
                                                 (ClAmsEntityRefT **)&siRef) );

        /*
         * Update the Component, CSI, SI and SU view of this assignment.
         */
        currentHaState = csiRef->haState;

        AMS_CALL ( clAmsPeCSITransitionHAStateExtended(
                                               csi,
                                               comp,
                                               csiRef->haState,
                                               CL_AMS_HA_STATE_NONE,
                                               switchoverMode) );

        AMS_ENTITY_LOG (comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
                        ("%sConfirming CSI [%s] assigned to Component [%s] is [Removed]\n",
                         (switchoverMode & CL_AMS_ENTITY_SWITCHOVER_FAST) ? "Self " : "",
                         csi->config.entity.name.value,
                         comp->config.entity.name.value));

#ifdef AMS_CPM_INTEGRATION

        _clAmsSAMarshalPGTrackDispatch(
                                       csi,
                                       comp,
                                       CL_AMS_PG_REMOVED);

#endif

        /*
         * If this is a proxy CSI then components proxied by this CSI need to
         * be updated depending on the ha state of this assignment. The ha
         * state is checked in the fn called.
         */

        if ( csi->config.isProxyCSI )
        {
            AMS_CALL ( clAmsPeCompUpdateProxiedComponents(comp, csiRef) );
        }

        AMS_CALL (clAmsCompDeleteCSIRefFromCSIList(&comp->status.csiList, csi));
        AMS_CALL (clAmsCSIDeleteCompRefFromPGList(&csi->status.pgList, comp));

        if ( comp->config.property == CL_AMS_COMP_PROPERTY_PROXIED_NON_PREINSTANTIABLE )
        {
            AMS_CALL ( clAmsPeCompTerminateCallback(comp, CL_OK) );
        }

        invocationsPendingForSI = clAmsInvocationsPendingForSI(si, su);
        
        if ( siRef && 
             (siRef->haState == CL_AMS_HA_STATE_NONE))
        {
            invokeCSIDependency = CL_FALSE;
            if(!invocationsPendingForSI)
            {
                ClAmsSUSIRefT tSiRef;
                ClAmsNotificationTypeT event = CL_AMS_NOTIFICATION_SI_UNASSIGNED;
                memcpy(&tSiRef, siRef, sizeof(tSiRef));
                AMS_CALL ( clAmsSUDeleteSIRefFromSIList(&su->status.siList, si) );
                AMS_CALL ( clAmsSIDeleteSURefFromSUList(&si->status.suList, su) );
                if(si->status.numActiveAssignments > 0 
                   || 
                   si->status.numStandbyAssignments > 0)
                {
                    event = CL_AMS_NOTIFICATION_SI_PARTIALLY_ASSIGNED;
                }
                CL_AMS_NOTIFICATION_PUBLISH(su,
                                            (ClAmsEntityRefT *)&tSiRef,
                                            currentHaState,
                                            event,
                                            switchoverMode);
                clLogDebug("SU_SI", "REMOVE", "SU remove SI callback for SU [%s], SI [%s]",
                           su->config.entity.name.value, si->config.entity.name.value);
                AMS_CALL ( clAmsPeSURemoveSICallback(su, si, error, switchoverMode) );
            }
        }

        /*
         * Callbacks for SI if there are no pending invocations across SGs.
         * We do this for standby CSIs since active CSIs with standby removed
         * would have anyway updated the SI dependent list apart from triggering
         * the SI work switchover. So to avoid redundant dependent SI updates,
         * we do only for standby CSIs.
         */
        if(currentHaState == CL_AMS_HA_STATE_STANDBY 
           &&
           !si->status.numStandbyAssignments
           &&
           (si != lastSI))
        {
            ClAmsEntityRefT *eRef = NULL;
            ClAmsSGT *sg;
            AMS_CHECK_SG ( sg = (ClAmsSGT*) si->config.parentSG.ptr );
            /* 
             * Minor optimisation glue for csi remove all to cache the last SI.
             *
             */
            lastSI = si;
            invocationsPendingForSI = 0;
            for ( eRef = clAmsEntityListGetFirst(&sg->status.assignedSUList);
                  eRef != (ClAmsEntityRefT *) NULL;
                  eRef = clAmsEntityListGetNext(&sg->status.assignedSUList, eRef) )
            {
                ClAmsSUT *tmpSU = (ClAmsSUT *) eRef->ptr;
                invocationsPendingForSI += clAmsInvocationsPendingForSI(si, tmpSU);
            }

            if(!invocationsPendingForSI)
            {
                AMS_CALL ( clAmsPeSISwitchoverCallback(si, CL_OK, switchoverMode) );
            }
        }
    }

    invocationsPendingForComp = clAmsInvocationsPendingForComp(comp);

    quiescedCSIs = comp->status.numQuiescedCSIs;
    /*
     * In user controlled redundancy mode, SI could have both 
     * active+standby assignments and active could have been quiesced first.
     */
    if(sg->config.redundancyModel == CL_AMS_SG_REDUNDANCY_MODEL_CUSTOM)
    {
        quiescedCSIs = 0;
    }

    if ( !comp->status.numStandbyCSIs && 
         !comp->status.numActiveCSIs && 
         !quiescedCSIs &&
         !invocationsPendingForComp
         && 
         !(switchoverMode & CL_AMS_ENTITY_SWITCHOVER_SWAP) )
    {
        if( (!(switchoverMode & CL_AMS_ENTITY_SWITCHOVER_SU))
            ||
            !su->status.numQuiescedSIs)
        {
            AMS_CALL ( clAmsPeCompSwitchoverCallback(comp, error, switchoverMode) );
        }
    }

    /*
     * Callbacks for Component. Note, we may make duplicate calls to comp
     * switchovercallback, once after all CSIs are quiesced and once after 
     * they are removed.
     */
    if(invokeCSIDependency
       &&
       !(switchoverMode & CL_AMS_ENTITY_SWITCHOVER_SWAP))
    {
        clAmsPeSUAssignSIDependencyCallback(su, affectedcsi, CL_AMS_HA_STATE_NONE, switchoverMode,
                                            invocationData.reassignCSI);
    }

    /*
     * If dummy csi list was created, delete it
     */

    if ( affectedcsi )
    {
        AMS_CALL ( clAmsCompDeleteCSIRefFromCSIList(csiList, affectedcsi) );
        AMS_CALL ( clAmsEntityListTerminate(csiList) );
        clAmsFreeMemory (csiList);
    }

    if(!(switchoverMode & (CL_AMS_ENTITY_SWITCHOVER_SWAP)))
    {
        /*
         * We evaluate the work for the SG since this could be a remove for a standby
         * after reassignment to active (MPLUSN ->reassign to active + remove standby assignment)
         * This delayed standby remove could have had impact on a spare trying to take over as 
         * standby.
         */
        AMS_CALL ( clAmsPeSGEvaluateWork(sg) );
    }
    return CL_OK;
}

ClRcT
clAmsPeCompRemoveCSICallback(
                             CL_IN       ClAmsCompT          *comp,
                             CL_IN       ClInvocationT       invocation,
                             CL_IN       ClRcT               error,
                             CL_IN       ClUint32T           switchoverMode)
{
    AMS_FUNC_ENTER ( ("Component [%s]\n", comp->config.entity.name.value) );

    /*
     * Handle error condition upfront.
     */

    if ( error )
    {
        return clAmsPeCompRemoveCSIError(comp, error);
    }

    return amsPeCompRemoveCSICallback(comp, invocation, error, switchoverMode);
}

static ClRcT
clAmsPeReplayCSIRemoveCallbacks(ClAmsInvocationT *pInvocations,
                                ClInt32T numInvocations,
                                ClAmsCSIReplayFilterT *filters,
                                ClUint32T numFilters,
                                ClUint32T switchoverMode)
{
    ClRcT rc = CL_OK;
    ClInt32T i;

    AMS_FUNC_ENTER(("\n"));

    AMS_LOG(CL_DEBUG_CRITICAL, ("Replaying [%d] CSI remove invocations\n",
                                numInvocations));

    for(i = 0; i < numInvocations; ++i)
    {
        ClAmsSUT *su = NULL;
        ClAmsCompT *comp = NULL;
        ClAmsNodeT *node = NULL;
        ClAmsCSIT *affectedcsi = NULL;
        ClAmsEntityListT *csiList = NULL;
        ClAmsInvocationT *pInvocation = pInvocations+i;
        ClAmsEntityRefT entityRef;
        ClAmsEntityRefT *pEntityRef = NULL;
        ClAmsEntityRefT *pNext = NULL;
        ClBoolT clearInvocation = CL_FALSE;

        if(!pInvocation->invocation) continue; /*skip cleared/processed invocations*/

        memset(&entityRef, 0, sizeof(entityRef));

        memcpy(&entityRef.entity.name,
               &pInvocation->compName,
               sizeof(entityRef.entity.name));

        entityRef.entity.type = CL_AMS_ENTITY_TYPE_COMP;
    
        rc = clAmsEntityDbFindEntity(&gAms.db.entityDb[CL_AMS_ENTITY_TYPE_COMP],
                                     &entityRef);

        if(rc != CL_OK)
        {
            pInvocation->invocation = 0;
            AMS_LOG(CL_DEBUG_ERROR, ("Unable to find comp [%.*s]\n",
                                     pInvocation->compName.length,
                                     pInvocation->compName.value));
            continue;
        }
    
        AMS_CHECK_COMP( comp = (ClAmsCompT*) entityRef.ptr);
        
        AMS_CHECK_SU( su = (ClAmsSUT*) comp->config.parentSU.ptr);

        AMS_CHECK_NODE ( node = (ClAmsNodeT*) su->config.parentNode.ptr);

        if(filters)
        {
            ClUint32T c;
            for(c = 0; c < numFilters; ++c)
                if(!filters[c].node ||
                   !strncmp(node->config.entity.name.value, filters[c].node->value,
                            filters[c].node->length))
                {
                    clearInvocation = filters[c].clearInvocation;
                    goto matched;
                }

            continue; /*unmatched*/
        }

        matched:
        if(clearInvocation)
        {
            ClAmsInvocationT invocationData = {0};
            if( !(switchoverMode & CL_AMS_ENTITY_SWITCHOVER_FAST) )
            {
                (void)amsPeCompRemoveCSICallback(comp, pInvocation->invocation, CL_OK, switchoverMode);
                pInvocation->invocation = 0;
                continue;
            }
            clAmsInvocationGetAndDelete(pInvocation->invocation, &invocationData);
        }
        
        clLogNotice("CSI", "REPLAY", "Replaying CSI [%s] remove invocation [%#llx] for component [%.*s]",
                    pInvocation->csi ? pInvocation->csi->config.entity.name.value : "All",
                    pInvocation->invocation, 
                    comp->config.entity.name.length,
                    comp->config.entity.name.value);

        pInvocation->invocation = 0;

        /*
         * Is this remove for one CSI or all CSI? If for one CSI, create a dummy 
         * list containing only that CSI, so rest of the logic in the function 
         * can be written once for a list of CSIs.
         */
        clAmsEntityTimerStopIfCountZero(
                                        (ClAmsEntityT *) comp,
                                        CL_AMS_COMP_TIMER_CSIREMOVE);
        affectedcsi = pInvocation->csi;
        if ( affectedcsi )
        {
            csiList = clHeapAllocate(sizeof (ClAmsEntityListT));

            AMS_CALL ( clAmsEntityListInstantiate(csiList,
                                                  CL_AMS_ENTITY_TYPE_CSI));

            AMS_CALL ( clAmsCompAddCSIRefToCSIList(csiList,
                                                   affectedcsi,
                                                   CL_AMS_HA_STATE_QUIESCING,
                                                   CL_AMS_CSI_NEW_ASSIGN));
        }
        else
        {
            csiList = &comp->status.csiList;
        }

        for ( pEntityRef = clAmsEntityListGetFirst(csiList);
              pEntityRef != (ClAmsEntityRefT *) NULL;
              pEntityRef = pNext )
        {
            ClAmsCSIT *csi;
            ClAmsSIT *si;
            ClAmsCompCSIRefT *csiRef;
            ClAmsSUSIRefT *siRef;
            ClUint32T invocationsPendingForSI;

            pNext = clAmsEntityListGetNext(csiList, pEntityRef);

            AMS_CHECK_CSI ( csi = (ClAmsCSIT *) pEntityRef->ptr );
            AMS_CHECK_SI ( si = (ClAmsSIT *) csi->config.parentSI.ptr );

            if ( clAmsEntityListFindEntityRef2(
                                                     &comp->status.csiList,
                                                     &csi->config.entity,
                                                     0,
                                                     (ClAmsEntityRefT **)&csiRef) != CL_OK)
                goto next;

            if ( clAmsEntityListFindEntityRef2(
                                                     &su->status.siList,
                                                     &si->config.entity,
                                                     0,
                                                     (ClAmsEntityRefT **)&siRef) != CL_OK)
                goto next;
            /*
             * Update the Component, CSI, SI and SU view of this assignment.
             */

            if ( clAmsPeCSITransitionHAStateExtended(
                                                   csi,
                                                   comp,
                                                   csiRef->haState,
                                                   CL_AMS_HA_STATE_NONE,
                                                   switchoverMode) != CL_OK)
                goto next;

            AMS_ENTITY_LOG (comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
                            ("Confirming CSI [%s] assigned to Component [%s] is [Removed]\n",
                             csi->config.entity.name.value,
                             comp->config.entity.name.value));

#ifdef AMS_CPM_INTEGRATION

            _clAmsSAMarshalPGTrackDispatch(
                                           csi,
                                           comp,
                                           CL_AMS_PG_REMOVED);

#endif

            /*
             * If this is a proxy CSI then components proxied by this CSI need to
             * be updated depending on the ha state of this assignment. The ha
             * state is checked in the fn called.
             */

            if ( csi->config.isProxyCSI )
            {
                if(clAmsPeCompUpdateProxiedComponents(comp, csiRef) != CL_OK)
                    goto next;
            }

            if(clAmsCompDeleteCSIRefFromCSIList(&comp->status.csiList, csi) != CL_OK)
                goto next;

            if(clAmsCSIDeleteCompRefFromPGList(&csi->status.pgList, comp) != CL_OK)
                goto next;

            invocationsPendingForSI = clAmsInvocationsPendingForSI(si, su);
        
            if ( siRef && 
                 (siRef->haState == CL_AMS_HA_STATE_NONE) &&
                 !invocationsPendingForSI )
            {
                if(clAmsSUDeleteSIRefFromSIList(&su->status.siList, si) != CL_OK)
                    goto next;
                    
                if(clAmsSIDeleteSURefFromSUList(&si->status.suList, su) != CL_OK)
                    goto next;
            }
        }

        /*
         * If dummy csi list was created, delete it
         */
    next:
        if ( affectedcsi )
        {
            AMS_CALL ( clAmsCompDeleteCSIRefFromCSIList(csiList, affectedcsi) );
            AMS_CALL ( clAmsEntityListTerminate(csiList) );
            clAmsFreeMemory (csiList);
        }
    }

    return CL_OK;
}

/*
 * clAmsPeCompRemoveCSIError
 * -------------------------
 * This fn is called when the remove csi operation fails either immediately
 * while removing, or due to a timeout or a callback is received with an
 * error.
 */

ClRcT
clAmsPeCompRemoveCSIError(
        CL_IN ClAmsCompT *comp,
        CL_IN ClRcT error) 
{
    ClRcT rc = CL_OK;
    ClAmsLocalRecoveryT recovery;
    ClUint32T escalation;

    AMS_FUNC_ENTER ( ("Component [%s]\n",comp->config.entity.name.value) );

    AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_ERROR,
        ("CSI removal from Component [%s] returned Error [0x%x]\n",
         comp->config.entity.name.value, error) ) 

    AMS_CALL ( clAmsEntityTimerStop(
                    (ClAmsEntityT *) comp,
                    CL_AMS_COMP_TIMER_CSIREMOVE) );

    /*
     * Go through the pending invocations for this component and reinvoke
     * CSI remove, but with a switchover mode of fast. This will cause all
     * CSIs pending removal to be removed as well as any necessary actions 
     * taken as if the CSI removal had succeeded.
     */
    clAmsFaultQueueAdd((ClAmsEntityT*)comp);
    clAmsInvocationListWalk(
                comp,
                amsPeCompRemoveCSICallback,
                error,
                CL_AMS_ENTITY_SWITCHOVER_FAST,
                CL_AMS_CSI_RMV_CALLBACK);

    if ( comp->config.property == CL_AMS_COMP_PROPERTY_PROXIED_NON_PREINSTANTIABLE )
    {
        if ( (comp->status.presenceState == CL_AMS_PRESENCE_STATE_TERMINATING) ||
             (comp->status.presenceState == CL_AMS_PRESENCE_STATE_RESTARTING) )
        {
            AMS_CALL ( clAmsPeCompTerminateError(comp, error) );
            clAmsFaultQueueDelete((ClAmsEntityT*)comp);
            return CL_OK;
        }
    }

    /*
     * Report a fault on the component
     */

    recovery = CL_AMS_RECOVERY_NO_RECOMMENDATION;
    escalation = clAmsPeEntityComputeFaultEscalation((ClAmsEntityT*) comp);

    rc = clAmsPeCompFaultReport(comp, &recovery, &escalation);
    clAmsFaultQueueDelete((ClAmsEntityT*)comp);
    return rc;
}

/*
 * clAmsPeCompRemoveCSITimeout
 * ---------------------------
 * This fn is called when the remove csi timer pops.
 */

ClRcT
clAmsPeCompRemoveCSITimeout(
        CL_IN ClAmsEntityTimerT  *timer) 
{
    ClAmsCompT *comp;

    AMS_CHECKPTR ( !timer );
    AMS_CHECK_COMP ( comp = (ClAmsCompT *) timer->entity );

#ifdef AMS_EMULATE_RMD_CALLS

    return clAmsInvocationListWalk(
                comp,
                amsPeCompRemoveCSICallback,
                CL_AMS_RC(CL_ERR_TIMEOUT),
                CL_AMS_ENTITY_SWITCHOVER_IMMEDIATE,
                CL_AMS_CSI_RMV_CALLBACK);

#endif

    AMS_FUNC_ENTER ( ("Component [%s]\n",comp->config.entity.name.value) );

    AMS_ENTITY_LOG (comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("CSI Remove Timer popped for Component [%s]\n",
             comp->config.entity.name.value));

    /*
     * Stop the healthchecks if any for the component on timeout as the 
     * component is presumably locked up in the dispatcher.
     */
    cpmCompHealthcheckStop(&comp->config.entity.name);

    return clAmsPeCompRemoveCSIError(comp, CL_AMS_RC(CL_ERR_TIMEOUT));
}

/*-----------------------------------------------------------------------------
 * Utility Functions
 *---------------------------------------------------------------------------*/
 
/*
 * clAmsPeCompComputeReadinessState
 * --------------------------------
 * Compute the readiness state of a component based on other states.
 */

ClRcT
clAmsPeCompComputeReadinessState(
        CL_IN       ClAmsCompT *comp,
        CL_OUT      ClAmsReadinessStateT *compState)
{
    AMS_CHECK_COMP ( comp );
    AMS_CHECKPTR ( !compState );

    AMS_FUNC_ENTER ( ("Component [%s]\n", comp->config.entity.name.value) );

    switch ( comp->status.operState )
    {
        case CL_AMS_OPER_STATE_DISABLED:
        {
            *compState = CL_AMS_READINESS_STATE_OUTOFSERVICE;
            break;
        }

        case CL_AMS_OPER_STATE_ENABLED:
        {
            // For now, force a recomputation of the SU readiness state
            // but this could be removed in the future.

            ClAmsSUT *su;
            ClAmsReadinessStateT suRstate;

            AMS_CHECK_SU ( su = (ClAmsSUT *) comp->config.parentSU.ptr );

            clAmsPeSUComputeReadinessState(su, &suRstate);
            
            CL_AMS_SET_R_STATE(su, suRstate);
            *compState = suRstate;
            break;
        }

        default:
        {
            AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG,CL_DEBUG_ERROR,
                ("Error: Component [%s] has invalid oper state [%s]. Reseting to disabled\n",
                 comp->config.entity.name.value,
                 CL_AMS_STRING_O_STATE(comp->status.operState)));

            CL_AMS_SET_O_STATE(comp, CL_AMS_OPER_STATE_DISABLED);
            *compState = CL_AMS_READINESS_STATE_OUTOFSERVICE;
        }
    }

    return CL_OK;
}

/*
 * clAmsPeCompUpdateReadinessState
 * -------------------------------
 * Update the readiness state of a component
 */

ClRcT
clAmsPeCompUpdateReadinessState(
        CL_IN       ClAmsCompT *comp)
{
    ClAmsReadinessStateT rstate;

    AMS_CHECK_COMP ( comp );

    AMS_CALL ( clAmsPeCompComputeReadinessState(comp, &rstate) );

    CL_AMS_SET_R_STATE(comp, rstate);

    return CL_OK;
}

static ClRcT clAmsPeProxiedCompTerminate(ClAmsCompT *comp, ClAmsCompT *proxy)
{
    ClAmsEntityRefT *csiRef = NULL;
    ClUint32T switchoverMode = CL_AMS_ENTITY_SWITCHOVER_IMMEDIATE;

    if(!proxy) return CL_OK;

    for(csiRef = clAmsEntityListGetFirst(&proxy->status.csiList);
        csiRef != NULL;
        csiRef = clAmsEntityListGetNext(&proxy->status.csiList, csiRef))
    {
        ClAmsCSIT *csi = (ClAmsCSIT*)csiRef->ptr;
        ClAmsEntityRefT *pgRef = NULL;
        for(pgRef = clAmsEntityListGetFirst(&csi->status.pgList);
            pgRef != NULL;
            pgRef = clAmsEntityListGetNext(&csi->status.pgList, pgRef))
        {
            ClAmsCSICompRefT *redundantRef = (ClAmsCSICompRefT*)pgRef;
            ClAmsCompT *redundant = (ClAmsCompT*)pgRef->ptr;
            if(redundant == proxy) continue;
            if(redundantRef->haState == CL_AMS_HA_STATE_ACTIVE)
            {
                /*
                 * No need to terminate proxied as there is an active
                 * proxy available.
                 */
                return CL_OK;
            }
        }
    }
    
    /*
     * No other active proxy available for the component. Terminate it
     */
    clAmsPeCompComputeSwitchoverMode(proxy, &switchoverMode);
                    
    AMS_ENTITY_LOG (comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
                    ("Removing Proxy [%s] for [%s] Component [%s]\n",
                     proxy->config.entity.name.value,
                     CL_AMS_STRING_COMP_PROPERTY(comp->config.property),
                     comp->config.entity.name.value));

    switch(comp->config.property)
    {
    case CL_AMS_COMP_PROPERTY_PROXIED_PREINSTANTIABLE:
        {
            if(! (switchoverMode & CL_AMS_ENTITY_SWITCHOVER_FAST))
            {
                AMS_CALL ( clAmsPeCompTerminate(comp) );
            }
            else
            {
                AMS_CALL ( clAmsPeCompTerminateCallback(comp, CL_OK) );
            }
        }
        break;

    case CL_AMS_COMP_PROPERTY_PROXIED_NON_PREINSTANTIABLE:
        {
            AMS_CALL (clAmsPeCompRemoveWork(comp, switchoverMode) );
        }
        break;
        
    default: break;
    }

    return CL_OK;
}

static ClRcT clAmsPeSUOperateProxied(ClAmsSUT *su, ClAmsCompT *proxy, ClAmsCSIT *proxyCSI,
                                     ClRcT (*fun)(ClAmsSUT*))
{
    ClAmsEntityRefT *eRef = NULL;
    /*
     * First update proxy references before the operation.
     */
    for(eRef = clAmsEntityListGetFirst(&su->config.compList);
        eRef != NULL;
        eRef = clAmsEntityListGetNext(&su->config.compList, eRef))
    {
        ClAmsCompT *comp = (ClAmsCompT*)eRef->ptr;
        if(!comp->status.proxyComp
           &&
           !memcmp(comp->config.proxyCSIType.value,
                   proxyCSI->config.type.value,
                   proxyCSI->config.type.length))
        {
            comp->status.proxyComp = (ClAmsEntityT*)proxy;
        }
    }
    
    if(fun)
        AMS_CALL((*fun)(su));
    return CL_OK;
}

/*
 * clAmsPeCompUpdateProxiedComponents
 * ----------------------------------
 * This fn is invoked when a proxyCSI undergoes a state change.
 */

static ClRcT
amsPeCompUpdateProxiedComponents(
                                 CL_IN ClAmsCompT *proxy,
                                 CL_IN ClAmsCompCSIRefT *proxyCSIRef,
                                 ClBoolT reassignCSI)
{

    ClCntNodeHandleT compNodeHandle, compNextNodeHandle;
    ClAmsCSIT *csi;
    ClAmsCompT *comp;
    ClRcT rc;

    AMS_CHECK_COMP ( proxy );
    AMS_CHECK_CSI ( csi = (ClAmsCSIT *) proxyCSIRef->entityRef.ptr );

    AMS_FUNC_ENTER ( ("Component [%s]\n", proxy->config.entity.name.value) );

    extern ClAmsT gAms;
    ClAmsEntityDbT *compDb = &gAms.db.entityDb[CL_AMS_ENTITY_TYPE_COMP];

    /*
     * Go through the db of components and for all components that have this
     * CSI set as a proxyCSI, see if they need to be started/stopped or assigned
     * work.
     */

    AMS_CALL ( clCntFirstNodeGet(
                                 compDb->db,
                                 &compNodeHandle) );

    while ( compNodeHandle )
    {
        ClAmsSUT *su;
        ClAmsNodeT *node;

        AMS_CALL ( clCntNodeUserDataGet(
                                        compDb->db,
                                        compNodeHandle,
                                        (ClCntDataHandleT *) &comp ) );
        
        AMS_CHECK_COMP ( comp );
        AMS_CHECK_SU ( su = (ClAmsSUT *) comp->config.parentSU.ptr );
        AMS_CHECK_NODE( node = (ClAmsNodeT*)su->config.parentNode.ptr );

        /*
         * Is this component proxied by this CSI?
         */

        if ( !memcmp(comp->config.proxyCSIType.value, 
                     csi->config.type.value, 
                     csi->config.type.length) )
        {
            /*
             * If this is an active assignment for the proxyCSI, then ask the proxy to start it
             * and/or assign work to it as per the component property.
             */
            
            if ( proxyCSIRef->haState == CL_AMS_HA_STATE_ACTIVE )
            {
                ClBoolT reassignProxy = reassignCSI;
                ClBoolT suUp, suDn, compUp, compNotUp, compNotDn;
                ClAmsPresenceStateT suState, compState;

                AMS_ENTITY_LOG (comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
                                ("Registering Proxy [%s] for [%s] Component [%s]\n",
                                 proxy->config.entity.name.value,
                                 CL_AMS_STRING_COMP_PROPERTY(comp->config.property),
                                 comp->config.entity.name.value));
                
                if(!reassignCSI && comp->status.proxyComp)
                {
                    if(comp->status.proxyComp == (ClAmsEntityT*)proxy)
                        goto nextComponent;
                    /*
                     * If the proxy has changed, send a reassign to this
                     * proxy. for the proxied
                     */
                    clLogNotice("AMS", "PROXY-UPDATE", "Reassigning proxy to [%s] from [%s]", 
                                proxy->config.entity.name.value, comp->status.proxyComp->name.value);
                    reassignProxy = CL_TRUE;
                }

                comp->status.proxyComp = (ClAmsEntityT *) proxy;

                /*
                 * We now have a proxied component with a new proxy. Ask the
                 * proxy to instantiate/terminate/assign CSIs to the proxied
                 * as some of these operations may be waiting for a proxy.
                 */

                /*
                 * Some shortcuts needed later
                 */

                suState   = su->status.presenceState;
                compState = comp->status.presenceState;

                compUp    = (compState == CL_AMS_PRESENCE_STATE_INSTANTIATED);

                compNotUp = (compState == CL_AMS_PRESENCE_STATE_UNINSTANTIATED) ||
                    (compState == CL_AMS_PRESENCE_STATE_INSTANTIATING)  ||
                    (compState == CL_AMS_PRESENCE_STATE_RESTARTING);

                compNotDn = (compState == CL_AMS_PRESENCE_STATE_INSTANTIATED)   ||
                    (compState == CL_AMS_PRESENCE_STATE_TERMINATING)    ||
                    (compState == CL_AMS_PRESENCE_STATE_RESTARTING);

                suUp      = (suState   == CL_AMS_PRESENCE_STATE_INSTANTIATING)  ||
                    (suState   == CL_AMS_PRESENCE_STATE_RESTARTING)     ||
                    (suState   == CL_AMS_PRESENCE_STATE_INSTANTIATED);

                suDn      = (suState   == CL_AMS_PRESENCE_STATE_UNINSTANTIATED) ||
                    (suState   == CL_AMS_PRESENCE_STATE_TERMINATING);

                /* 
                 * Proxied preinstantiable components are started only if the 
                 * SU they belong to is in some state of instantiation and the
                 * components have not already been started.
                 */

                if ( comp->config.property ==
                     CL_AMS_COMP_PROPERTY_PROXIED_PREINSTANTIABLE )
                {
                    if ( suUp && compNotUp )
                    {
                        AMS_CALL ( clAmsPeCompInstantiate(comp) );
                    }
                    
                    if ( suDn && compNotDn )
                    {
                        AMS_CALL ( clAmsPeCompTerminate(comp) );
                    }
                    
                    /*
                     * The proxied is modelled on a non-asp aware node. 
                     * But since the proxy is up, we fire the instantiate to the proxy.
                     */
                    if( !node->config.isASPAware && suDn && compNotUp)
                    {
                        AMS_CALL( clAmsPeSUOperateProxied(su, proxy, csi, clAmsPeSUInstantiate) );
                    }

                    if ( comp->status.readinessState != CL_AMS_READINESS_STATE_OUTOFSERVICE )
                    {
                        AMS_CALL ( clAmsPeCompAssignCSIAgain(comp) );
                    }
                    else if(suUp && compUp && reassignProxy)
                    {
                        AMS_CALL ( clAmsPeCompAssignCSIAgain(comp) );
                    }
                }

                /*
                 * Proxied nonpreinstantiable components are started only if the
                 * SU they belong has readiness state of inservice and is not
                 * an inservice spare.
                 */

                if ( comp->config.property ==
                     CL_AMS_COMP_PROPERTY_PROXIED_NON_PREINSTANTIABLE )
                {
                    if ( suUp && compNotUp ) 
                    {
                        if(comp->status.numActiveCSIs)
                        {
                            AMS_CALL ( clAmsPeCompInstantiate(comp) );
                        }
                        else
                        {
                            if(su->status.presenceState == CL_AMS_PRESENCE_STATE_INSTANTIATED)
                            {
                                AMS_CALL( clAmsPeSUOperateProxied(su, proxy, csi, clAmsPeSUEvaluateWork) );
                            }
                            else
                            {
                                AMS_CALL( clAmsPeSUOperateProxied(su, proxy, csi, clAmsPeSUInstantiate) );
                            }
                        }
                    }


                    /*
                     * Check for proxied on a non-asp aware node in which case,
                     * we would need to fire it to the proxy which is up.
                     */
                    if(!node->config.isASPAware && suDn && compNotUp)
                    {
                        AMS_CALL (clAmsPeSUOperateProxied(su, proxy, csi, clAmsPeSUInstantiate) );
                    }

                    if ( suDn && compNotDn )
                    {
                        AMS_CALL ( clAmsPeCompTerminate(comp) );
                    }

                    if(suUp && compUp && reassignProxy)
                    {
                        /*
                         * This is an instantiate request. to the new proxy.
                         */
                        CL_AMS_SET_P_STATE(comp, CL_AMS_PRESENCE_STATE_UNINSTANTIATED);
                        AMS_CALL( clAmsPeCompAssignCSIAgain(comp) );
                    }
                }
            }

            /*
             * If this proxyCSI was removed and thus has a state of NONE,
             * and the proxied component still has this component as proxy,
             * remove it. The proxied component now has no proxy and AMF has
             * no control over it. This transition happens, if a proxyCSI
             * is directly removed without going through a quiescing state.
             */

            if ( proxyCSIRef->haState == CL_AMS_HA_STATE_NONE )
            {
                if ( comp->status.proxyComp == (ClAmsEntityT *) proxy )
                {
                    AMS_CALL ( clAmsPeProxiedCompTerminate(comp, proxy) );
                }
            }

        }

        nextComponent:
        rc = clCntNextNodeGet(
                              compDb->db,
                              compNodeHandle,
                              &compNextNodeHandle);

        if ( CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST ) break;
        if ( rc != CL_OK )
            return rc;

        compNodeHandle = compNextNodeHandle;
    }

    return CL_OK; 
}

ClRcT clAmsPeCompUpdateProxiedComponents(
                                         CL_IN ClAmsCompT *proxy,
                                         CL_IN ClAmsCompCSIRefT *proxyCSIRef)
{
    return amsPeCompUpdateProxiedComponents(proxy, proxyCSIRef, CL_FALSE);
}

/*
 * clAmsPeCompIsProxyReady
 * -----------------------
 * Returns CL_TRUE if a proxy is ready and has active assignment for the
 * proxy CSI, else returns CL_FALSE.
 */

ClRcT
clAmsPeCompIsProxyReady(
        CL_IN   ClAmsCompT *comp)
{
    ClAmsCompT *proxy = (ClAmsCompT *) comp->status.proxyComp;

    AMS_CHECK_COMP ( comp );

    AMS_FUNC_ENTER ( ("Component [%s]\n",comp->config.entity.name.value) );
    
    if ( proxy && (proxy->status.readinessState == CL_AMS_READINESS_STATE_INSERVICE) )
    {
        ClAmsEntityRefT *entityRef;

        for ( entityRef = clAmsEntityListGetFirst(&proxy->status.csiList);
              entityRef != (ClAmsEntityRefT *) NULL;
              entityRef = clAmsEntityListGetNext(&proxy->status.csiList, entityRef) )
        {
            ClAmsCompCSIRefT *csiRef = (ClAmsCompCSIRefT *) entityRef;
            ClAmsCSIT *csi = (ClAmsCSIT *) entityRef->ptr;

            AMS_CHECK_CSI ( csi );

            if ( !memcmp(comp->config.proxyCSIType.value, 
                         csi->config.type.value, 
                         csi->config.type.length) )
            {
                if ( csi->config.isProxyCSI && 
                     (csiRef->haState == CL_AMS_HA_STATE_ACTIVE) )
                {
                    return CL_TRUE;
                }
            }
        }
    }

    return CL_FALSE;
}

/*
 * clAmsPeCompReset
 * ----------------
 * Reset the status of a component 
 */

ClRcT
clAmsPeCompReset(
        CL_IN ClAmsCompT *comp)
{
    AMS_CHECK_COMP ( comp );

    AMS_FUNC_ENTER ( ("Component [%s]\n",comp->config.entity.name.value) );

    clAmsEntityReset((ClAmsEntityT *) comp);

    return CL_OK;
}

/*
 * Future: All functions below are future work items
 */

ClRcT
clAmsPeCompAmStartTimeout(
        CL_IN ClAmsEntityTimerT  *timer) 
{
    ClAmsCompT *comp;

    AMS_CHECKPTR ( !timer );

    comp = (ClAmsCompT *) timer->entity;

    AMS_FUNC_ENTER ( ("Component [%s]\n", comp->config.entity.name.value) );

    return CL_AMS_RC(CL_ERR_NOT_IMPLEMENTED);
}

ClRcT clAmsPeCompAmStopTimeout(
        CL_IN ClAmsEntityTimerT  *timer) 
{
    ClAmsCompT *comp;

    AMS_CHECKPTR ( !timer );

    comp = (ClAmsCompT *) timer->entity;

    AMS_FUNC_ENTER ( ("Component [%s]\n", comp->config.entity.name.value) );

    return CL_AMS_RC(CL_ERR_NOT_IMPLEMENTED);
}

ClRcT
clAmsPeCompAmStartCallback(
        CL_IN       ClAmsEntityT        *entity,
        CL_IN       ClInvocationT       invocation,
        CL_IN       ClRcT               error)
{
    return CL_AMS_RC(CL_ERR_NOT_IMPLEMENTED);
}

ClRcT
clAmsPeCompAmStopCallback(
        CL_IN       ClAmsEntityT        *entity,
        CL_IN       ClInvocationT       invocation,
        CL_IN       ClRcT               error)
{
    return CL_AMS_RC(CL_ERR_NOT_IMPLEMENTED);
}

/******************************************************************************
 * CSI Functions
 *****************************************************************************/

/*-----------------------------------------------------------------------------
 * Utility Functions
 *---------------------------------------------------------------------------*/
 
/*
 * clAmsPeCSITransitionHAState
 * ---------------------------
 * This fn updates the states of a CSI, Component, SI, SU and SG when a HA
 * transition takes place for a CSI.
 */

static ClRcT
_clAmsPeCSITransitionHAState(
        CL_IN   ClAmsCSIT *csi,
        CL_IN   ClAmsCompT *comp,
        CL_IN   ClAmsHAStateT oldState,
        CL_IN   ClAmsHAStateT newState,
        CL_IN   ClUint32T switchoverMode)
{
    ClAmsSUT *su;
    ClAmsSIT *si;
    ClAmsSGT *sg;
    ClAmsSUSIRefT *siRef;
    ClAmsSISURefT *suRef;
    ClAmsCSICompRefT *compRef;
    ClAmsCompCSIRefT *csiRef;
    ClRcT rc = CL_OK;
    ClRcT invalid = CL_FALSE;
    ClBoolT suWasAssigned, suIsAssigned;

    AMS_CHECK_COMP ( comp );
    AMS_CHECK_CSI  ( csi );
    AMS_CHECK_SI   ( si = (ClAmsSIT *) csi->config.parentSI.ptr );
    AMS_CHECK_SU   ( su = (ClAmsSUT *) comp->config.parentSU.ptr );
    AMS_CHECK_SG   ( sg = (ClAmsSGT *) su->config.parentSG.ptr );

    AMS_FUNC_ENTER ( ("CSI [%s]\n", csi->config.entity.name.value) );

    if ( oldState == newState )
    {
        return CL_OK;
    }

    suWasAssigned = ( su->status.numActiveSIs + su->status.numStandbySIs ) ?
                            CL_TRUE : CL_FALSE;

    if ( oldState == CL_AMS_HA_STATE_NONE )
    {
        AMS_CALL (clAmsCompAddCSIRefToCSIList(
                        &comp->status.csiList,
                        csi,
                        CL_AMS_HA_STATE_NONE,
                        CL_AMS_CSI_NEW_ASSIGN));

        AMS_CALL (clAmsCSIAddCompRefToPGList(
                        &csi->status.pgList,
                        comp,
                        CL_AMS_HA_STATE_NONE));
    }

    AMS_CALL ( clAmsEntityListFindEntityRef2(
                    &comp->status.csiList, 
                    &csi->config.entity,
                    0, 
                    (ClAmsEntityRefT **)&csiRef) );

    AMS_CALL ( clAmsEntityListFindEntityRef2(
                    &csi->status.pgList,
                    &comp->config.entity,
                    0,
                    (ClAmsEntityRefT **)&compRef) );

    if ( (rc = clAmsEntityListFindEntityRef2(
                    &su->status.siList,
                    &si->config.entity,
                    0,
                    (ClAmsEntityRefT **)&siRef) ) != CL_OK)
    {
        clLogError("CSI", "CHG", "EntityRef find for si [%s] failed with [%#x] "
                   "at Line [%d], Function [%s]",
                   si->config.entity.name.value, rc, __LINE__, __FUNCTION__);
        if(oldState == CL_AMS_HA_STATE_NONE)
        {
            clAmsCompDeleteCSIRefFromCSIList(&comp->status.csiList, csi);
            clAmsCSIDeleteCompRefFromPGList(&csi->status.pgList, comp);
        }
        return rc;
    }

    if( (rc = clAmsEntityListFindEntityRef2(
                    &si->status.suList,
                    &su->config.entity,
                    0,
                    (ClAmsEntityRefT **)&suRef) ) != CL_OK)
    {
        clLogError("CSI", "CHG", "EntityRef find for su [%s] failed with [%#x] "
                   "at Line [%d], Function [%s]",
                   su->config.entity.name.value, rc, __LINE__, __FUNCTION__);
        if(oldState == CL_AMS_HA_STATE_NONE)
        {
            clAmsCompDeleteCSIRefFromCSIList(&comp->status.csiList, csi);
            clAmsCSIDeleteCompRefFromPGList(&csi->status.pgList, comp);
        }
        return rc;
    }

    /*
     * The switch below implements the state machine for the HA states associated 
     * to a component on behalf of a CSI.
     *
     * Note, Quiescing states are considered as active, hence active count is not
     * reduced when transition takes place from active to quiescing, instead both
     * are reduced when transition takes place to quiesced.
     */

    switch ( oldState )
    {
        /*
         * None -> { Active, Standby }
         */

        case CL_AMS_HA_STATE_NONE:
        {
            switch ( newState )
            {
                case CL_AMS_HA_STATE_ACTIVE:
                {
                    comp->status.numActiveCSIs++;
                    siRef->numActiveCSIs++;

                    if ( siRef->numActiveCSIs == 1 )
                    {
                        su->status.numActiveSIs++;

                        if ( su->status.numActiveSIs == 1 )
                        {
                            sg->status.numCurrActiveSUs ++;
                        }

                        si->status.numActiveAssignments++;
                    }

                    if ( siRef->numActiveCSIs == si->config.numCSIs )
                    {
                    }

                    break;
                }

                case CL_AMS_HA_STATE_STANDBY:
                {
                    comp->status.numStandbyCSIs++;
                    siRef->numStandbyCSIs++;

                    if ( siRef->numStandbyCSIs == 1 )
                    {
                        su->status.numStandbySIs++;

                        if ( su->status.numStandbySIs == 1 )
                        {
                            sg->status.numCurrStandbySUs ++;
                        }

                        si->status.numStandbyAssignments++;
                    }

                    if ( siRef->numStandbyCSIs == si->config.numCSIs )
                    {
                    }

                    break;
                }

                default:
                {
                    invalid = CL_TRUE;
                }
            }

            break;
        }

        /*
         * Active -> { Quiescing, Quiesced, None }
         */

        case CL_AMS_HA_STATE_ACTIVE:
        {
            switch ( newState )
            {
                case CL_AMS_HA_STATE_QUIESCING:
                {
                    comp->status.numQuiescingCSIs++;
                    siRef->numQuiescingCSIs++;

                    break;
                }

                case CL_AMS_HA_STATE_QUIESCED:
                {
                    comp->status.numQuiescedCSIs++;
                    siRef->numQuiescedCSIs++;

                    comp->status.numActiveCSIs--;
                    siRef->numActiveCSIs--;

                    if ( siRef->numActiveCSIs == 0 )
                    {
                        su->status.numActiveSIs--;
                        su->status.numQuiescedSIs++;

                        if ( su->status.numActiveSIs == 0 )
                        {
                            sg->status.numCurrActiveSUs--;
                        }
                        si->status.numActiveAssignments--;
                    }

                    if ( siRef->numActiveCSIs == (si->config.numCSIs - 1) )
                    {
                    }

                    break;
                }

                case CL_AMS_HA_STATE_STANDBY:
                {
                    --comp->status.numActiveCSIs;
                    --siRef->numActiveCSIs;
                    if(siRef->numActiveCSIs == 0)
                    {
                        --su->status.numActiveSIs;
                        if(su->status.numActiveSIs == 0)
                        {
                            --sg->status.numCurrActiveSUs;
                        }
                        --si->status.numActiveAssignments;
                    }
                    ++comp->status.numStandbyCSIs;
                    ++siRef->numStandbyCSIs;
                    if(siRef->numStandbyCSIs == 1)
                    {
                        ++su->status.numStandbySIs;
                        if(su->status.numStandbySIs == 1)
                        {
                            ++sg->status.numCurrStandbySUs;
                        }
                        ++si->status.numStandbyAssignments;
                    }
                    break;
                }

                case CL_AMS_HA_STATE_NONE:
                {
                    comp->status.numActiveCSIs--;
                    siRef->numActiveCSIs--;

                    if ( siRef->numActiveCSIs == 0 )
                    {
                        su->status.numActiveSIs--;

                        if ( su->status.numActiveSIs == 0 )
                        {
                            sg->status.numCurrActiveSUs--;
                        }
                        si->status.numActiveAssignments--;
                    }

                    if ( siRef->numActiveCSIs == (si->config.numCSIs - 1) )
                    {
                    }

                    break;
                }

                default:
                {
                    invalid = CL_TRUE;
                }
            }

            break;
        }

        /*
         * Standby -> { Active, None }
         */

        case CL_AMS_HA_STATE_STANDBY:
        {
            switch ( newState )
            {
                case CL_AMS_HA_STATE_ACTIVE:
                {
                    comp->status.numStandbyCSIs--;
                    siRef->numStandbyCSIs--;

                    if ( siRef->numStandbyCSIs == 0 )
                    {
                        su->status.numStandbySIs--;

                        if ( su->status.numStandbySIs == 0 )
                        {
                            sg->status.numCurrStandbySUs--;
                        }
                        si->status.numStandbyAssignments--;
                    }

                    if ( siRef->numStandbyCSIs == (si->config.numCSIs - 1) )
                    {
                        //si->status.numStandbyAssignments--;
                    }

                    comp->status.numActiveCSIs++;
                    siRef->numActiveCSIs++;

                    if ( siRef->numActiveCSIs == 1 )
                    {
                        su->status.numActiveSIs++;

                        if ( su->status.numActiveSIs == 1 )
                        {
                            sg->status.numCurrActiveSUs++;
                        }
                        si->status.numActiveAssignments++;
                    }
                        
                    if ( siRef->numActiveCSIs == si->config.numCSIs )
                    {
                    }

                    break;
                }

                case CL_AMS_HA_STATE_NONE:
                {
                    comp->status.numStandbyCSIs--;
                    siRef->numStandbyCSIs--;

                    if ( siRef->numStandbyCSIs == 0 )
                    {
                        su->status.numStandbySIs--;

                        if ( su->status.numStandbySIs == 0 )
                        {
                            sg->status.numCurrStandbySUs--;
                        }
                        si->status.numStandbyAssignments--;
                    }

                    if ( siRef->numStandbyCSIs == (si->config.numCSIs - 1) )
                    {
                        //si->status.numStandbyAssignments--;
                    }

                    break;
                }

                default:
                {
                    invalid = CL_TRUE;
                }
            }

            break;
        }

        /*
         * Quiescing -> { Quiesced, Active, None }
         */

        case CL_AMS_HA_STATE_QUIESCING:
        {
            switch ( newState )
            {
                case CL_AMS_HA_STATE_QUIESCED:
                {
                    comp->status.numQuiescedCSIs++;
                    siRef->numQuiescedCSIs++;

                    comp->status.numQuiescingCSIs--;
                    siRef->numQuiescingCSIs--;

                    comp->status.numActiveCSIs--;
                    siRef->numActiveCSIs--;

                    if ( siRef->numActiveCSIs == 0 )
                    {
                        su->status.numActiveSIs--;
                        su->status.numQuiescedSIs++;

                        if ( su->status.numActiveSIs == 0 )
                        {
                            sg->status.numCurrActiveSUs--;
                        }
                        si->status.numActiveAssignments--;
                    }

                    if ( siRef->numActiveCSIs == (si->config.numCSIs - 1) )
                    {
                        //si->status.numActiveAssignments--;
                    }

                    break;
                }

                case CL_AMS_HA_STATE_ACTIVE:
                {
                    comp->status.numQuiescingCSIs--;
                    siRef->numQuiescingCSIs--;

                    break;
                }

                case CL_AMS_HA_STATE_NONE:
                {
                    comp->status.numQuiescingCSIs--;
                    siRef->numQuiescingCSIs--;

                    comp->status.numActiveCSIs--;
                    siRef->numActiveCSIs--;

                    if ( siRef->numActiveCSIs == 0 )
                    {
                        su->status.numActiveSIs--;

                        if ( su->status.numActiveSIs == 0 )
                        {
                            sg->status.numCurrActiveSUs--;
                        }
                        si->status.numActiveAssignments--;
                    }

                    if ( siRef->numActiveCSIs == (si->config.numCSIs - 1) )
                    {
                        //si->status.numActiveAssignments--;
                    }

                    break;
                }

                default:
                {
                    invalid = CL_TRUE;
                }
            }

            break;
        }

        /*
         * Quiesced -> { Active, Standby, None }
         */

        case CL_AMS_HA_STATE_QUIESCED:
        {
            switch ( newState )
            {
                case CL_AMS_HA_STATE_ACTIVE:
                {
                    comp->status.numQuiescedCSIs--;
                    siRef->numQuiescedCSIs--;

                    comp->status.numActiveCSIs++;
                    siRef->numActiveCSIs++;

                    if ( siRef->numActiveCSIs == 1 )
                    {
                        su->status.numActiveSIs++;
                        su->status.numQuiescedSIs--;

                        if ( su->status.numActiveSIs == 1 )
                        {
                            sg->status.numCurrActiveSUs ++;
                        }
                        si->status.numActiveAssignments++;
                    }

                    if ( siRef->numActiveCSIs == si->config.numCSIs )
                    {
                    }

                    break; 
                }

                case CL_AMS_HA_STATE_STANDBY:
                {
                    comp->status.numQuiescedCSIs--;
                    siRef->numQuiescedCSIs--;

                    comp->status.numStandbyCSIs++;
                    siRef->numStandbyCSIs++;

                    if ( siRef->numStandbyCSIs == 1 )
                    {
                        su->status.numStandbySIs++;
                        su->status.numQuiescedSIs--;

                        if ( su->status.numStandbySIs == 1 )
                        {
                            sg->status.numCurrStandbySUs ++;
                        }
                        si->status.numStandbyAssignments++;
                    }

                    if ( siRef->numStandbyCSIs == si->config.numCSIs )
                    {
                    }

                    break;
                }

                case CL_AMS_HA_STATE_NONE:
                {
                    comp->status.numQuiescedCSIs--;
                    siRef->numQuiescedCSIs--;

                    if ( siRef->numQuiescedCSIs == 0 )
                    {
                        su->status.numQuiescedSIs--;
                    }

                    break;
                }

                default:
                {
                    invalid = CL_TRUE;
                }
            }

            break;
        }

        default:
        {
            invalid = CL_TRUE;
        }
    }

    if ( invalid )
    {
        AMS_ENTITY_LOG(comp, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_ERROR,
            ("Error: HA State assigned to Component [%s] for CSI [%s] cannot transition from [%s] to [%s].\n",
             comp->config.entity.name.value,
             csi->config.entity.name.value,
             CL_AMS_STRING_H_STATE(oldState),
             CL_AMS_STRING_H_STATE(newState)));

        return CL_OK;
    }

    /*
     * Update the HA state of this CSI assignment. If the new state is none
     * or unassigned, remove the comp and csi references.
     */

    CL_AMS_SET_H_STATE(comp, csiRef, newState, switchoverMode);
    CL_AMS_SET_H_STATE(csi, compRef, newState, switchoverMode);

    if ( newState == CL_AMS_HA_STATE_NONE )
    {
        // Don't delete the CSIref here, we remove it after receiving confirmation
        // from component.

        //AMS_CALL (clAmsCompDeleteCSIRefFromCSIList(&comp->status.csiList, csi));
        //AMS_CALL (clAmsCSIDeleteCompRefFromPGList(&csi->status.pgList, comp));
    }

    /*
     * Update the HA state of this SI assignment. If the new state is none
     * or unassigned, remove the su and si references.
     */

    if ( siRef->numActiveCSIs )
    {
        if ( siRef->numActiveCSIs == siRef->numQuiescingCSIs )
        {
            CL_AMS_SET_H_STATE(su, siRef, CL_AMS_HA_STATE_QUIESCING, switchoverMode);
            CL_AMS_SET_H_STATE(si, suRef, CL_AMS_HA_STATE_QUIESCING, switchoverMode);
        }
        else
        {
            CL_AMS_SET_H_STATE(su, siRef, CL_AMS_HA_STATE_ACTIVE, switchoverMode);
            CL_AMS_SET_H_STATE(si, suRef, CL_AMS_HA_STATE_ACTIVE, switchoverMode);
        }
    }
    else if ( siRef->numQuiescingCSIs )
    {
        CL_AMS_SET_H_STATE(su, siRef, CL_AMS_HA_STATE_QUIESCING, switchoverMode);
        CL_AMS_SET_H_STATE(si, suRef, CL_AMS_HA_STATE_QUIESCING, switchoverMode);
    }
    else if ( siRef->numStandbyCSIs )
    {
        CL_AMS_SET_H_STATE(su, siRef, CL_AMS_HA_STATE_STANDBY, switchoverMode);
        CL_AMS_SET_H_STATE(si, suRef, CL_AMS_HA_STATE_STANDBY, switchoverMode);
    }
    else if ( siRef->numQuiescedCSIs )
    {
        CL_AMS_SET_H_STATE(su, siRef, CL_AMS_HA_STATE_QUIESCED, switchoverMode);
        CL_AMS_SET_H_STATE(si, suRef, CL_AMS_HA_STATE_QUIESCED, switchoverMode);
    }
    else
    {
        CL_AMS_SET_H_STATE(su, siRef, CL_AMS_HA_STATE_NONE, switchoverMode);
        CL_AMS_SET_H_STATE(si, suRef, CL_AMS_HA_STATE_NONE, switchoverMode);

        // Don't delete the SIref here, we remove it after receiving confirmation
        // from SU.

        //AMS_CALL ( clAmsSUDeleteSIRefFromSIList(&su->status.siList, si) );
        //AMS_CALL ( clAmsSIDeleteSURefFromSUList(&si->status.suList, su) );
    }


    /*
     * Update the operational/assignment state of the SI
     */

    if ( siRef->numActiveCSIs == 0 )
    {
        if ( si->status.numActiveAssignments == 0 )
        {
            CL_AMS_SET_O_STATE(si, CL_AMS_OPER_STATE_DISABLED);
        }
    }

    if ( siRef->numActiveCSIs == si->config.numCSIs )
    {
        if ( si->status.numActiveAssignments == 1 )
        {
            // Do this when we have an ack from the SU, otherwise
            // dependents will prematurely assume the SI is active
            // CL_AMS_SET_O_STATE(si, CL_AMS_OPER_STATE_ENABLED);
        }
    }

    /*
     * Update SUs assignment status based on change
     */

    suIsAssigned = ( su->status.numActiveSIs + su->status.numStandbySIs ) ?
                            CL_TRUE : CL_FALSE;

    if ( !suWasAssigned && suIsAssigned )
    {
        AMS_CALL ( clAmsPeSUMarkAssigned(su) );
    }

    if ( suWasAssigned && !suIsAssigned )
    {
        AMS_CALL ( clAmsPeSUMarkUnassigned(su) );
    }

#if 0
    printf("SI [%s]\n", si->config.entity.name.value);
    printf("\tactiveAssignments  [%d]\n", si->status.numActiveAssignments);
    printf("\tstandbyAssignments [%d]\n", si->status.numStandbyAssignments);
    printf("\tOper State         [%s]\n", CL_AMS_STRING_O_STATE(si->status.operState));
    printf("\tnumActiveCSIs      [%d]\n", siRef->numActiveCSIs);
    printf("\tnumStandbyCSIs     [%d]\n", siRef->numStandbyCSIs);
    printf("\tnumQuiescingCSIs   [%d]\n", siRef->numQuiescingCSIs);
    printf("\tnumQuiescedCSIs    [%d]\n", siRef->numQuiescedCSIs);
#endif

    return CL_OK;
}

ClRcT
clAmsPeCSITransitionHAState(
        CL_IN   ClAmsCSIT *csi,
        CL_IN   ClAmsCompT *comp,
        CL_IN   ClAmsHAStateT oldState,
        CL_IN   ClAmsHAStateT newState)
{
    return _clAmsPeCSITransitionHAState(csi, comp, oldState, newState, 
                                        CL_AMS_ENTITY_SWITCHOVER_IMMEDIATE);
}

ClRcT
clAmsPeCSITransitionHAStateExtended(
        CL_IN   ClAmsCSIT *csi,
        CL_IN   ClAmsCompT *comp,
        CL_IN   ClAmsHAStateT oldState,
        CL_IN   ClAmsHAStateT newState,
        CL_IN   ClUint32T switchoverMode)
{
    return _clAmsPeCSITransitionHAState(csi, comp, oldState, newState,
                                        switchoverMode);
}

/*
 * clAmsPeCSIComputeAdminState
 * ---------------------------
 * The computed admin state for a CSI is the admin state of its SI.
 */

ClRcT
clAmsPeCSIComputeAdminState(
        CL_IN ClAmsCSIT *csi,
        CL_OUT ClAmsAdminStateT *adminState)
{
    ClAmsSIT *si;

    AMS_CHECK_SI ( si = (ClAmsSIT *) csi->config.parentSI.ptr );

    AMS_CALL ( clAmsPeSIComputeAdminState(si, adminState) );

    return CL_OK;
}

static ClRcT 
clAmsPeSISwapNWay(ClAmsSIT *si, ClAmsSGT *sg)
{
    /*
     * Even though the above implementation for MPlusN/2N should work for N-way also.
     */
    return CL_AMS_RC(CL_ERR_NOT_IMPLEMENTED);
}

ClRcT
clAmsPeSISwap(ClAmsSIT *si)
{
    ClRcT rc = CL_OK;
    ClAmsSGT *sg = NULL;

    AMS_CHECK_SG( sg = (ClAmsSGT*)si->config.parentSG.ptr);

    if(sg->config.autoAdjust)
    {
        clLogWarning("SI", "SWAP", "Cannot swap SIs for SG [%s], "
                     "which has auto adjust option enabled.",
                     sg->config.entity.name.value);
        return CL_AMS_RC(CL_ERR_BAD_OPERATION);
    }

    if(si->config.adminState != CL_AMS_ADMIN_STATE_UNLOCKED)
    {
        AMS_LOG(CL_DEBUG_ERROR, ("SI [%.*s] admin state is not unlocked\n",
                                 si->config.entity.name.length-1,
                                 si->config.entity.name.value));
        return CL_AMS_RC(CL_ERR_BAD_OPERATION);
    }

    if(!si->config.numStandbyAssignments 
       || 
       !si->status.numStandbyAssignments)
    {
        AMS_LOG(CL_DEBUG_ERROR, ("SI [%.*s] doesnt have standby assignments\n",
                                 si->config.entity.name.length-1,
                                 si->config.entity.name.value));
        return CL_AMS_RC(CL_ERR_BAD_OPERATION);
    }

    if(clAmsInvocationsPendingForSG(sg))
    {
        clLogInfo("SI", "SWAP", "SG [%s] containing SI [%s] has pending invocations. Deferring swap",
                  sg->config.entity.name.value, si->config.entity.name.value);
        return CL_AMS_RC(CL_ERR_TRY_AGAIN);
    }

    switch(sg->config.redundancyModel)
    {

    case CL_AMS_SG_REDUNDANCY_MODEL_TWO_N:
    case CL_AMS_SG_REDUNDANCY_MODEL_M_PLUS_N:
        rc = clAmsPeSISwapMPlusN(si, sg);
        break;

    case CL_AMS_SG_REDUNDANCY_MODEL_N_WAY:
        rc = clAmsPeSISwapNWay(si, sg);
        break;

    case CL_AMS_SG_REDUNDANCY_MODEL_CUSTOM:
        break;

    default:
    case CL_AMS_SG_REDUNDANCY_MODEL_NO_REDUNDANCY:
    case CL_AMS_SG_REDUNDANCY_MODEL_N_WAY_ACTIVE:
        AMS_LOG(CL_DEBUG_ERROR, ("SG [%.*s] redundancy model doesnt support SWAP\n",
                                 sg->config.entity.name.length-1, sg->config.entity.name.value));
        return CL_AMS_RC(CL_ERR_BAD_OPERATION);
    }

    return rc;
}

/******************************************************************************
 * Generic Entity Functions
 *****************************************************************************/

ClRcT
clAmsPeEntityGetAdminState(
        CL_IN       ClAmsEntityT        *entity,
        CL_OUT      ClAmsAdminStateT    *adminState)
{
    AMS_CHECKPTR ( !entity || !adminState );
    
    switch ( entity->type )
    {
        case CL_AMS_ENTITY_TYPE_CLUSTER:
        {
            return CL_AMS_RC(CL_AMS_ERR_INVALID_ENTITY);
        }

        case CL_AMS_ENTITY_TYPE_APP:
        {
            return CL_AMS_RC(CL_AMS_ERR_INVALID_ENTITY);
        }

        case CL_AMS_ENTITY_TYPE_SG:
        {
            ClAmsSGT *sg = (ClAmsSGT *) entity;

            AMS_CHECK_SG ( sg );

            *adminState = sg->config.adminState;

            return CL_OK;
        }

        case CL_AMS_ENTITY_TYPE_NODE:
        {
            ClAmsNodeT *node = (ClAmsNodeT *) entity;

            AMS_CHECK_NODE ( node );

            *adminState = node->config.adminState;

            return CL_OK;
        }

        case CL_AMS_ENTITY_TYPE_SU:
        {
            ClAmsSUT *su = (ClAmsSUT *) entity;

            AMS_CHECK_SU ( su );

            *adminState = su->config.adminState;

            return CL_OK;
        }

        case CL_AMS_ENTITY_TYPE_SI:
        {
            ClAmsSIT *si = (ClAmsSIT *) entity;

            AMS_CHECK_SI ( si );

            *adminState = si->config.adminState;

            return CL_OK;
        }

        default:
        {
            return CL_AMS_RC(CL_AMS_ERR_INVALID_ENTITY);
        }
    }
}

/*-----------------------------------------------------------------------------
 * Administration API functions included in Management API
 *---------------------------------------------------------------------------*/
 
/*
 * clAmsPeEntityUnlock
 * -------------------
 * This event informs the AMS PE that the entity is now administratively
 * unlocked and can be used.
 */

ClRcT
clAmsPeEntityUnlock(
        CL_IN       ClAmsEntityT        *entity)
{
    ClRcT rc = CL_OK;
    ClAmsAdminStateT adminState;

    AMS_OP_INCR(&gAms.ops);

    AMS_CHECKPTR ( !entity );

    AMS_FUNC_ENTER ( ("Entity [%s]\n",entity->name.value) );

    AMS_CALL ( clAmsEntityValidate(
                    (ClAmsEntityT *) entity,
                    CL_AMS_ENTITY_VALIDATE_CONFIG) );

    if ( (rc = clAmsPeEntityGetAdminState(entity, &adminState)) == CL_OK )
    {
        if ( adminState == CL_AMS_ADMIN_STATE_UNLOCKED )
        {
            rc = CL_AMS_RC(CL_ERR_NO_OP);
        }
        else if ( adminState == CL_AMS_ADMIN_STATE_LOCKED_I )
        {
            rc = CL_AMS_RC(CL_ERR_BAD_OPERATION);
        }
        else
        {
            switch ( entity->type )
            {
                case CL_AMS_ENTITY_TYPE_CLUSTER:
                {
                    rc = CL_AMS_RC(CL_AMS_ERR_INVALID_ENTITY);

                    break;
                }

                case CL_AMS_ENTITY_TYPE_APP:
                {
                    rc = CL_AMS_RC(CL_AMS_ERR_INVALID_ENTITY);

                    break;
                }

                case CL_AMS_ENTITY_TYPE_SG:
                {
                    rc = clAmsPeSGUnlock((ClAmsSGT *) entity);

                    break;
                }

                case CL_AMS_ENTITY_TYPE_NODE:
                {
                    rc = clAmsPeNodeUnlock((ClAmsNodeT *) entity);

                    break;
                }

                case CL_AMS_ENTITY_TYPE_SU:
                {
                    rc = clAmsPeSUUnlock((ClAmsSUT *) entity);

                    break;
                }

                case CL_AMS_ENTITY_TYPE_SI:
                {
                    rc = clAmsPeSIUnlock((ClAmsSIT *) entity);

                    break;
                }

                default:
                {
                    rc = CL_AMS_RC(CL_AMS_ERR_INVALID_ENTITY);
                }
            }
        }
    }

    if ( rc == CL_OK )
    {
        AMS_ENTITY_LOG (entity, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("Admin Operation [Unlock] on [%s] in Admin State [%s] returned Okay\n",
             entity->name.value,
             CL_AMS_STRING_A_STATE(adminState)));
    }
    else if ( CL_GET_ERROR_CODE(rc) == CL_AMS_ERR_INVALID_ENTITY )
    {
        AMS_ENTITY_LOG (entity, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("Admin Operation [Unlock] on [%s] is invalid. Continuing..\n",
             entity->name.value));
    }
    else if ( CL_GET_ERROR_CODE(rc) == CL_ERR_NO_OP )
    {
        AMS_ENTITY_LOG (entity, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("Admin Operation [Unlock] on [%s] in Admin State [%s] is a NoOp. Continuing..\n",
             entity->name.value,
             CL_AMS_STRING_A_STATE(adminState)));
    }
    else if ( CL_GET_ERROR_CODE(rc) == CL_ERR_BAD_OPERATION )
    {
        AMS_ENTITY_LOG (entity, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("Admin Operation [Unlock] on [%s] in Admin State [%s] is not possible. Continuing..\n",
             entity->name.value,
             CL_AMS_STRING_A_STATE(adminState)));
    }
    else
    {
        AMS_ENTITY_LOG (entity, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("Admin Operation [Unlock] on [%s] in Admin State [%s] returned Error [0x%x]\n",
             entity->name.value,
             CL_AMS_STRING_A_STATE(adminState),
             rc));
    }

    return rc;
}

/*
 * clAmsPeEntityLockInstantiate
 * ----------------------------
 * This event informs the AMS PE that the entity is now administratively
 * locked and cannot be instantiated. If the entity is in use, it is
 * gracefully removed.
 */

ClRcT
clAmsPeEntityLockInstantiate(
        CL_IN       ClAmsEntityT        *entity)
{
    ClRcT rc = CL_OK;
    ClAmsAdminStateT adminState;

    AMS_OP_INCR(&gAms.ops);

    AMS_CHECKPTR ( !entity );

    AMS_FUNC_ENTER ( ("Entity [%s]\n",entity->name.value) );

    if ( (rc = clAmsPeEntityGetAdminState(entity, &adminState)) == CL_OK )
    {
        if ( adminState == CL_AMS_ADMIN_STATE_LOCKED_I )
        {
            rc = CL_AMS_RC(CL_ERR_NO_OP);
        }
        else if ( (adminState == CL_AMS_ADMIN_STATE_UNLOCKED) ||
                  (adminState == CL_AMS_ADMIN_STATE_SHUTTINGDOWN) )
        {
            rc = CL_AMS_RC(CL_ERR_BAD_OPERATION);
        }
        else
        {
            switch ( entity->type )
            {
                case CL_AMS_ENTITY_TYPE_CLUSTER:
                {
                    rc = CL_AMS_RC(CL_AMS_ERR_INVALID_ENTITY);

                    break;
                }

                case CL_AMS_ENTITY_TYPE_APP:
                {
                    rc = CL_AMS_RC(CL_AMS_ERR_INVALID_ENTITY);

                    break;
                }

                case CL_AMS_ENTITY_TYPE_SG:
                {
                    rc = clAmsPeSGLockInstantiation((ClAmsSGT *) entity);

                    break;
                }

                case CL_AMS_ENTITY_TYPE_NODE:
                {
                    rc = clAmsPeNodeLockInstantiation((ClAmsNodeT *) entity);

                    break;
                }

                case CL_AMS_ENTITY_TYPE_SU:
                {
                    rc = clAmsPeSULockInstantiation((ClAmsSUT *) entity);

                    break;
                }

                default:
                {
                    rc = CL_AMS_RC(CL_AMS_ERR_INVALID_ENTITY);
                }
            }
        }
    }

    if ( rc == CL_OK )
    {
        AMS_ENTITY_LOG (entity, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("Admin Operation [Lock Instantiation] on [%s] in Admin State [%s] returned Okay\n",
             entity->name.value,
             CL_AMS_STRING_A_STATE(adminState)));
    }
    else if ( CL_GET_ERROR_CODE(rc) == CL_AMS_ERR_INVALID_ENTITY )
    {
        AMS_ENTITY_LOG (entity, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("Admin Operation [Lock Instantiation] on [%s] is invalid. Continuing..\n",
             entity->name.value));
    }
    else if ( CL_GET_ERROR_CODE(rc) == CL_ERR_NO_OP )
    {
        AMS_ENTITY_LOG (entity, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("Admin Operation [Lock Instantiation] on [%s] in Admin State [%s] is a NoOp. Continuing..\n",
             entity->name.value,
             CL_AMS_STRING_A_STATE(adminState)));
    }
    else if ( CL_GET_ERROR_CODE(rc) == CL_ERR_BAD_OPERATION )
    {
        AMS_ENTITY_LOG (entity, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("Admin Operation [Lock Instantiation] on [%s] in Admin State [%s] is not possible. Continuing..\n",
             entity->name.value,
             CL_AMS_STRING_A_STATE(adminState)));
    }
    else
    {
        AMS_ENTITY_LOG (entity, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("Admin Operation [Lock Instantiation] on [%s] in Admin State [%s] returned Error [0x%x]\n",
             entity->name.value,
             CL_AMS_STRING_A_STATE(adminState),
             rc));
    }

    return rc;
}

/*
 * clAmsPeEntityLockAssignment
 * ---------------------------
 * This event informs the AMS PE that the entity is now administratively
 * locked and cannot be assigned for use. (It can be instantiated) If the 
 * entity is in use, it is gracefully removed.
 */

ClRcT
clAmsPeEntityLockAssignment(
        CL_IN       ClAmsEntityT        *entity)
{
    ClRcT rc = CL_OK;
    ClAmsAdminStateT adminState;

    AMS_OP_INCR(&gAms.ops);

    AMS_CHECKPTR ( !entity );

    AMS_FUNC_ENTER ( ("Entity [%s]\n",entity->name.value) );

    AMS_CALL ( clAmsEntityValidate(
                    (ClAmsEntityT *) entity,
                    CL_AMS_ENTITY_VALIDATE_CONFIG) );

    if ( (rc = clAmsPeEntityGetAdminState(entity, &adminState)) == CL_OK )
    {
        if ( adminState == CL_AMS_ADMIN_STATE_LOCKED_A )
        {
            rc = CL_AMS_RC(CL_ERR_NO_OP);
        }
        else if ( adminState == CL_AMS_ADMIN_STATE_SHUTTINGDOWN )
        {
            rc = CL_AMS_RC(CL_ERR_BAD_OPERATION);
        }
        else
        {
            switch ( entity->type )
            {
                case CL_AMS_ENTITY_TYPE_CLUSTER:
                {
                    rc = CL_AMS_RC(CL_AMS_ERR_INVALID_ENTITY);

                    break;
                }

                case CL_AMS_ENTITY_TYPE_APP:
                {
                    rc = CL_AMS_RC(CL_AMS_ERR_INVALID_ENTITY);

                    break;
                }

                case CL_AMS_ENTITY_TYPE_SG:
                {
                    rc = clAmsPeSGLockAssignment((ClAmsSGT *) entity);

                    break;
                }

                case CL_AMS_ENTITY_TYPE_NODE:
                {
                    rc = clAmsPeNodeLockAssignment((ClAmsNodeT *) entity);

                    break;
                }


                case CL_AMS_ENTITY_TYPE_SU:
                {
                    rc = clAmsPeSULockAssignment((ClAmsSUT *) entity);

                    break;
                }

                case CL_AMS_ENTITY_TYPE_SI:
                {
                    rc = clAmsPeSILockAssignment((ClAmsSIT *) entity);

                    break;
                }

                default:
                {
                    rc = CL_AMS_RC(CL_AMS_ERR_INVALID_ENTITY);
                }
            }
        }
    }

    if ( rc == CL_OK )
    {
        AMS_ENTITY_LOG (entity, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("Admin Operation [Lock Assignment] on [%s] in Admin State [%s] returned Okay\n",
             entity->name.value,
             CL_AMS_STRING_A_STATE(adminState)));
    }
    else if ( CL_GET_ERROR_CODE(rc) == CL_AMS_ERR_INVALID_ENTITY )
    {
        AMS_ENTITY_LOG (entity, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("Admin Operation [Lock Assignment] on [%s] is invalid. Continuing..\n",
             entity->name.value));
    }
    else if ( CL_GET_ERROR_CODE(rc) == CL_ERR_NO_OP )
    {
        AMS_ENTITY_LOG (entity, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("Admin Operation [Lock Assignment] on [%s] in Admin State [%s] is a NoOp. Continuing..\n",
             entity->name.value,
             CL_AMS_STRING_A_STATE(adminState)));
    }
    else if ( CL_GET_ERROR_CODE(rc) == CL_ERR_BAD_OPERATION )
    {
        AMS_ENTITY_LOG (entity, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("Admin Operation [Lock Assignment] on [%s] in Admin State [%s] is not possible. Continuing..\n",
             entity->name.value,
             CL_AMS_STRING_A_STATE(adminState)));
    }
    else
    {
        AMS_ENTITY_LOG (entity, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("Admin Operation [Lock Assignment] on [%s] in Admin State [%s] returned Error [0x%x]\n",
             entity->name.value,
             CL_AMS_STRING_A_STATE(adminState),
             rc));
    }

    return rc;
}

/*
 * clAmsPeEntityShutdown
 * ---------------------
 * This event informs the AMS PE that the entity should be shutdown
 * gracefully.
 */

ClRcT
clAmsPeEntityShutdown(
        CL_IN       ClAmsEntityT        *entity)
{
    ClRcT rc = CL_OK;
    ClAmsAdminStateT adminState;

    AMS_OP_INCR(&gAms.ops);

    AMS_CHECKPTR ( !entity );

    AMS_FUNC_ENTER ( ("Entity [%s]\n",entity->name.value) );

    AMS_CALL ( clAmsEntityValidate(
                    (ClAmsEntityT *) entity,
                    CL_AMS_ENTITY_VALIDATE_CONFIG) );

    if ( (rc = clAmsPeEntityGetAdminState(entity, &adminState)) == CL_OK )
    {
        if ( adminState == CL_AMS_ADMIN_STATE_SHUTTINGDOWN )
        {
            rc = CL_AMS_RC(CL_ERR_NO_OP);
        }
        else if ( adminState != CL_AMS_ADMIN_STATE_UNLOCKED )
        {
            rc = CL_AMS_RC(CL_ERR_BAD_OPERATION);
        }
        else
        {
            switch ( entity->type )
            {
                case CL_AMS_ENTITY_TYPE_CLUSTER:
                {
                    rc = CL_AMS_RC(CL_AMS_ERR_INVALID_ENTITY);

                    break;
                }

                case CL_AMS_ENTITY_TYPE_APP:
                {
                    rc = CL_AMS_RC(CL_AMS_ERR_INVALID_ENTITY);

                    break;
                }

                case CL_AMS_ENTITY_TYPE_SG:
                {
                    rc = clAmsPeSGShutdown((ClAmsSGT *) entity);

                    break;
                }

                case CL_AMS_ENTITY_TYPE_NODE:
                {
                    rc = clAmsPeNodeShutdown((ClAmsNodeT *) entity);

                    break;
                }

                case CL_AMS_ENTITY_TYPE_SU:
                {
                    rc = clAmsPeSUShutdown((ClAmsSUT *) entity);

                    break;
                }

                case CL_AMS_ENTITY_TYPE_SI:
                {
                    rc = clAmsPeSIShutdown((ClAmsSIT *) entity);

                    break;
                }

                default:
                {
                    rc = CL_AMS_RC(CL_AMS_ERR_INVALID_ENTITY);
                }
            }
        }
    }

    if ( rc == CL_OK )
    {
        AMS_ENTITY_LOG (entity, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("Admin Operation [Shutdown] on [%s] in Admin State [%s] returned Okay\n",
             entity->name.value,
             CL_AMS_STRING_A_STATE(adminState)));
    }
    else if ( CL_GET_ERROR_CODE(rc) == CL_AMS_ERR_INVALID_ENTITY )
    {
        AMS_ENTITY_LOG (entity, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("Admin Operation [Shutdown] on [%s] is invalid. Continuing..\n",
             entity->name.value));
    }
    else if ( CL_GET_ERROR_CODE(rc) == CL_ERR_NO_OP )
    {
        AMS_ENTITY_LOG (entity, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("Admin Operation [Shutdown] on [%s] in Admin State [%s] is a NoOp. Continuing..\n",
             entity->name.value,
             CL_AMS_STRING_A_STATE(adminState)));
    }
    else if ( CL_GET_ERROR_CODE(rc) == CL_ERR_BAD_OPERATION )
    {
        AMS_ENTITY_LOG (entity, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("Admin Operation [Shutdown] on [%s] in Admin State [%s] is not possible. Continuing..\n",
             entity->name.value,
             CL_AMS_STRING_A_STATE(adminState)));
    }
    else
    {
        AMS_ENTITY_LOG (entity, CL_AMS_MGMT_SUB_AREA_MSG, CL_DEBUG_TRACE,
            ("Admin Operation [Shutdown] on [%s] in Admin State [%s] returned Error [0x%x]\n",
             entity->name.value,
             CL_AMS_STRING_A_STATE(adminState),
             rc));
    }

    return rc;
}

/*
 * clAmsPeEntityRestart
 * --------------------
 * This event informs the AMS PE that the entity under consideration
 * should be restarted. A restart includes one of the below sequences
 * on the concerned entity:
 *
 * terminate + instantiate           : graceful termination
 * cleanup + instantiate             : forced termination due to fault
 * terminate + cleanup + instantiate : fault during graceful termination
 */

ClRcT
clAmsPeEntityRestart(
        CL_IN       ClAmsEntityT        *entity,
        CL_IN       ClUint32T           mode)
{
    AMS_OP_INCR(&gAms.ops);

    AMS_CHECKPTR ( !entity );

    AMS_FUNC_ENTER ( ("Entity [%s]\n",entity->name.value) );

    switch ( entity->type ) 
    {
        case CL_AMS_ENTITY_TYPE_CLUSTER:
        {
            return CL_AMS_RC(CL_AMS_ERR_INVALID_ENTITY);
        }

        case CL_AMS_ENTITY_TYPE_APP:
        {
            return CL_AMS_RC(CL_AMS_ERR_INVALID_ENTITY);
        }

        case CL_AMS_ENTITY_TYPE_NODE:
        {
            return clAmsPeNodeRestart((ClAmsNodeT*) entity, mode);
        }

        case CL_AMS_ENTITY_TYPE_SU:
        {
            return clAmsPeSUAdminRestart((ClAmsSUT*) entity, mode);
        }

        case CL_AMS_ENTITY_TYPE_COMP:
        {
            return clAmsPeCompAdminRestart((ClAmsCompT*) entity, mode);
        }

        default:
        {
            return CL_AMS_RC(CL_AMS_ERR_INVALID_ENTITY);
        }
    }
}

/*
 * clAmsPeEntityRepaired
 * ---------------------
 * This event informs the AMS PE that the entity can be considered ready 
 * again for use. 
 */

ClRcT
clAmsPeEntityRepaired(
        CL_IN       ClAmsEntityT        *entity)
{
    AMS_OP_INCR(&gAms.ops);

    AMS_CHECKPTR ( !entity );

    AMS_FUNC_ENTER ( ("Entity [%s]\n",entity->name.value) );

    switch ( entity->type ) 
    {
        case CL_AMS_ENTITY_TYPE_SU:
        {
            return clAmsPeSURepaired((ClAmsSUT*) entity);
        }

        case CL_AMS_ENTITY_TYPE_NODE:
        {
            return clAmsPeNodeRepaired((ClAmsNodeT*) entity);
        }

        default:
        {
            return CL_AMS_RC(CL_AMS_ERR_INVALID_ENTITY);
        }
    }
}

/*-----------------------------------------------------------------------------
 * Other management API functions
 *---------------------------------------------------------------------------*/
 
/*
 * clAmsPeEntityAdd
 * ----------------
 * This event informs the AMS PE that a new AMS entity has been added and
 * should be used.
 */

ClRcT
clAmsPeEntityAdd(
        CL_IN       ClAmsEntityT        *entity)
{
    AMS_FUNC_ENTER ( ("Entity [%s]\n",entity->name.value) );

    // Dynamic addition of entities not supported

    return CL_AMS_RC(CL_ERR_NOT_IMPLEMENTED);
}

/*
 * clAmsPeEntityDelete
 * -------------------
 * This event informs the AMS PE that a new AMS entity should be deleted
 * and should be removed from use.
 */

ClRcT
clAmsPeEntityDelete(
        CL_IN       ClAmsEntityT        *entity)
{
    AMS_FUNC_ENTER ( ("Entity [%s]\n",entity->name.value) );

    // Dynamic addition of entities not supported

    return CL_AMS_RC(CL_ERR_NOT_IMPLEMENTED);
}

/*
 * clAmsPeEntityInstantiate
 * ------------------------
 * This event informs the AMS PE that an AMS entity in the database should 
 * be started.  This function can only be invoked for components, SUs and 
 * nodes, ie all entities that have a presence state.
 *
 * Note: This fn is only for testing. Instantiation of entities is normally
 * decided by AMS.
 */

ClRcT
clAmsPeEntityInstantiate(
        CL_IN       ClAmsEntityT        *entity)

{
    AMS_CHECKPTR ( !entity );

    AMS_FUNC_ENTER ( ("Entity [%s]\n",entity->name.value) );

    switch ( entity->type ) 
    {
        case CL_AMS_ENTITY_TYPE_COMP:
        {
            return clAmsPeCompInstantiate((ClAmsCompT*) entity);
        }

        case CL_AMS_ENTITY_TYPE_SU:
        {
            return clAmsPeSUInstantiate((ClAmsSUT*) entity);
        }

        case CL_AMS_ENTITY_TYPE_NODE:
        {
            return clAmsPeNodeInstantiate((ClAmsNodeT*) entity);
        }

        default:
        {
            return CL_AMS_RC(CL_AMS_ERR_INVALID_ENTITY);
        }
    }
}

/*
 * clAmsPeEntityTerminate
 * ----------------------
 * This event informs the AMS PE that the entity under consideration
 * should be terminated.
 *
 * Note: This fn is only for testing. Termination of entities is normally
 * decided by AMS.
 */

ClRcT
clAmsPeEntityTerminate(
        CL_IN   ClAmsEntityT        *entity)

{
    AMS_CHECKPTR ( !entity );

    AMS_FUNC_ENTER ( ("Entity [%s]\n",entity->name.value) );

    switch ( entity->type ) 
    {
        case CL_AMS_ENTITY_TYPE_COMP:
        {
            return clAmsPeCompTerminate((ClAmsCompT*) entity);
        }

        case CL_AMS_ENTITY_TYPE_SU:
        {
            return clAmsPeSUTerminate((ClAmsSUT*) entity);
        }

        case CL_AMS_ENTITY_TYPE_NODE:
        {
            return clAmsPeNodeTerminate((ClAmsNodeT*) entity);
        }

        default:
        {
            return CL_AMS_RC(CL_AMS_ERR_INVALID_ENTITY);
        }
    }
}

/*
 * clAmsPeEntitySwitchover
 * -----------------------
 * This event informs the AMS PE that the workloads assigned to the
 * entity under consideration should be switchedover. The entity
 * under consideration is restarted if permitted.
 */

ClRcT clAmsPeEntitySwitchover(
        CL_IN       ClAmsEntityT        *entity,
        CL_IN       ClUint32T           mode)
{
    return CL_AMS_RC(CL_ERR_NOT_IMPLEMENTED);
}

/*-----------------------------------------------------------------------------
 * Event API functions
 *---------------------------------------------------------------------------*/
 
/*
 * clAmsPeEntityFaultReport
 * ------------------------
 * This event informs the AMS PE that a fault is being reported for the 
 * entity under consideration. This function would typically be called
 * from the event API with the appropriate entity and recovery params.
 *
 * A fault reported via the AMS event API can be only against a component 
 * or a node, but escalation allows it to be reported against a SU and 
 * node as well. If any escalation is done, it is returned to the event
 * API. If the recovery action has changed as a result of the escalation,
 * this is also returned to the event API.
 */

ClRcT
clAmsPeEntityFaultReport(
        CL_IN       ClAmsEntityT                *entity,
        CL_INOUT    ClAmsLocalRecoveryT         *recovery,
        CL_INOUT    ClUint32T                   *escalation)
{
    AMS_OP_INCR(&gAms.ops);

    AMS_CHECKPTR ( !entity || !recovery || !escalation );

    /*
     * This is the entry point, so set escalation to zero.
     */

    *escalation = CL_FALSE;

    switch ( entity->type )
    {
        case CL_AMS_ENTITY_TYPE_COMP:
        {
            return clAmsPeCompFaultReport((ClAmsCompT *) entity, recovery, escalation);
        }

        case CL_AMS_ENTITY_TYPE_NODE:
        {
            return clAmsPeNodeFaultReport(
                        (ClAmsNodeT*) entity,
                        NULL,
                        recovery,
                        escalation);
        }

        default:
        {
            AMS_LOG(CL_DEBUG_ERROR, 
                ("Error: Fault reported for invalid entity type = %d\n",
                 entity->type));
        }
    }

    return CL_OK;
}

/*
 * clAmsPeEntityRecoveryScopeLarger
 * --------------------------------
 * Returns true if recovery a has larger scope than recovery b.
 *
 * - a comp restart can be received while a su restart is in progress 
 *   and it may be a valid new fault. However, we treat it as lower in
 *   scope than su restart to avoid a situation where there is a su
 *   and comp restart in progress at the same time.
 *
 * - node faults and app restart are mutually larger in scope 
 *   than the other.
 */

ClRcT
clAmsPeEntityRecoveryScopeLarger(
        CL_IN ClAmsRecoveryT a,
        CL_IN ClAmsRecoveryT b)
{
    
    if ( a == b )
    {
        return CL_TRUE;
    }

    switch ( a )
    {
        case CL_AMS_RECOVERY_NONE:
        {
            break;
        }

        case CL_AMS_RECOVERY_COMP_RESTART:
        {
            if ( b == CL_AMS_RECOVERY_NONE )
            {
                return CL_TRUE;
            }

            break;
        }

        case CL_AMS_RECOVERY_SU_RESTART:
        {
            if ( (b == CL_AMS_RECOVERY_NONE)            ||
                 (b == CL_AMS_RECOVERY_COMP_RESTART) )
            {
                return CL_TRUE;
            }

            break;
        }

        case CL_AMS_RECOVERY_COMP_FAILOVER:
        case CL_AMS_RECOVERY_INTERNALLY_RECOVERED:
        {
            if ( (b == CL_AMS_RECOVERY_NONE)            ||
                 (b == CL_AMS_RECOVERY_COMP_RESTART)    ||
                 (b == CL_AMS_RECOVERY_SU_RESTART) )
            {
                return CL_TRUE;
            }
            
            break;
        }

        case CL_AMS_RECOVERY_NODE_SWITCHOVER:
        {
            if ( (b == CL_AMS_RECOVERY_NONE)            ||
                 (b == CL_AMS_RECOVERY_COMP_RESTART)    ||
                 (b == CL_AMS_RECOVERY_SU_RESTART)      ||
                 (b == CL_AMS_RECOVERY_COMP_FAILOVER)   ||
                 (b == CL_AMS_RECOVERY_INTERNALLY_RECOVERED) ||
                 (b == CL_AMS_RECOVERY_APP_RESTART) )
            {
                return CL_TRUE;
            }
            
            break;
        }

        case CL_AMS_RECOVERY_NODE_FAILOVER:
        {
            if ( (b == CL_AMS_RECOVERY_NONE)            ||
                 (b == CL_AMS_RECOVERY_COMP_RESTART)    ||
                 (b == CL_AMS_RECOVERY_SU_RESTART)      ||
                 (b == CL_AMS_RECOVERY_COMP_FAILOVER)   ||
                 (b == CL_AMS_RECOVERY_INTERNALLY_RECOVERED) ||
                 (b == CL_AMS_RECOVERY_NODE_SWITCHOVER) ||
                 (b == CL_AMS_RECOVERY_NODE_FAILOVER)   ||
                 (b == CL_AMS_RECOVERY_APP_RESTART) )
            {
                return CL_TRUE;
            }
            
            break;
        }

        case CL_AMS_RECOVERY_NODE_FAILFAST:
        {
            if ( (b == CL_AMS_RECOVERY_NONE)            ||
                 (b == CL_AMS_RECOVERY_COMP_RESTART)    ||
                 (b == CL_AMS_RECOVERY_SU_RESTART)      ||
                 (b == CL_AMS_RECOVERY_COMP_FAILOVER)   ||
                 (b == CL_AMS_RECOVERY_INTERNALLY_RECOVERED) ||
                 (b == CL_AMS_RECOVERY_NODE_SWITCHOVER) ||
                 (b == CL_AMS_RECOVERY_NODE_FAILOVER)   ||
                 (b == CL_AMS_RECOVERY_APP_RESTART) )
            {
                return CL_TRUE;
            }
            
            break;
        }

        case CL_AMS_RECOVERY_NODE_HALT:
        {
            /*
             * Supercedes everything else.
             */
            return CL_TRUE;
        }
        break;

        case CL_AMS_RECOVERY_APP_RESTART:
        {
            if ( (b == CL_AMS_RECOVERY_NONE)            ||
                 (b == CL_AMS_RECOVERY_COMP_RESTART)    ||
                 (b == CL_AMS_RECOVERY_SU_RESTART)      ||
                 (b == CL_AMS_RECOVERY_COMP_FAILOVER)   ||
                 (b == CL_AMS_RECOVERY_INTERNALLY_RECOVERED) ||
                 (b == CL_AMS_RECOVERY_NODE_SWITCHOVER) ||
                 (b == CL_AMS_RECOVERY_NODE_FAILOVER)   ||
                 (b == CL_AMS_RECOVERY_NODE_FAILFAST) )
            {
                return CL_TRUE;
            }
            
            break;
            
        }

        case CL_AMS_RECOVERY_CLUSTER_RESET:
        default:
        {
            return CL_FALSE;
        }
    }

    return CL_FALSE;
}

/*
 * clAmsPeEntityComputeFaultEscalation
 * -----------------------------------
 * This fn computes if a new fault on a component/su/node should be treated
 * as being escalated, ie, there was a pending fault before and the new 
 * fault is a secondary fault. The escalation is then used in the computation
 * of the recovery action.
 *
 * This fn is only used internally. All external faults arrive with no
 * escalation as set in clAmsPeEntityFaultReport. This allows the recovery
 * computation logic to ignore potentially duplicate faults.
 */

ClRcT
clAmsPeEntityComputeFaultEscalation(
        CL_IN ClAmsEntityT *entity)
{
    ClBoolT escalation = CL_FALSE;

    AMS_CHECKPTR ( !entity );

    switch ( entity->type )
    {
        case CL_AMS_ENTITY_TYPE_COMP:
        {
            ClAmsCompT  *comp   = (ClAmsCompT *)entity;
            ClAmsSUT    *su     = (ClAmsSUT *) comp->config.parentSU.ptr;
            ClAmsNodeT  *node   = (ClAmsNodeT *) su->config.parentNode.ptr;

            if ( comp->status.recovery ||
                 su->status.recovery   ||
                 node->status.recovery )
            {
                escalation = CL_TRUE;
            }

            break;
        }

        case CL_AMS_ENTITY_TYPE_SU:
        {
            ClAmsSUT    *su     = (ClAmsSUT *) entity;
            ClAmsNodeT  *node   = (ClAmsNodeT *) su->config.parentNode.ptr;

            if ( su->status.recovery   ||
                 node->status.recovery )
            {
                escalation = CL_TRUE;
            }

            break;
        }

        case CL_AMS_ENTITY_TYPE_NODE:
        {
            ClAmsNodeT  *node   = (ClAmsNodeT *) entity;

            if ( node->status.recovery )
            {
                escalation = CL_TRUE;
            }

            break;
        }

        default:
        {
            escalation = CL_FALSE;
        }
    }

    return escalation;
}

ClBoolT clAmsPeIsDbEmpty(ClAmsT *ams)
{
    ClAmsEntityDbT *suDb  = &ams->db.entityDb[CL_AMS_ENTITY_TYPE_SU];
    ClAmsEntityDbT *compDb = &ams->db.entityDb[CL_AMS_ENTITY_TYPE_COMP];
    ClAmsEntityDbT *csiDb  = &ams->db.entityDb[CL_AMS_ENTITY_TYPE_CSI];

    return !(suDb->numEntities && compDb->numEntities && csiDb->numEntities);
}

/*-----------------------------------------------------------------------------
 * AMS PE functions to preprocess the database
 * - This will hopefully disappear in a future release once CW starts marking
 *   CSIs as proxy or not.
 *---------------------------------------------------------------------------*/

ClRcT
clAmsPePreprocessDb(
        CL_IN ClAmsT *ams)
{
    ClCntNodeHandleT csiNodeHandle, csiNextNodeHandle;
    ClCntNodeHandleT compNodeHandle, compNextNodeHandle;
    ClAmsCSIT *csi;
    ClAmsCompT *comp;
    ClRcT rc1, rc2;
    ClAmsEntityDbT *csiDb  = &ams->db.entityDb[CL_AMS_ENTITY_TYPE_CSI];
    ClAmsEntityDbT *compDb = &ams->db.entityDb[CL_AMS_ENTITY_TYPE_COMP];

    AMS_CHECKPTR ( !ams );

    if (clAmsPeIsDbEmpty(ams)) return CL_OK;

    AMS_CALL ( clCntFirstNodeGet(
                csiDb->db,
                &csiNodeHandle) );

    while ( csiNodeHandle )
    {
        AMS_CALL ( clCntNodeUserDataGet(
                    csiDb->db,
                    csiNodeHandle,
                    (ClCntDataHandleT *) &csi ) );

        AMS_CALL ( clCntFirstNodeGet(
                    compDb->db,
                    &compNodeHandle) );

        while ( compNodeHandle )
        {
            AMS_CALL ( clCntNodeUserDataGet(
                        compDb->db,
                        compNodeHandle,
                        (ClCntDataHandleT *) &comp ) );
            
            /*
             * We have a csi and a comp.
             */

            if ( !memcmp(comp->config.proxyCSIType.value, 
                        csi->config.type.value, 
                        csi->config.type.length) )
            {
                csi->config.isProxyCSI = CL_TRUE;
            }

            rc2 = clCntNextNodeGet(
                    compDb->db,
                    compNodeHandle,
                    &compNextNodeHandle);

            if ( CL_GET_ERROR_CODE(rc2) == CL_ERR_NOT_EXIST ) break;
            if ( rc2 != CL_OK )
                return rc2;

            compNodeHandle = compNextNodeHandle;
        }

        rc1 = clCntNextNodeGet(
                csiDb->db,
                csiNodeHandle,
                &csiNextNodeHandle);

        if ( CL_GET_ERROR_CODE(rc1) == CL_ERR_NOT_EXIST ) break;
        if ( rc1 != CL_OK )
            return rc1;

        csiNodeHandle = csiNextNodeHandle;
    }

    return CL_OK; 
}
 

/*
 * Run the auto adjust procedure on the SG
 * The rotation of SIs is restricted to the max. number of standby
 * So if there are actives having lower ranks than standby, than they
 * they would be reverted. The below could lead to a series of SI relocations.
 * based on the current configuration.
 */


ClRcT clAmsPeSGAutoAdjust(ClAmsSGT *sg)
{
    if(!sg->config.autoAdjust) return CL_OK;
    switch(sg->config.redundancyModel)
    {
    case CL_AMS_SG_REDUNDANCY_MODEL_NO_REDUNDANCY:
    case CL_AMS_SG_REDUNDANCY_MODEL_TWO_N:
    case CL_AMS_SG_REDUNDANCY_MODEL_M_PLUS_N:
        return clAmsPeSGAutoAdjustMPlusN(sg);
    case CL_AMS_SG_REDUNDANCY_MODEL_CUSTOM:
        return clAmsPeSGAutoAdjustCustom(sg);
    default:
        break;
    }
    return CL_AMS_RC(CL_ERR_NOT_IMPLEMENTED);
}

ClRcT clAmsPeSGAdjust(ClAmsSGT *sg, ClUint32T enable)
{
    if(!enable)
    {
        sg->config.autoAdjust = CL_FALSE;
        return CL_OK;
    }
    if(sg->config.autoAdjust) return CL_OK;
    if(clAmsInvocationsPendingForSG(sg))
    {
        clLogInfo("SG", "ADJUST",
                  "SG [%s] has pending invocations. Deferring adjust",
                  sg->config.entity.name.value);
        return CL_AMS_RC(CL_ERR_TRY_AGAIN);
    }
    sg->config.autoAdjust = CL_TRUE;
    /*
     * Start the auto adjust probation timer to reset adjustments.
     */
    AMS_CALL ( clAmsEntityTimerStart(&sg->config.entity, CL_AMS_SG_TIMER_ADJUST_PROBATION) );
    return clAmsPeSGAutoAdjust(sg);
}

ClRcT clAmsPeSGAdjustTimeout(ClAmsEntityTimerT *timer)
{
    ClAmsSGT *sg = (ClAmsSGT*)timer->entity;
    clLogInfo("SG", "ADJUST", "Running SG [%.*s] evaluation after adjust timeout of [%d] ms",
              sg->config.entity.name.length-1, sg->config.entity.name.value,
              CL_AMS_SG_ADJUST_DURATION);
    clAmsEntityTimerStop((ClAmsEntityT*)sg, CL_AMS_SG_TIMER_ADJUST);
    AMS_CALL ( clAmsPeSGEvaluateWork(sg) );
    return CL_OK;
}

ClRcT clAmsPeSGAdjustProbationTimeout(ClAmsEntityTimerT *timer)
{
    ClAmsSGT *sg = (ClAmsSGT*)timer->entity;
    clLogInfo("SG", "ADJUST", "Running SG [%.*s] adjust probation after timeout of [%d] ms",
              sg->config.entity.name.length-1, sg->config.entity.name.value,
              CL_AMS_SG_ADJUST_PROBATION);
    clAmsEntityTimerStop(&sg->config.entity, CL_AMS_SG_TIMER_ADJUST_PROBATION);
    if(clAmsInvocationsPendingForSG(sg))
    {
        clLogInfo("SG", "ADJUST-PROBE", "SG [%.*s] has pending invocations."
                  "Adjustment likely in progress. Scheduling a probe in [%d] ms",
                  sg->config.entity.name.length-1, sg->config.entity.name.value,
                  CL_AMS_SG_ADJUST_PROBATION);
        AMS_CALL ( clAmsEntityTimerStart(&sg->config.entity, CL_AMS_SG_TIMER_ADJUST_PROBATION) );
        return CL_OK;
    }
    clLogDebug("SG", "ADJUST-PROBE", "Resetting auto adjust for SG [%.*s] as adjustments are done",
               sg->config.entity.name.length-1, sg->config.entity.name.value);
    sg->config.autoAdjust = CL_FALSE;
    return CL_OK;
}

ClRcT clAmsPeSUAssignmentTimeout(ClAmsEntityTimerT *timer)
{
    ClAmsSUT *su  = (ClAmsSUT*)timer->entity;
    ClAmsSGT *sg =  (ClAmsSGT*)su->config.parentSG.ptr;

    clAmsEntityTimerStop((ClAmsEntityT*)su, CL_AMS_SU_TIMER_ASSIGNMENT);
    clLogInfo("SU", "ASSGN-DELAY", "Running SG [%.*s] evaluation after SU [%.*s] "
              "assignment  timeout of [%d] ms",
              sg->config.entity.name.length-1, sg->config.entity.name.value,
              su->config.entity.name.length-1, su->config.entity.name.value,
              CL_AMS_SU_ASSIGNMENT_DELAY);
    AMS_CALL ( clAmsPeSGEvaluateWork(sg) );
    return CL_OK;
}

ClRcT clAmsPeSUProbationTimeout(ClAmsEntityTimerT *timer)
{
    ClAmsSUT *su = (ClAmsSUT*)timer->entity;
    ClAmsSGT *sg = (ClAmsSGT*)su->config.parentSG.ptr;
    clAmsEntityTimerStop(timer->entity, CL_AMS_SU_TIMER_PROBATION);
    clLogInfo("SG", "ADJUST", "Probation period [%lld] milliseconds for SU [%.*s] over. Readjusting SG [%.*s]",
              sg->config.autoAdjustProbation,su->config.entity.name.length-1, su->config.entity.name.value,
              sg->config.entity.name.length-1, sg->config.entity.name.value);
    AMS_CALL ( clAmsPeSUEvaluateWork(su) );
    return CL_OK;
}

/*
 * When an SU is about to be assigned as standby,
 * or when an SI unlocked, do an SG realignment.
 */

ClRcT
clAmsPeSGRealignSU(ClAmsSGT *sg, ClAmsSUT *su, ClAmsHAStateT *newHAState)
{
    ClRcT rc = CL_OK;
    ClAmsEntityRefT *eRef = NULL;
    ClAmsSUT *lowestRankedActiveSU = NULL;

    if(!sg->config.autoAdjust) return CL_OK;

    if(!su->config.rank) return CL_OK;

    /*
     * If its in the probation window, dont realign.
     */
    if(su->status.suProbationTimer.count) return CL_OK;

    for(eRef = clAmsEntityListGetFirst(&sg->status.assignedSUList);
        eRef != NULL;
        eRef = clAmsEntityListGetNext(&sg->status.assignedSUList, eRef))
    {
        ClAmsSUT *activeSU = (ClAmsSUT*)eRef->ptr;

        if(clAmsPeSUIsAssignable(activeSU) != CL_OK)
            continue;

        if(!activeSU->status.numActiveSIs)
            continue;
        
        if(!activeSU->config.rank)
        {
            lowestRankedActiveSU = activeSU;
        }
        else
        {
            if(activeSU->config.rank > 0 )
            {
                if(lowestRankedActiveSU)
                {
                    if(activeSU->config.rank > lowestRankedActiveSU->config.rank)
                    {
                        lowestRankedActiveSU = activeSU;
                    }
                }
                else
                {
                    lowestRankedActiveSU = activeSU;
                }

            }
        }
    }

    if(lowestRankedActiveSU)
    {
        /*
         * Do nothing incase the rank of the new SU is the least.
         */
        if(lowestRankedActiveSU->config.rank && 
           su->config.rank >= lowestRankedActiveSU->config.rank)
            return CL_OK;

        /*
         * If the incoming SU is ranked higher, then do a failback/swap
         */
        rc = clAmsPeSUSwitchoverWorkActiveAndStandby(lowestRankedActiveSU, 
                                                     NULL,
                                                     CL_AMS_ENTITY_SWITCHOVER_IMMEDIATE);
        if(rc != CL_OK)
        {
            clLogError("SG", "ADJUST", "SU [%.*s] switchover returned [%#x]",
                       lowestRankedActiveSU->config.entity.name.length-1,
                       lowestRankedActiveSU->config.entity.name.value, rc);
            return rc;
        }
        *newHAState = CL_AMS_HA_STATE_ACTIVE;
    }

    return CL_OK;
}

static ClRcT
clAmsPeSGCheckSUHigherRankList(ClAmsSGT *sg, ClAmsSUT *su, ClAmsEntityListT *entityList)
{
    ClAmsEntityRefT *eRef = NULL;
    for(eRef = clAmsEntityListGetFirst(entityList);
        eRef != NULL;
        eRef = clAmsEntityListGetNext(entityList, eRef))
    {
        ClAmsSUT *spareSU = (ClAmsSUT*)eRef->ptr;
        if(!spareSU->config.rank) continue;
        if(spareSU->status.suProbationTimer.count) continue;
        if(spareSU->status.numActiveSIs + spareSU->status.numStandbySIs > 0) continue; /* Already used */        
        if(clAmsPeSUIsAssignable(spareSU) != CL_OK) continue;
        if(!su->config.rank) return CL_OK;
        if(spareSU->config.rank < su->config.rank) return CL_OK;
    }
    return CL_AMS_RC(CL_ERR_NOT_EXIST);
}

/*
 * Check for a higher ranked SU than the given SU in the 
 * instantiated and instantiable list for the SU.
 */

ClRcT
clAmsPeSGCheckSUHigherRank(ClAmsSGT *sg, ClAmsSUT *su,ClAmsSIT *si)
{
    ClRcT rc = CL_OK;
    ClAmsEntityRefT *eRef = NULL;
    
    /*
     * Dont defer unnecessarily incase its a reassignment indicator.
     */
    if(si 
       && 
       clAmsEntityOpPending(&si->config.entity, &si->status.entity,
                            CL_AMS_ENTITY_OP_SI_REASSIGN_MPLUSN)
       &&
       clAmsEntityOpPending(&su->config.entity, &su->status.entity,
                            CL_AMS_ENTITY_OP_SI_REASSIGN_MPLUSN))
        return CL_AMS_RC(CL_ERR_NOT_EXIST);

    for(eRef = clAmsEntityListGetFirst(&sg->status.assignedSUList);
        eRef != NULL;
        eRef = clAmsEntityListGetNext(&sg->status.assignedSUList, eRef))
    {
        ClAmsSUT *standbySU = (ClAmsSUT*)eRef->ptr;
        if(standbySU->status.suProbationTimer.count) continue;
        if(clAmsPeSUIsAssignable(standbySU) != CL_OK) continue;
        if(standbySU->status.numStandbySIs)
        {
            if(!standbySU->config.rank) continue;
            if(!su->config.rank) return CL_OK;
            if(standbySU->config.rank < su->config.rank) 
            {
                clLogInfo("SG", "ADJUST", 
                          "Removing standby assignments from higher ranked SU [%.*s] of rank [%d] "
                          "for active reassignment",
                          standbySU->config.entity.name.length-1, standbySU->config.entity.name.value,
                          standbySU->config.rank);
                clAmsPeSUSwitchoverWorkBySI(standbySU, CL_AMS_ENTITY_SWITCHOVER_IMMEDIATE);
                return CL_OK;
            }
        }
    }

    /*
      Some cases are not handled.
      1. If the SI is currently assigned to an SU NOT in the SISU list, but of a lower numerical rank than something that is in the SISU list.  In that case
         we should swap to the SISU list.
         If anything is free in the SISU list, swap to it if you're not in the list.
      2. If the SI is currently assigned to an SU NOT in the SISU list, then use the SU's rank order to decide whether to swap.      
     */
    
    if ((si)&&(sg->config.loadingStrategy == CL_AMS_SG_LOADING_STRATEGY_BY_SI_PREFERENCE))
    {
        clLogInfo("SG", "ADJUST", 
                          "Using SI SU rank list to find preferred SU for SI [%.*s]",
                           si->config.entity.name.length-1, si->config.entity.name.value
                  );
        
        rc = clAmsPeSGCheckSUHigherRankList(sg, su, &si->config.suList);
        if(rc == CL_OK) return rc;
    }
    else
    {        
        rc = clAmsPeSGCheckSUHigherRankList(sg, su, &sg->status.inserviceSpareSUList);
        if(rc == CL_OK) return rc;

        rc = clAmsPeSGCheckSUHigherRankList(sg, su, &sg->status.instantiatedSUList);
        if(rc == CL_OK) return rc;

        rc = clAmsPeSGCheckSUHigherRankList(sg, su, &sg->status.instantiableSUList);
    }
    
    return rc;

}

ClRcT 
clAmsPeEntityOpReplay(ClAmsEntityT *entity, ClAmsEntityStatusT *status, ClUint32T op, ClBoolT recovery)
{
    ClRcT rc = CL_OK;
    void *data = NULL;
    ClUint32T dataSize = 0;

    if(!status)
        status = clAmsEntityGetStatusStruct(entity);

    if(!status->opStack.numOps) return CL_OK;

    rc = clAmsEntityOpGet(entity, status, op, &data, &dataSize);

    if(rc != CL_OK) return CL_OK;

    switch(op)
    {
    case CL_AMS_ENTITY_OP_REMOVE_MPLUSN:
        rc = clAmsPeEntityOpRemove(entity, data, dataSize, recovery);
        break;

    case CL_AMS_ENTITY_OP_ACTIVE_REMOVE_MPLUSN:
        rc = clAmsPeEntityOpActiveRemove(entity, data, dataSize, recovery);
        break;

    case CL_AMS_ENTITY_OP_SWAP_REMOVE_MPLUSN:
        rc = clAmsPeEntityOpSwapRemove(entity, data, dataSize, recovery);
        break;

    case CL_AMS_ENTITY_OP_SWAP_ACTIVE_MPLUSN:
        rc = clAmsPeEntityOpSwapActive(entity, data, dataSize, recovery);
        break;

    case CL_AMS_ENTITY_OP_REDUCE_REMOVE_MPLUSN:
        rc = clAmsPeEntityOpReduceRemove(entity, data, dataSize, recovery);
        break;

    default: break;
    }

    if(CL_GET_ERROR_CODE(rc) == CL_ERR_TRY_AGAIN)
    {
        rc = CL_OK;
        goto out;
    }

    /*
     * Clear and re-fetch. 
     */
    data = NULL;
    rc = clAmsEntityOpClear(entity, status, op,
                            &data, &dataSize);
    if(data)
        clHeapFree(data);

    out:
    return rc;
}

/*
 * Replay the entity operations pending against this entity.
 */
ClRcT
clAmsPeEntityOpsReplay(ClAmsEntityT *entity, ClAmsEntityStatusT *status, ClBoolT recovery)
{
    void *data = NULL;
    ClUint32T dataSize = 0;
    ClUint32T op = 0;
    CL_LIST_HEAD_DECLARE(tempList);

    if(!status)
        status = clAmsEntityGetStatusStruct(entity);

    if(!status->opStack.numOps) return CL_OK;

    while(clAmsEntityOpPop(entity, status, &data, &dataSize, &op) == CL_OK)
    {
        ClRcT rc = CL_OK;
        switch(op)
        {
        case CL_AMS_ENTITY_OP_REMOVE_MPLUSN:
            {
                rc = clAmsPeEntityOpRemove(entity, data, dataSize, recovery);
            }
            break;
            
        case CL_AMS_ENTITY_OP_ACTIVE_REMOVE_MPLUSN:
            {
                rc = clAmsPeEntityOpActiveRemove(entity, data, dataSize, recovery);
            }
            break;

        case CL_AMS_ENTITY_OP_SWAP_REMOVE_MPLUSN:
            {
                rc = clAmsPeEntityOpSwapRemove(entity, data, dataSize, recovery);
            }
            break;

        case CL_AMS_ENTITY_OP_SWAP_ACTIVE_MPLUSN:
            {
                rc = clAmsPeEntityOpSwapActive(entity, data, dataSize, recovery);
            }
            break;

        case CL_AMS_ENTITY_OP_REDUCE_REMOVE_MPLUSN:
            {
                rc = clAmsPeEntityOpReduceRemove(entity, data, dataSize, recovery);
            }
            break;

        default: break;
        }

        if(CL_GET_ERROR_CODE(rc) == CL_ERR_TRY_AGAIN)
        {
            ClAmsEntityOpT *opBlock = clHeapCalloc(1, sizeof(*opBlock));
            CL_ASSERT(opBlock != NULL);
            opBlock->op = op;
            opBlock->data = data;
            opBlock->dataSize = dataSize;
            clListAddTail(&opBlock->list, &tempList);
        }
        else if(data)
        {
            clHeapFree(data);
            data = NULL;
        }
    }

    while(!CL_LIST_HEAD_EMPTY(&tempList))
    {
        ClListHeadT *entry = tempList.pNext;
        ClAmsEntityOpT *opBlock = CL_LIST_ENTRY(entry, ClAmsEntityOpT, list);
        clListDel(entry);
        clAmsEntityOpPush(entity, status, opBlock->data, opBlock->dataSize, opBlock->op);
        clHeapFree(opBlock);
    }

    return CL_OK;
}

ClRcT clAmsPeSGCheckSUAssignmentDelay(ClAmsSGT *sg)
{
    ClAmsEntityRefT *entityRef = NULL;
    for(entityRef = clAmsEntityListGetFirst(&sg->config.suList);
        entityRef != NULL;
        entityRef = clAmsEntityListGetNext(&sg->config.suList, entityRef))
    {
        ClAmsSUT *su = (ClAmsSUT*)entityRef->ptr;
        if(su->status.numDelayAssignments) return CL_OK;
    }
    return CL_AMS_RC(CL_ERR_NOT_EXIST);
}

static ClRcT clAmsPeSGFailoverHistoryRecover(ClAmsNodeT *node, ClAmsCompT *faultyComp)
{
    ClAmsSUT *su = NULL;
    ClAmsSGT *sg = NULL;

    if(!node || !faultyComp)
        return CL_AMS_RC(CL_ERR_INVALID_PARAMETER);

    /*
     * Now check for the SUs ha state of the faulty comp.
     * If the ha state is standby, then just halt the node running the standby.
     * If the ha state is now active, then halt the active as well as all the redundant standby nodes
     * for the component instance
     */
    su = (ClAmsSUT*)faultyComp->config.parentSU.ptr;
    if(!su) 
        return CL_AMS_RC(CL_ERR_NO_OP);
    sg = (ClAmsSGT*)su->config.parentSG.ptr;
    if(!sg)
        return CL_AMS_RC(CL_ERR_NO_OP);

    if(!su->status.numActiveSIs)
    {
        clLogCritical("FAILOVER", "RECOVER", "Shutting down ASP on the standby node [%s] since the SG [%s] "
                      "has undergone %d failovers within %lld ms", node->config.entity.name.value,
                      sg->config.entity.name.value,
                      sg->config.maxFailovers, sg->config.failoverDuration);
        _clAmsSANodeHalt(&node->config.entity.name, node->config.isASPAware, CL_TRUE);
    }
    else
    {
        ClAmsNodeT **nodeList = NULL;
        ClUint32T nodes = 0;
        ClUint32T i;
        ClAmsEntityRefT *siRef = NULL;

        for(siRef = clAmsEntityListGetFirst(&su->status.siList);
            siRef != NULL;
            siRef = clAmsEntityListGetNext(&su->status.siList, siRef))
        {
            ClAmsSIT *si = (ClAmsSIT*)siRef->ptr;
            ClAmsEntityRefT *suRef = NULL;
            for(suRef = clAmsEntityListGetFirst(&si->status.suList);
                suRef != NULL;
                suRef = clAmsEntityListGetNext(&si->status.suList, suRef))
            {
                ClAmsSUT *targetSU = (ClAmsSUT*)suRef->ptr;
                ClAmsNodeT *targetNode = NULL;
                if(!targetSU || targetSU == su || !targetSU->status.numStandbySIs) continue;
                targetNode = (ClAmsNodeT*)su->config.parentNode.ptr;
                if(!targetNode || targetNode == node) continue;
                for(i = 0; i < nodes; ++i)
                {
                    if(nodeList[i] == targetNode)
                        break;
                }
                if(i == nodes)
                {
                    nodeList = clHeapRealloc(nodeList, (nodes+1)*sizeof(*nodeList));
                    CL_ASSERT(nodeList != NULL);
                    nodeList[nodes++] = targetNode;
                }
            }
        }
        
        /*
         * Add the faulty node at the end.
         */
        nodeList = clHeapRealloc(nodeList, (nodes+1)*sizeof(*nodeList));
        CL_ASSERT(nodeList != NULL);
        nodeList[nodes++] = node;

        for(i = 0; i < nodes; ++i)
        {
            clLogCritical("FAILOVER", "RECOVER", 
                          "Shutting down ASP on the %s node [%s] since the protected SG [%s] "
                          "has undergone %d failovers within %lld ms", 
                          nodeList[i] == node ? "Active" : "Standby",
                          nodeList[i]->config.entity.name.value,
                          sg->config.entity.name.value,
                          sg->config.maxFailovers, sg->config.failoverDuration);

            if(!i)
                _clAmsSANodeHalt(&nodeList[i]->config.entity.name, node->config.isASPAware, CL_TRUE);
            else
                _clAmsSANodeHalt(&nodeList[i]->config.entity.name, node->config.isASPAware, CL_FALSE);
        }

        clHeapFree(nodeList);
    }
    return CL_OK;
}
