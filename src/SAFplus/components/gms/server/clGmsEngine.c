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
 * ModuleName  : gms                                                           
 * File        : clGmsEngine.c
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *
 * 
 *  This file implements the GMS View internal functions. This file mainly contains
 *  View join/close/addition/deletion function calls. It also contains function
 *  calls to get the view changes and view dump to cli. 
 *
 *
 *
 *
 * TODO:
 *  - The local IP address is hardcoded now.  We must make it either
 *    configurable, or detected automatically.
 *
 *****************************************************************************/

#include <string.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <clCommonErrors.h>
#include <clGmsErrors.h>
#include <clGms.h>
#include <clGmsCli.h>
#include <clGmsEngine.h>
#include <clLogApi.h>
#include <clGmsLog.h>
#include <clIocIpi.h>
#include <clNodeCache.h>
#include <clGmsDb.h>
#include <clGmsMsg.h>
#include <clGmsApiHandler.h>
#include <clGmsRmdServer.h>
#include <clNodeCache.h>
#include <clCpmExtApi.h>

# define CL_MAX_CREDENTIALS     (~0U)
#define __SC_PROMOTE_CREDENTIAL_BIAS (CL_IOC_MAX_NODES+1)

static      ClTimerHandleT  timerHandle = NULL;
ClBoolT     bootTimeElectionDone = CL_FALSE;
static ClBoolT reElect = CL_FALSE;
static ClBoolT timerRestarted = CL_FALSE;
static ClRcT _clGmsEngineClusterJoinWrapper(
                                      CL_IN   const ClGmsGroupIdT   groupId,
                                      CL_IN   const ClGmsNodeIdT    nodeId,
                                      CL_IN   ClGmsViewNodeT* node,
                                      CL_IN   ClBoolT reElection,
                                      CL_IN   ClBoolT *pTrackNotify);

/*
   timerCallback
   -------------
   Get the current view from the database .
   Invoke the leader election algorithm .
   mark that boottime election is done. 
 */

static ClRcT gmsViewPopulate(ClBoolT *pTrackNotify)
{
    ClNodeCacheMemberT *pMembers = NULL;
    ClUint32T maxNodes = CL_IOC_MAX_NODES;
    ClRcT rc = CL_OK;
    ClUint32T i;
    pMembers = clHeapCalloc(CL_IOC_MAX_NODES, sizeof(*pMembers));
    CL_ASSERT(pMembers != NULL);
    rc = clNodeCacheViewGetFastSafe(pMembers, &maxNodes);
    if(rc != CL_OK)
    {
        clLogError("CACHE", "GET", "Node cache view get failed with [%#x]", rc);
        goto out_free;
    }
    for(i = 0; i < maxNodes; ++i)
    {
        _clGmsEngineClusterJoinWrapper(0, pMembers[i].address, NULL, CL_TRUE, pTrackNotify);
    }

    out_free:
    clHeapFree(pMembers);
    return rc;
}

static ClRcT timerCallback( void *arg )
{

    ClRcT rc = CL_OK;
    ClGmsNodeIdT leaderNodeId = CL_GMS_INVALID_NODE_ID;
    ClGmsNodeIdT deputyNodeId = CL_GMS_INVALID_NODE_ID;
    ClGmsNodeIdT lastLeader = CL_GMS_INVALID_NODE_ID;
    ClGmsViewT   *view = NULL;
    ClBoolT trackNotify = CL_FALSE;
    ClBoolT leadershipChanged = CL_FALSE;

    clLog(INFO,LEA,NA,
          "Running boot time leader election after 5 secs of GMS startup");

    gmsViewPopulate(&trackNotify); 

    rc = _clGmsViewFindAndLock( 0x0 , &view );
    CL_ASSERT( (rc == CL_OK) && (view != NULL));

    reElect = CL_FALSE; /* reset re-election trigger*/
    timerRestarted = CL_FALSE;

    rc = _clGmsEngineLeaderElect ( 
                                  0x0,
                                  NULL , 
                                  CL_GMS_MEMBER_JOINED,
                                  &leaderNodeId,
                                  &deputyNodeId
                                   );

    if (rc != CL_OK)
    {
        clLog(ERROR,LEA,NA,
              "Error in boot time leader election. rc [0x%x]",rc);
    }
    lastLeader = view->leader;
    leadershipChanged = (lastLeader != leaderNodeId);
    if(leadershipChanged || (view->deputy != deputyNodeId))     
    {
        if(!trackNotify)
        {
            view->leader = leaderNodeId;
            view->deputy = deputyNodeId;
            view->leadershipChanged = leadershipChanged;
            clEoMyEoObjectSet ( gmsGlobalInfo.gmsEoObject );
            if (_clGmsTrackNotify ( 0x0 ) != CL_OK)
            {
                clLog(ERROR,LEA,NA, "_clGmsTrackNotify failed in leader election timer callback");
            }
        }
    }

    bootTimeElectionDone = CL_TRUE;

    // ready to Serve only to be set when there is Synch done 
    //readyToServeGroups = CL_TRUE;

    // if there is no leader in the group or there is only 1 node in the cluster group, set to true 
    clLog(INFO,LEA,NA, "leader [%d] & noOfViewMemebers [%d] is found!", view->leader, view->noOfViewMembers);
    if (view->leader || view->noOfViewMembers == 1)
    {
    	clLog(INFO,LEA,NA,
              "There is only leader in the default group; hence setting readyToServeGroups as True");
        readyToServeGroups = CL_TRUE;
    }

    if(!timerRestarted)
    {
        clTimerDeleteAsync(&timerHandle); /*delete the timer*/
    }

    if (_clGmsViewUnlock(0x0) != CL_OK)
    {
        clLog(ERROR,LEA,NA,
              ("_clGmsViewUnlock failed in leader election timer callback"));
    }

    clLog(INFO,LEA,NA,
          "Initial boot time leader election complete");
    return rc;
}

static ClRcT leaderElectionTimerRun(ClBoolT restart, ClTimerTimeOutT *pTimeOut)
{
    ClTimerTimeOutT timeOut = {.tsSec = gmsGlobalInfo.config.bootElectionTimeout, .tsMilliSec=0 };
    if(restart && timerHandle)
        clTimerDeleteAsync(&timerHandle);
    if(pTimeOut)  timeOut = *pTimeOut;
    timerRestarted = CL_TRUE;
    clLogNotice(GEN, NA, "Starting boot time election timer for [%d] secs", timeOut.tsSec);
    return clTimerCreateAndStart(
            timeOut,
            CL_TIMER_ONE_SHOT,
            CL_TIMER_TASK_CONTEXT,
            timerCallback,
            NULL,
            &timerHandle 
            );
}

/*
 * Try to keep the totem and xport notification view in sync.
 */

static ClHandleT  gNotificationCallbackHandle = CL_HANDLE_INVALID_VALUE;

static void gmsNotificationCallback(ClIocNotificationIdT eventId, ClPtrT unused, ClIocAddressT *pAddress)
{
    ClRcT rc = CL_OK;
    if(gmsGlobalInfo.opState != CL_GMS_STATE_RUNNING)
        return;
    
    if(eventId == CL_IOC_NODE_LEAVE_NOTIFICATION || eventId == CL_IOC_NODE_LINK_DOWN_NOTIFICATION)
    {
        clLogNotice("NOTIF", "LEAVE", "Triggering node leave for node [%#x], port [%#x]",
                    pAddress->iocPhyAddress.nodeAddress, pAddress->iocPhyAddress.portId);
        rc = _clGmsEngineClusterLeaveExtended(0, pAddress->iocPhyAddress.nodeAddress, CL_TRUE);
    }
    else
    {
#if 0
        if(pAddress->iocPhyAddress.portId != CL_IOC_CPM_PORT)
           return;
#endif
        clLogNotice("NOTIF", "JOIN", "Triggering node join for node [%u], port [%#x]", pAddress->iocPhyAddress.nodeAddress, pAddress->iocPhyAddress.portId);
        rc = _clGmsEngineClusterJoinWrapper(0, pAddress->iocPhyAddress.nodeAddress, NULL, CL_FALSE, NULL);
    }  
    
    if(rc != CL_OK)
    {
        clLogError("NOTIF", "CALLBACK", "GMS engine processing returned with [%#x]", rc);
    }
}


ClRcT clGmsIocNotification(ClEoExecutionObjT *pThis, ClBufferHandleT eoRecvMsg,ClUint8T priority,ClUint8T protoType,ClUint32T length,ClIocPhysicalAddressT srcAddr)
{
    ClIocNotificationT notification = {0};
    ClUint32T len = sizeof(notification);

    clBufferNBytesRead(eoRecvMsg, (ClUint8T*)&notification, &len);

    notification.id = ntohl(notification.id);
    notification.nodeAddress.iocPhyAddress.nodeAddress = ntohl(notification.nodeAddress.iocPhyAddress.nodeAddress);
    notification.nodeAddress.iocPhyAddress.portId = ntohl(notification.nodeAddress.iocPhyAddress.portId);
    
    gmsNotificationCallback(notification.id, 0, &notification.nodeAddress);

    if ((notification.id == CL_IOC_NODE_ARRIVAL_NOTIFICATION) && (notification.nodeAddress.iocPhyAddress.nodeAddress != clIocLocalAddressGet()))
    {
        clLogDebug("NTF", "LEA", "Node [%d] arrival msg len [%u] notif len [%lu]", notification.nodeAddress.iocPhyAddress.nodeAddress,length,sizeof(notification));

        if (length-sizeof(notification) >= sizeof(ClUint32T))  /* leader status is appended onto the end of the message */
        {
            ClUint32T reportedLeader = 0;
            //ClRcT rc = clBufferHeaderTrim(eoRecvMsg, sizeof(notification));
            if (1) // rc == CL_OK)
            {
                ClIocNodeAddressT currentLeader;
                clBufferNBytesRead(eoRecvMsg, (ClUint8T*)&reportedLeader, &len);
                reportedLeader = ntohl(reportedLeader);
                if (clNodeCacheLeaderGet(&currentLeader)==CL_OK)
                {
                    if (currentLeader == reportedLeader)
                    {                        
                    clLogDebug("NTF", "LEA", "Node [%d] reports leader as [%d].  Consistent with this node.", currentLeader, reportedLeader);
                    }
                    else
                    {
                        ClGmsNodeIdT leaderNodeId = CL_GMS_INVALID_NODE_ID;
                        ClGmsNodeIdT deputyNodeId = CL_GMS_INVALID_NODE_ID;

                        clLogAlert("NTF", "LEA", "Split brain.  Node [%d] reports leader as [%d].  Inconsistent with this node's leader [%d]", notification.nodeAddress.iocPhyAddress.nodeAddress, reportedLeader,currentLeader);
                        clNodeCacheLeaderSet(reportedLeader);
                        _clGmsEngineLeaderElect (0x0, NULL , CL_GMS_MEMBER_JOINED, &leaderNodeId, &deputyNodeId);
                    }
                }
                else
                {
                clLogDebug("NTF", "LEA", "Node [%d] reports leader as [%d].", notification.nodeAddress.iocPhyAddress.nodeAddress, reportedLeader);
                clNodeCacheLeaderSet(reportedLeader);
                }
            }
        }
    }
    
    if(eoRecvMsg)
        clBufferDelete(&eoRecvMsg);
    return CL_OK;
}


static void gmsNotificationInitialize(void)
{
    ClIocPhysicalAddressT compAddr = {0};
    /*
     * Don't register for notifications if running under totem to avoid clashes
     * between the two with node views getting synced from different sources
     */
    if(!clAspNativeLeaderElection())
        return;
    compAddr.nodeAddress = CL_IOC_BROADCAST_ADDRESS;
    compAddr.portId = CL_IOC_CPM_PORT;
    clCpmNotificationCallbackInstall(compAddr, gmsNotificationCallback, NULL, &gNotificationCallbackHandle);

    if (1)
    {
        
        ClEoProtoDefT eoProtoDef = 
        {
            CL_IOC_PORT_NOTIFICATION_PROTO,
            "IOC notification to GMS",
            clGmsIocNotification,
            NULL,
            CL_EO_STATE_ACTIVE | CL_EO_STATE_SUSPEND | CL_EO_STATE_THREAD_SAFE
        };

        clEoProtoSwitch(&eoProtoDef);
    }
    
}

static ClRcT gmsClusterStart(void)
{
    ClRcT rc = CL_OK;
    ClUint32T version = CL_VERSION_CODE(5, 0, 0);
    
    clNodeCacheMinVersionGet(NULL, &version);

    if(clAspNativeLeaderElection() && version >= CL_VERSION_CODE(5, 0, 0))
    {
        rc = clCpmComponentRegister(
                                    gmsGlobalInfo.cpmHandle,
                                    &gmsGlobalInfo.gmsComponentName,
                                    NULL
                                    );
        if(rc != CL_OK)
        {
            clLogError("CLUSTER", "START",
                       "clCpmComponent register failed with rc 0x%x", rc);
        }
        else
        {
            clLogMultiline(DBG,"CLUSTER", "START",
                           "clCpmComponentRegister successful. Updating GMS state as RUNNING");
            gmsGlobalInfo.opState = CL_GMS_STATE_RUNNING;
        }

    }
    else
    {
        gClTotemRunning = CL_TRUE;
        clLogMultiline(INFO,GEN,NA,
                       "Invoking aisexec_main call of openais. This thread will continue\n"
                       "to run in openais context until finalize");

        /* The follwoing call into openais will initialize _and_ run the openais
         * code and will block as long as it is interrupted or it crashes
         * (normally it does not exit by itself..
         */
        rc = aisexec_main(0,NULL); /* Will run forever! */
        if (rc != 0)
        {
            rc = CL_GMS_RC(CL_GMS_ERR_TOTEM_PROTOCOL_ERROR);
        }
    }

    return rc;
}    

ClRcT
_clGmsEngineStart()
{
    ClRcT       rc = CL_OK;
    ClGmsNodeIdT    thisNodeId = 0;
    ClNameT         thisNodeName = {0};
    ClGmsLeadershipCredentialsT     credentials = 0;
    
    /* Get the details of this node and set it */
    rc = clCpmLocalNodeNameGet(&thisNodeName);
    if (rc != CL_OK)
    {
        clLog(EMER,GEN,NA,
                "Failed to get this node name from CPM. rc 0x%x",rc);
        return rc;
    }

    thisNodeId = clIocLocalAddressGet();

    /*minimum credentials for eligible SC */
    credentials = thisNodeId;

    if (clCpmIsSC())
    {
        /*Adding extra credentials for original SC's */
        credentials += CL_IOC_MAX_NODES + 1;
    } 
    else if (clParseEnvBoolean("ASP_SC_PROMOTE") != CL_TRUE)
    {
        credentials = CL_GMS_INELIGIBLE_CREDENTIALS;
    }
    
    _clGmsSetThisNodeInfo(thisNodeId, &thisNodeName, credentials);

    /* 
       we install a timer to wait for prospective leaders to join the ring
       during booting of the cluster. After the timer fires the leader election
       is done from the callback and the interested parties are notified if
       there is a change in the leadership.Timer is a one shot timer and is
       delted after it fires .
     */
    rc = leaderElectionTimerRun(CL_FALSE, NULL);
    if(rc != CL_OK)
    {
        clLogMultiline(EMER,GEN,NA,
                "Boot time leader election timer creation failed. rc [0x%x]\nBooting Aborted...",rc);
        exit(0);
    }

    gmsNotificationInitialize();

    gmsViewPopulate(NULL); /* preload a default view for aspinfo */
    
    rc = gmsClusterStart();

    return rc;
}


static __inline__ ClBoolT canElectNodeAsLeader(ClGmsNodeIdT lastLeader, ClGmsClusterMemberT *member)
{
#define CL_GMS_LEADER_SOAK_INTERVAL (gmsGlobalInfo.config.leaderSoakInterval * 1000000L)
    ClTimeT curTimestamp = 0;

    if(gClTotemRunning) return CL_TRUE;
    if(clCpmIsSCCapable()) return CL_TRUE;
    curTimestamp = clOsalStopWatchTimeGet();
    if(member->bootTimestamp 
       && 
       (curTimestamp - member->bootTimestamp) < CL_GMS_LEADER_SOAK_INTERVAL)
    {
        /*
         * If there is an existing leader and we are trying to flip within the soak time,
         * disallow as we are in the failover window where the standby cannot become active.
         */
        if(lastLeader 
           &&
           lastLeader != CL_GMS_INVALID_NODE_ID
           && 
           lastLeader != member->nodeId)
        {
            clLogNotice("CHECK", "LEADER", 
                        "Node [%d] cannot yet become a leader is it is within the leader election soak time",
                        member->nodeId);
            return CL_FALSE;
        }
    }
    return CL_TRUE;
}

/*
   Description
   ------------
   Leader is elected among the members having highest credentials in the
   ring.If there are more than one prospective leaders , the tie is broken
   basing on who arrived first comparing their boottimestamps (NOTE: Assuming
   the clocks of all the nodes are in sync or atleast that of the prospective
   leaders).
 */

static ClRcT
findNodeWithHigestCredential (ClGmsClusterMemberT **nodes,
                              ClUint32T             noOfNodes,
                              ClGmsNodeIdT         *nodeId)
{
    ClUint32T   i = 0;
    ClUint32T   higestCredentialIndex = 0;
    ClUint32T   maxCredential = 0;
     
    *nodeId = CL_GMS_INVALID_NODE_ID;
    
    for (i = 0; i < noOfNodes; i++)
    {   
        if (nodes[i] != NULL && nodes[i]->credential > maxCredential)
        {
            higestCredentialIndex = i;
            maxCredential = nodes[i]->credential;
        }
    }

    if (NULL != nodes[higestCredentialIndex])
    {
        *nodeId = nodes[higestCredentialIndex]->nodeId;
    }
    
    return CL_OK;
}

static ClRcT 
computeLeaderDeputyWithHighestCredential (
        const  ClGmsClusterNotificationBufferT* const buffer,
        ClGmsNodeIdT*   const                   leaderNodeId,
        ClGmsNodeIdT*   const                   deputyNodeId )
{

    const ClRcT          rc = CL_OK;
    
    ClGmsClusterMemberT *currentNode = NULL;
    ClGmsClusterMemberT *eligibleNodes[64] = {0};
    ClUint32T            noOfEligibleNodes = 0;
    
    ClGmsClusterMemberT *existingLeaders[64] = {0};
    ClUint32T            noOfExistingLeaders = 0;
    ClUint32T            i = 0;

    CL_ASSERT( (buffer != (const void*)NULL) && (buffer->numberOfItems != 0x0) && (buffer->notification != NULL));
    CL_ASSERT( (leaderNodeId != NULL) && (deputyNodeId != NULL));

    /* From the notification buffer find the list of nodes which
     * have eligible credentials */
    
    for ( i = 0 ; i < buffer->numberOfItems ; i++ )
    {
        currentNode = &(buffer->notification[i].clusterNode);
        if( currentNode->credential != CL_GMS_INELIGIBLE_CREDENTIALS )
        {
            eligibleNodes[noOfEligibleNodes] = currentNode;
            if (eligibleNodes[noOfEligibleNodes]->isPreferredLeader == CL_TRUE)
            {
                /*
                 * If the leaderpreference was set from the debug cli, that has a higher affinity
                 * then preferred ones as it overrides the preferred if any.
                 */
                eligibleNodes[noOfEligibleNodes]->credential = 
                    CL_MAX_CREDENTIALS - (eligibleNodes[noOfEligibleNodes]->isPreferredLeader 
                                          ^
                                          eligibleNodes[noOfEligibleNodes]->leaderPreferenceSet);
            }
            noOfEligibleNodes++;
        }
    }
    
    if(noOfEligibleNodes == 0)
    {
        clLogMultiline(WARN,LEA,NA,
                 "Could not elect any leader from the current cluster view.\n"
                 "Possibly no system controller is running");
        return rc;
    }

    /* A preferred leader node is not found. So we fall back to
     * our old algorithm:
     * Find a node with highest node id as the leader. If there is
     * already an existing leader node, then dont elect a new leader.
     */
    for (i = 0; i < noOfEligibleNodes; i++)
    {
        if (eligibleNodes[i]->isCurrentLeader == CL_TRUE 
            ||
            CL_NODE_CACHE_SC_PROMOTE_CAPABILITY(eligibleNodes[i]->isCurrentLeader)
            ||
            eligibleNodes[i]->isPreferredLeader)
        {
            if(CL_NODE_CACHE_SC_PROMOTE_CAPABILITY(eligibleNodes[i]->isCurrentLeader))
            {
                eligibleNodes[i]->credential += __SC_PROMOTE_CREDENTIAL_BIAS;
            }
            eligibleNodes[i]->isCurrentLeader &= ~__SC_PROMOTE_CAPABILITY_MASK;
            existingLeaders[noOfExistingLeaders++] = eligibleNodes[i];
        }
    }

    clLog(DBG,CLM,NA,
            "No of  existing leaders = %d",noOfExistingLeaders);
    if (noOfExistingLeaders)
    {
        /* There are already leader nodes in the cluster.
         * Elect the one among those leaders with the highest nodeId
         */
        findNodeWithHigestCredential(existingLeaders, noOfExistingLeaders, leaderNodeId);
    } else {
        /* There was no existing leader. So elect the node with the
         * highest node id among the nodes with highest credentials
         */
        findNodeWithHigestCredential(eligibleNodes, noOfEligibleNodes, leaderNodeId);
    }

    /* Now to elect deputy, among the eligible nodes
     * mark the leader node index as NULL and among the remaining
     * nodes, elect the one with highest node Id
     */
    for (i = 0; i < noOfEligibleNodes; i++)
    {
        if (eligibleNodes[i]->nodeId == *leaderNodeId)
        {
            eligibleNodes[i] = NULL;
            break;
        }
    }

    findNodeWithHigestCredential(eligibleNodes, noOfEligibleNodes, deputyNodeId);

    clLogNotice(LEA, "ELECT",
            "Results of leader election: Leader [%d], Deputy [%d]",
            *leaderNodeId,*deputyNodeId);
    return rc;
}



/* Default algorithm used by the GMS engine.
   If the current node being joined has the same credentials as the leader
   node then the leader is not elected instead the newnode becomes the deputy 
 */
ClRcT 
_clGmsDefaultLeaderElectionAlgorithm (
        const ClGmsClusterNotificationBufferT buffer,
        ClGmsNodeIdT*  const        leaderNodeId,
        ClGmsNodeIdT*  const        deputyNodeId, 
        ClGmsClusterMemberT* const  memberJoinedOrLeft ,
        const ClGmsGroupChangesT    cond )
{
    /*
       Algorithm
       ----------
       Leader is elected as the member who arrived first in the ring between
       the members with the same credentials. A timer is started to wait for a
       certain period of time in which prospective leaders can arrive in to
       the ring. A timer is started to wait for all the prospective leaders to
       join the ring , the leader is computed after the timer fires and the
       member who arrived first is elected as the leader of the ring. 
     */
    ClRcT rc = CL_OK;

    clLog(INFO,GEN,NA,
            "Default leader election algorithm is invoked");

    CL_ASSERT((buffer.numberOfItems != 0x0)  && (buffer.notification != NULL));
    CL_ASSERT( (leaderNodeId != NULL) && (deputyNodeId != NULL));

    switch( cond )
    {

        case CL_GMS_MEMBER_JOINED:
        case CL_GMS_MEMBER_LEFT:
        case CL_GMS_LEADER_ELECT_API_REQUEST:
            rc = computeLeaderDeputyWithHighestCredential(
                    &buffer,
                    leaderNodeId,
                    deputyNodeId
                    );
            return rc;

            /* 
               some body called clGmsClusterLeaderElect API , the condition is
               neither JOIN nor LEAVE .
               */
        default:
            return rc;
    }

}


ClRcT
_clGmsEngineLeaderElect(
        CL_IN   const ClGmsGroupIdT         groupId ,
        CL_IN   ClGmsViewNodeT* const       node ,
        CL_IN   const ClGmsGroupChangesT    cond,
        CL_OUT  ClGmsNodeIdT*   const       leaderNodeId ,
        CL_OUT  ClGmsNodeIdT*   const       deputyNodeId )
{
    ClRcT   rc = CL_OK;
    ClGmsClusterNotificationBufferT buffer = {0};
    ClGmsDbT    *thisViewDb = NULL;
    ClGmsViewNodeT      *viewNode  = NULL;
    ClGmsClusterMemberT *currentNode = NULL;
    ClGmsNodeIdT lastLeader = 0;
    ClUint32T           i = 0;

    if ((leaderNodeId == NULL) || (deputyNodeId == NULL))
    {
        return CL_ERR_NULL_POINTER;
    }

    /* Get current view in a notification buffer */
    rc = _clGmsViewGetCurrentViewLocked(groupId, (void*)&buffer);

    if (rc != CL_OK)
    {
        clLog (CRITICAL,LEA,NA,
                "Getting current view database failed with rc [0x%x] "
                "Aborting leader election...",rc);

        return rc;
    }

    if (buffer.numberOfItems == 0)
    {
        clLogMultiline(CRITICAL,LEA,NA,
                "No nodes in the cluster view to run leader election.\n"
                "This could be because:\n"
                "Firewall is enabled on your machine which is restricting multicast messages\n"
                "Use \'iptables -F\' to disable firewall");
        rc = CL_ERR_UNSPECIFIED;
        goto done_return;
    }

    CL_ASSERT((buffer.numberOfItems != 0x0) && (buffer.notification != NULL));
    CL_ASSERT(gmsGlobalInfo.config.leaderAlgDb != NULL);

    /* Dont initialize the leaderNodeId to invalideId if it is from
     * preferredLeaderElect() request. Because here it is is already
     * initialized with the preferred leaderId 
     */
    if (cond != CL_GMS_LEADER_ELECT_API_REQUEST)
    {
        *leaderNodeId = CL_GMS_INVALID_NODE_ID;
    }
    *deputyNodeId = CL_GMS_INVALID_NODE_ID;

    /* Invoke the user algorithm with the notificationbuffer */ 
    rc = gmsGlobalInfo.config.leaderAlgDb[groupId](
            buffer,
            leaderNodeId,
            deputyNodeId,
            node == NULL ? NULL: &(node->viewMember.clusterMember),
            cond 
            );

    if ((rc != CL_OK) || (*leaderNodeId == CL_GMS_INVALID_NODE_ID))
    {
        reElect = CL_TRUE; /* trigger reelection*/
        clLog(ERROR, CLM,NA,"Leader election failed for group [%d]. rc = 0x%x",groupId,rc);
        goto done_return;
    }


    clLog(DBG,CLM,NA, "Leader election is done. Now updating the leadership status");
    /* Update current leader and "gratuitous" sending of our view of the leader to other AMFs */
    clNodeCacheLeaderUpdate(*leaderNodeId, CL_TRUE);
   
    rc  = _clGmsViewDbFind(groupId, &thisViewDb);

    if (rc != CL_OK)
    {
        goto done_return;
    }
    CL_ASSERT(thisViewDb != NULL);
    lastLeader = thisViewDb->view.leader;

    /* 
     * Unset the isCurrentLeader flag if set for any other node,
     * Set it for new leader if not already set
     */
    for ( i = 0x0 ; i < buffer.numberOfItems ; i++ )
    {
        currentNode = &(buffer.notification[i].clusterNode);

        if ((currentNode->isCurrentLeader == CL_TRUE) &&
                (currentNode->nodeId != *leaderNodeId))
        {
            /* Unset the flag for this node */
            rc = _clGmsViewFindNodePrivate(thisViewDb, currentNode->nodeId, 
                    CL_GMS_CURRENT_VIEW, &viewNode);
            if (rc != CL_OK)
            {
                clLog(CRITICAL,LEA,NA,
                        "FindNode operation for nodeId [%d] failed with rc [0x%x] "
                        "during leader election. This might lead to improper "
                        "state of cluster",currentNode->nodeId,rc);
                goto done_return;
            }

            viewNode->viewMember.clusterMember.isCurrentLeader = CL_FALSE;
        }
        else if ((currentNode->nodeId == *leaderNodeId) &&
                (currentNode->isCurrentLeader == CL_FALSE))
        {
            rc = _clGmsViewFindNodePrivate(thisViewDb, currentNode->nodeId,
                    CL_GMS_CURRENT_VIEW, &viewNode);
            if (rc != CL_OK)
            {
                clLog(CRITICAL,LEA,NA,
                        "FindNode operation for nodeId [%d] failed with rc [0x%x] "
                        "during leader election. This might lead to improper "
                        "state of cluster",currentNode->nodeId,rc);
                goto done_return;
            }

            /*
             * Now check if this elected node can really become the leader or not
             * in case we are switching too fast within the switchover soak time
             */
            if(canElectNodeAsLeader(lastLeader, &viewNode->viewMember.clusterMember))
            {
                viewNode->viewMember.clusterMember.isCurrentLeader = CL_TRUE;
            }
            else
            {
                /*
                 * But mark the node with a promotion capability in case it doesn't get reset
                 * due to the premature switchover again. Also trigger re-election
                 */
                ClTimerTimeOutT reElectTimeout = 
                { .tsSec = gmsGlobalInfo.config.bootElectionTimeout + gmsGlobalInfo.config.leaderSoakInterval,
                  .tsMilliSec = 0 
                };
                viewNode->viewMember.clusterMember.isCurrentLeader = __SC_PROMOTE_CAPABILITY_MASK;
                *leaderNodeId = CL_GMS_INVALID_NODE_ID;
                leaderElectionTimerRun(CL_TRUE, &reElectTimeout);
            }
        }

        /* If I am the leader then I need to update my global data structure */
        if ((currentNode->nodeId == gmsGlobalInfo.config.thisNodeInfo.nodeId) && 
            (*leaderNodeId == gmsGlobalInfo.config.thisNodeInfo.nodeId))
        {
            clLog(DBG, CLM, NA,
                    "I am the leader. Updating my global data structure");
            /* If I am elected as leader, update the global database.*/
            gmsGlobalInfo.config.thisNodeInfo.isCurrentLeader = CL_TRUE;
            gmsGlobalInfo.config.thisNodeInfo.isPreferredLeader = 
                currentNode->isPreferredLeader;
            gmsGlobalInfo.config.thisNodeInfo.leaderPreferenceSet = 
                currentNode->leaderPreferenceSet;
        }
    }

done_return:
    clHeapFree((void*)buffer.notification);
    return rc;
}


ClRcT
_clGmsEnginePreferredLeaderElect(
        /* Suppressing coverity warning for pass by value with below comment */
        // coverity[pass_by_value]
        CL_IN   ClGmsClusterMemberT  preferredLeaderNode,
        CL_IN   ClUint64T            contextHandle,
        CL_IN   ClInt32T             isLocalMsg)
{
    ClRcT               rc = CL_OK;
    ClRcT              _rc = CL_OK;
    ClGmsNodeIdT        leaderNode = CL_GMS_INVALID_NODE_ID;
    ClGmsNodeIdT        deputyNode = CL_GMS_INVALID_NODE_ID;
    ClGmsNodeIdT        oldLeaderNode = CL_GMS_INVALID_NODE_ID;
    ClContextInfoT     *context_info = NULL;
    ClGmsCsSectionT     contextCondVar = {0};
    ClHandleT           handle = (ClHandleT)contextHandle;
    ClGmsViewNodeT     *foundNode      = NULL;
    ClGmsDbT           *thisViewDb = NULL;
    ClCntNodeHandleT   *gmsOpaque = NULL;
    ClUint32T           j=0;

    clLog(TRACE, CLM, NA,
            "_clGmsEnginePreferredLeaderElect is invoked");

    // currently been done for default group only
    rc = _clGmsViewDbFind(0, &thisViewDb);

    if (rc != CL_OK)
    {
        return rc;
    }
    CL_ASSERT(thisViewDb != NULL);

    clGmsMutexLock(thisViewDb->viewMutex);

    /* Walk through the database and set the leaderPreferenceSet to TRUE 
     * for this node and unset it for all other nodes. */
    rc = _clGmsDbGetFirst(thisViewDb, CL_GMS_CURRENT_VIEW, &gmsOpaque,
            (void **)&foundNode);
    j = 0;
    while ((rc == CL_OK) && (j < thisViewDb->view.noOfViewMembers)
            && (foundNode != NULL))
    {
        if(foundNode->viewMember.clusterMember.nodeId == preferredLeaderNode.nodeId)
        {
            foundNode->viewMember.clusterMember.isPreferredLeader = CL_TRUE;
            foundNode->viewMember.clusterMember.leaderPreferenceSet = CL_TRUE;
        } else {
            foundNode->viewMember.clusterMember.leaderPreferenceSet = CL_FALSE;
        }
        rc = _clGmsDbGetNext(thisViewDb, CL_GMS_CURRENT_VIEW, &gmsOpaque,
                (void **)&foundNode);
        j++;
    }

    /* Pass the preferred leader nodeId through leaderNode variable.
     * This has to be used to unset the leadership preference on other nodes
     */
    oldLeaderNode = thisViewDb->view.leader;
    leaderNode = preferredLeaderNode.nodeId;

    // currently been done for default group only
    rc = _clGmsEngineLeaderElect(
            0,
            NULL,
            CL_GMS_LEADER_ELECT_API_REQUEST,
            &leaderNode,
            &deputyNode);

    if ((rc != CL_OK) && (rc != CL_ERR_ALREADY_EXIST))
    {
        clLog(ERROR,LEA,NA,
                "Leader Election API request failed with rc [0x%x]",rc);
        goto error;
    }

    clLog(NOTICE, LEA, NA,
            "Leader Election API request successful. New Leader [%d], Deputy [%d]",
            leaderNode, deputyNode);


    if ((leaderNode != oldLeaderNode) && (leaderNode != CL_GMS_INVALID_NODE_ID))
    {
        /* leader changed */
        thisViewDb->view.leader = leaderNode;
        thisViewDb->view.leadershipChanged = CL_TRUE;
        clNodeCacheLeaderUpdate(oldLeaderNode, leaderNode);
    }

    thisViewDb->view.deputy = deputyNode;

    rc =  _clGmsTrackNotify(0);
    if( rc!= CL_OK )
    {
        clLog(ERROR,CLM,NA,
                 "Cluster track notification failed during explicit leader election request. rc [0x%x]",rc);
        goto error;
    }

error:
    clGmsMutexUnlock(thisViewDb->viewMutex);
    if (isLocalMsg)
    {
        /* Checkout the context handle, put rc value and notify cond */
        _rc = clHandleCheckout(contextHandleDatabase,handle,(void*)&context_info);
        if (_rc != CL_OK)
        {
            clLog(ERROR,GEN,NA,
                    "Bad handle... Context handle checkout "
                     "failed with rc=0x%x",rc);
            return rc;
        }
        contextCondVar.cond = context_info->condVar.cond;
        contextCondVar.mutex = context_info->condVar.mutex;
        context_info->rc = rc;
        _rc = clHandleCheckin(contextHandleDatabase,handle);
        if (_rc != CL_OK)
        {
            clLog(ERROR,GEN,NA,
                    "Context handle checkin failed with rc=0x%x",rc);
            return rc;
        }

        clGmsCsLeave(&contextCondVar);
    }
    return rc;
}


/* Called from totem protocol when it receives a cluster join
 * message. Even this node join to cluster is called from 
 * totem protocol  */

static ClRcT _clGmsEngineClusterJoinWrapper(
                                      CL_IN   const ClGmsGroupIdT   groupId,
                                      CL_IN   const ClGmsNodeIdT    nodeId,
                                      CL_IN   ClGmsViewNodeT* node,
                                      CL_IN   ClBoolT reElection,
                                      CL_IN   ClBoolT *pTrackNotify)
{
    ClRcT              rc = CL_OK, add_rc = CL_OK;
    ClGmsViewT        *thisClusterView = NULL;
    ClGmsNodeIdT       currentLeader = CL_GMS_INVALID_NODE_ID;
    ClGmsNodeIdT       newLeader = CL_GMS_INVALID_NODE_ID;
    ClGmsNodeIdT       newDeputy = CL_GMS_INVALID_NODE_ID;
    ClGmsNodeIdT       currentDeputy = CL_GMS_INVALID_NODE_ID;
    ClTimerTimeOutT    timeout;

    timeout.tsSec = gmsGlobalInfo.config.bootElectionTimeout;
    timeout.tsMilliSec = 0;
    
    clLog(CL_LOG_INFO,CLM,"ENG","Cluster join invoked for node Id [%d] for group [%d]", nodeId, groupId);

    rc = _clGmsViewFindAndLock(groupId, &thisClusterView);

    if ((rc != CL_OK) || (thisClusterView == NULL))
    {
        clLog(ERROR,CLM,NA,
              "Could not get current GMS view. Join failed. rc 0x%x",rc);
        if(node) clHeapFree(node);
        return rc;
    }
    /*
     * Get from the view cache.
     */
    if(!node)
    {
        rc = clGmsViewCacheCheckAndAdd(thisClusterView->leader, nodeId, &node);
        if(rc != CL_OK || !node)
        {
            if(CL_GET_ERROR_CODE(rc) == CL_ERR_TRY_AGAIN
               &&
               bootTimeElectionDone)
            {
                if(!reElect)
                {
                    timeout.tsSec = gmsGlobalInfo.config.leaderReElectInterval;
                    reElect = CL_TRUE;
                }
                rc = CL_OK;
            }
            else 
            {
                if(node)
                    clHeapFree(node);
                goto ENG_ADD_ERROR;
            }
        }
      clLog(CL_LOG_DEBUG,CLM,NA,"Node loaded from View Cache");
    }

    if (1)
    {
        ClGmsClusterMemberT *currentNode =  &node->viewMember.clusterMember;
        clLog(CL_LOG_DEBUG,CLM,NA, "Node [%.*s:%d] credential [%d].  Details: is leader [%d] is preferred [%d] set by cli [%d] is member [%d] boot time [%llu] (%p)",currentNode->nodeName.length,currentNode->nodeName.value, currentNode->nodeId, currentNode->credential,currentNode->isCurrentLeader,currentNode->isPreferredLeader,currentNode->leaderPreferenceSet, currentNode->memberActive, currentNode->bootTimestamp,(void*) node);
    }

    if (0 == strncmp(node->viewMember.clusterMember.nodeName.value, gmsGlobalInfo.config.preferredActiveSCNodeName, node->viewMember.clusterMember.nodeName.length))
    {
        /* This node is a preferred Leader. So set ifPreferredLeader flag to TRUE */
        node->viewMember.clusterMember.isPreferredLeader = CL_TRUE;
        clLog(DBG,CLM,NA,
              "Node [%s] is marked as preferred leader in the in the config file"
              " So marking this node as preferred leader",
              gmsGlobalInfo.config.preferredActiveSCNodeName);
    }

    currentLeader = thisClusterView->leader;
    currentDeputy = thisClusterView->deputy;

    add_rc = _clGmsViewAddNodeExtended(groupId, nodeId, reElection, &node);

    if (add_rc != CL_OK)
    {
        if(CL_GET_ERROR_CODE(add_rc) != CL_ERR_ALREADY_EXIST)
        {
            clLog(ERROR,CLM,NA,
                  "Error while adding new node to GMS view. rc 0x%x",rc);
     
            goto ENG_ADD_ERROR;
        }
        else
        {
            clLog(INFO,CLM,NA,
                  "Node already exists in GMS view. Returning OK");
            add_rc = CL_OK;
            goto ENG_ADD_ERROR;
        }
    }

    /* If the group id is zero then its a cluster booting so we should wait
     *  for the timer to fire , if the timer is not fired we simply return
     *  with out invoking the algorithm.
     */
    clLogMultiline(INFO,CLM,NA, "DEBUG : boot time election done %s", (bootTimeElectionDone)?"yes!":"no!");
    if( bootTimeElectionDone != CL_TRUE)
    {
        clLogMultiline(INFO,CLM,NA,
                       "Boot time election is not done. So not invoking cluster \n"
                       "track callback for this node join. Unblocking and returning");
        if (_clGmsViewUnlock(groupId) != CL_OK)
        {
            clLog(ERROR,GEN,NA,
                  "_clGmsViewUnlock failed");
        }
        return rc;
    }
    /*
     * Trigger re-election if scheduled.
     */
    if(reElect)
    {
        if (_clGmsViewUnlock(groupId) != CL_OK)
        {
            clLog(ERROR,GEN,NA,
                  "_clGmsViewUnlock failed");
        }
        /*
         * Skip if re-election timer is already on.
         */
        if(!timerHandle)
        {
            rc = leaderElectionTimerRun(CL_TRUE, &timeout);
            if(rc == CL_OK)
            {
                clLogNotice(GEN, NA, "Leader re-election timer set to restart after [%d] secs",
                            timeout.tsSec);
            }
            else
            {
                clLogError(GEN, NA, "Leader re-election timer restart failed with error [%#x]. "
                           "The GMS cluster view could be inconsistent", rc);
            }
        }
        else
        {
            clLogNotice(GEN, NA, "Leader re-election timer is running. "
                        "Skipping leader election for node [%d], group [%d]", nodeId, groupId);
        }
        return rc;
    }

    rc = _clGmsEngineLeaderElect(groupId, node, CL_GMS_MEMBER_JOINED, &newLeader, &newDeputy);

    if (rc != CL_OK)
    {
        clLog(ERROR,CLM,NA,
              "Leader election failed during node join. rc 0x%x",rc);
        goto ENG_ADD_ERROR;
    }

    if ((newLeader != currentLeader) && (newLeader != CL_GMS_INVALID_NODE_ID))
    {
        /* leader changed */
        thisClusterView->leader = newLeader;
        thisClusterView->leadershipChanged = CL_TRUE;
        thisClusterView->deputy = newDeputy;
        clNodeCacheLeaderUpdate(currentLeader, newLeader);
    }

    thisClusterView->deputy = newDeputy;

    if(pTrackNotify)
        *pTrackNotify = CL_TRUE;

    rc =  _clGmsTrackNotify(groupId);
    if( rc!= CL_OK )
    {
        clLog(ERROR,CLM,NA,
              "Cluster track notification failed for node join [%d] rc [0x%x]",
              nodeId,rc);
        goto ENG_ADD_ERROR;
    }

    clLog (INFO,CLM,NA,
           "Node [%d] joined the cluster successfully",nodeId);

    // Moved out synch send by leader or deputy; as Sync has to be done prior to leader election.

    ENG_ADD_ERROR:
    if (_clGmsViewUnlock(groupId) != CL_OK)
    {
        clLog(ERROR,GROUPS,NA,
              "_clGmsViewUnlock failed");
    }
    clLog(TRACE,CLM,NA,
          "Unlocked the view DB. Now returning from cluster join");

    if (add_rc)
    {
        return add_rc;
    }
    return rc;
}

ClRcT _clGmsEngineClusterJoin(
                              CL_IN   const ClGmsGroupIdT   groupId,
                              CL_IN   const ClGmsNodeIdT    nodeId,
                              CL_IN   ClGmsViewNodeT* node)
{
    return _clGmsEngineClusterJoinWrapper(groupId, nodeId, node, CL_FALSE, NULL);
}

ClRcT _clGmsEngineClusterLeaveWrapper(
                                      CL_IN   ClGmsGroupIdT   groupId,
                                      CL_IN   ClCharT*        nodeIp)
{
    /* This function will find the node in the database
     * which has given IP address and then calls cluster
     * leave on this node */
    ClRcT               rc = CL_OK ;
    ClGmsDbT           *thisViewDb = NULL;
    ClGmsViewNodeT     *foundNode = NULL;
    ClCntNodeHandleT   *gmsOpaque = NULL;
    ClGmsNodeIdT        foundNodeId = 0;
    ClUint32T           j = 0;

    rc = _clGmsViewDbFind(groupId, &thisViewDb);

    if (rc != CL_OK)
        return rc;

    clGmsMutexLock(thisViewDb->viewMutex);

    rc = _clGmsDbGetFirst(thisViewDb, CL_GMS_CURRENT_VIEW, &gmsOpaque,
                          (void **)&foundNode);
    j = 0;
    while ((rc == CL_OK) && (j < thisViewDb->view.noOfViewMembers)
           && (foundNode != NULL))
    {
        if(!strcmp(nodeIp,(char*)foundNode->viewMember.clusterMember.nodeIpAddress.value))
        {
            clLog(INFO,CLM,NA,
                  "Node with IP %s is leaving the cluster\n",
                  foundNode->viewMember.clusterMember.nodeIpAddress.value);
            foundNodeId = foundNode->viewMember.clusterMember.nodeId;
            break;
        }
        rc = _clGmsDbGetNext(thisViewDb, CL_GMS_CURRENT_VIEW, &gmsOpaque,
                             (void **)&foundNode);
        j++;
    }

    clGmsMutexUnlock(thisViewDb->viewMutex);
    if (foundNodeId != 0)
    {
        return _clGmsEngineClusterLeave(groupId,foundNodeId);
    }
    else 
    {
        clLogWarning(CLM,NA, 
                     "Could not find node with IP %s in gms database",nodeIp);
        /* Returning CL_OK because this might also have caused due to
         * inference of multicast ports with other nodes running on
         * the network, or also when active node goes down, CPM gives
         * explicit cluster leave and the node is deleted before getting
         * this call from openais. */
    }
    return CL_OK;
}

ClRcT _clGmsEngineClusterLeaveExtended(
                                       CL_IN   const ClGmsGroupIdT   groupId,
                                       CL_IN   const ClGmsNodeIdT    nodeId,
                                       CL_IN   ClBoolT viewCache)
{
    ClRcT               rc = CL_OK ;
    ClGmsViewT         *thisClusterView = NULL;
    ClGmsNodeIdT        new_leader= CL_GMS_INVALID_NODE_ID;
    ClGmsNodeIdT        new_deputy= CL_GMS_INVALID_NODE_ID;

    clLog(INFO,CLM,NA,
            "Cluster leave is invoked for node ID %d\n",nodeId);
    rc = _clGmsViewFindAndLock(groupId, &thisClusterView);

    if (rc != CL_OK)
        return rc;

    rc = _clGmsViewDeleteNodeExtended(groupId, nodeId, viewCache);

    if (rc != CL_OK)
    {
        clLogError("ENGINE", "LEAVE", "Node leave for [%#x] returned with [%#x]", nodeId, rc);
        goto unlock_and_exit;
    }

    /* condition should never happen */
    CL_ASSERT(!((thisClusterView->leader == nodeId) && 
            (thisClusterView->deputy==nodeId )));

    /*
       Check whether the leaving node is a leader or deputy in the ring then
       invoke the leader election to elect the new deputy and the leader of
       the ring 
     */
    if ((nodeId == thisClusterView->leader) || 
            (nodeId == thisClusterView ->deputy))
    {
        timerRestarted = CL_FALSE;

        rc = _clGmsEngineLeaderElect(
                0x0,
                NULL,   /* FIXME: need to send the node leaving the cluster*/
                CL_GMS_MEMBER_LEFT,
                &new_leader,
                &new_deputy 
                );

        if (CL_GET_ERROR_CODE(rc) == CL_GMS_ERR_EMPTY_GROUP)
        {
            /* Nothing to do, group is empty */
            goto unlock_and_exit;
        }

        if (rc != CL_OK)
        {
            goto unlock_and_exit;
        }
        clNodeCacheLeaderUpdate(thisClusterView->leader, new_leader);
        if( nodeId == thisClusterView->leader )
        {
            thisClusterView->leader = new_leader;
            thisClusterView->leadershipChanged = CL_TRUE;
        }
        thisClusterView->deputy= new_deputy;
    }

    if (bootTimeElectionDone == CL_TRUE)
    {
        /*Track notify should be done only if leader election timer
         *has expired. Because at this time, we dont know the leader
         *and deputy leader values.
         */
        rc = _clGmsTrackNotify(groupId);
    }

    /* Delete all the group members which were there on this group */
     _clGmsRemoveAllMembersOnShutdown (nodeId);


    /*Delete the callback timer used while joining.*/
    if (timerHandle &&
        (timerRestarted == CL_FALSE) &&
        (nodeId == gmsGlobalInfo.config.thisNodeInfo.nodeId))
    {
        rc = clTimerDeleteAsync(&timerHandle);

        if(rc != CL_OK)
        {
           clLog(ERROR,CLM,NA,
                 "Initial Leader Election Soak Timer deletion Failed with [%#x] for node [%d]", rc, nodeId);
        }
    }

unlock_and_exit:

    if (_clGmsViewUnlock(groupId) != CL_OK)
    {
        clLog(ERROR,GEN,NA,
                "_clGmsViewUnlock failed");
    }

    if (rc)
        return rc;
    /* I'm leaving the cluster. Therefore, I am not the leader. The global database need to be updated*/
    if (nodeId == gmsGlobalInfo.config.thisNodeInfo.nodeId)
    {
        clLog(DBG, CLM, NA,
                    "I am not the leader. Updating my global data structure");
        gmsGlobalInfo.config.thisNodeInfo.isCurrentLeader = CL_FALSE;
    }

    clLog(INFO,CLM,NA, 
            "Cluster node [%d] left the cluster",nodeId);
    return rc;
}

ClRcT _clGmsEngineClusterLeave(
        CL_IN   const ClGmsGroupIdT   groupId,
        CL_IN   const ClGmsNodeIdT    nodeId)
{
    return _clGmsEngineClusterLeaveExtended(groupId, nodeId, CL_TRUE);
}


/* Suppressing coverity warning for pass by value with below comment */
                                // coverity[pass_by_value]
ClRcT   _clGmsEngineGroupCreate(CL_IN    ClGmsGroupNameT     groupName,
                                // coverity[pass_by_value]
                                CL_IN    ClGmsGroupParamsT   groupParams,
                                CL_IN    ClUint64T           contextHandle,
                                CL_IN    ClInt32T            isLocalMsg)
{
    ClRcT       rc = CL_OK;
    ClGmsDbT   *thisViewDb = NULL;
    ClUint32T   groupId = 0;
    time_t      t= 0x0;
    ClContextInfoT  *context_info = NULL;
    ClGmsCsSectionT  contextCondVar = {0};
    ClHandleT       handle = (ClHandleT)contextHandle;
    ClRcT   _rc = CL_OK;

    /* Take mutex for name-id db and check if the name exists, if so
       return error */
    clGmsMutexLock(gmsGlobalInfo.nameIdMutex);
    
    rc = _clGmsNameIdDbFind(&gmsGlobalInfo.groupNameIdDb,&groupName,&groupId);
    if (rc == CL_OK)
    {
        clOsalMutexUnlock(gmsGlobalInfo.nameIdMutex);
        rc = CL_ERR_ALREADY_EXIST;
        goto error_return;
    }

    clGmsMutexLock(gmsGlobalInfo.dbMutex);

    /* Create the database */
    rc = _clGmsDbCreate(gmsGlobalInfo.db,&thisViewDb);
    if (rc != CL_OK)
    {
        clGmsMutexUnlock(gmsGlobalInfo.dbMutex);
        clGmsMutexUnlock(gmsGlobalInfo.nameIdMutex);
        goto error_return;
    }
    CL_ASSERT(thisViewDb != NULL);


    /* Get the database index */
    rc = _clGmsDbIndexGet(gmsGlobalInfo.db,&thisViewDb, &groupId);
    if (rc != CL_OK)
    {
        clGmsMutexUnlock(gmsGlobalInfo.dbMutex);
        clGmsMutexUnlock(gmsGlobalInfo.nameIdMutex);
        goto error_return;
    }

    if (time(&t) < 0)
    {
        clLog (ERROR,GROUPS,NA,
                "time() system call failed. System returned error [%s]",strerror(errno));
    }

    thisViewDb->view.bootTime = (ClTimeT)(t*CL_GMS_NANO_SEC);
    thisViewDb->view.lastModifiedTime = (ClTimeT)(t*CL_GMS_NANO_SEC);
    thisViewDb->view.isActive   = CL_TRUE;
    thisViewDb->viewType        = CL_GMS_GROUP;
    thisViewDb->view.id         = groupId;
    thisViewDb->groupInfo.iocMulticastAddr = 
                                CL_IOC_MULTICAST_ADDRESS_FORM(0, groupId);
    strncpy(thisViewDb->view.name.value,groupName.value,
                            groupName.length);
    thisViewDb->view.name.length = groupName.length;

    thisViewDb->groupInfo.groupId = groupId;
    memcpy(&thisViewDb->groupInfo.groupName,&groupName,sizeof(ClGmsGroupNameT));
    memcpy(&thisViewDb->groupInfo.groupParams,&groupParams,sizeof(ClGmsGroupParamsT));
    thisViewDb->groupInfo.noOfMembers = 0;
    thisViewDb->groupInfo.setForDelete = CL_FALSE;
    thisViewDb->groupInfo.creationTimestamp = thisViewDb->view.bootTime;
    thisViewDb->groupInfo.lastChangeTimestamp = thisViewDb->view.lastModifiedTime;

    /* Update name id database */
    rc = _clGmsNameIdDbAdd(&gmsGlobalInfo.groupNameIdDb,&groupName,groupId);
    CL_ASSERT(rc == CL_OK);

    clGmsMutexUnlock(gmsGlobalInfo.dbMutex);
    clGmsMutexUnlock(gmsGlobalInfo.nameIdMutex);

error_return:
    /* See if the message was originated on the same node, if so,
       then notify the caller */
    if (isLocalMsg)
    {
        /* Checkout the context handle, put rc value and notify cond */
        _rc = clHandleCheckout(contextHandleDatabase,handle,(void*)&context_info);
        if (_rc != CL_OK)
        {
            clLog(ERROR,GEN,NA,
                    "Bad handle... Context handle checkout "
                     "failed with rc=0x%x",rc);
            return rc;
        }
        contextCondVar.cond = context_info->condVar.cond;
        contextCondVar.mutex = context_info->condVar.mutex;
        context_info->rc = rc;
        _rc = clHandleCheckin(contextHandleDatabase,handle);
        if (_rc != CL_OK)
        {
            clLog(ERROR,GEN,NA,
                    "Context handle checkin failed with rc=0x%x",rc);
            return rc;
        }

        clGmsCsLeave(&contextCondVar);
    }
    return rc;
}


ClRcT   _clGmsEngineGroupDestroy(CL_IN   ClGmsGroupIdT       groupId,
                                 /* Suppressing coverity warning for pass by value with below comment */
                                 // coverity[pass_by_value]
                                 CL_IN   ClGmsGroupNameT     groupName,
                                 CL_IN   ClUint64T           contextHandle,
                                 CL_IN   ClInt32T            isLocalMsg)
{
    ClRcT                rc = CL_OK;
    ClGmsDbT            *thisViewDb = NULL;
    ClContextInfoT      *context_info = NULL;
    ClGmsCsSectionT      contextCondVar = {0};
    ClHandleT            handle = (ClHandleT)contextHandle;
    ClRcT                _rc = CL_OK;
    ClIocMulticastAddressT  multicastAddr = 0;

    /* Take mutex for name-id db and check if the name exists, if so
       return error */
    clGmsMutexLock(gmsGlobalInfo.nameIdMutex);
    
    rc = _clGmsNameIdDbFind(&gmsGlobalInfo.groupNameIdDb,&groupName,&groupId);
    if (rc != CL_OK)
    {
        clGmsMutexUnlock(gmsGlobalInfo.nameIdMutex);
        rc = CL_ERR_DOESNT_EXIST;
        goto error_return;
    }

    clGmsMutexLock(gmsGlobalInfo.dbMutex);

    /* Get the database the database */
    rc = _clGmsViewDbFind(groupId,&thisViewDb);
    if (rc != CL_OK) /* This should not happen */
    {
        clGmsMutexUnlock(gmsGlobalInfo.dbMutex);
        clGmsMutexUnlock(gmsGlobalInfo.nameIdMutex);
        goto error_return;
    }

    if (thisViewDb->groupInfo.noOfMembers != 0)
    {
        thisViewDb->groupInfo.setForDelete = CL_TRUE;
        clGmsMutexUnlock(gmsGlobalInfo.dbMutex); 
        clGmsMutexUnlock(gmsGlobalInfo.nameIdMutex); 
        return CL_ERR_INUSE;
    }


    rc = _clGmsDbDestroy(thisViewDb);
    if (rc != CL_OK)
    {
        clGmsMutexUnlock(gmsGlobalInfo.dbMutex);
        clGmsMutexUnlock(gmsGlobalInfo.nameIdMutex);
        goto error_return;
    }


    /* Delete the entry from the name-id db */
    rc = _clGmsNameIdDbDelete(&gmsGlobalInfo.groupNameIdDb,&groupName);
    clGmsMutexUnlock(gmsGlobalInfo.dbMutex);
    clGmsMutexUnlock(gmsGlobalInfo.nameIdMutex);

error_return:
    if (isLocalMsg)
    {
        if (rc == CL_OK)
        {
            /* DeregisterAll with IOC */
            multicastAddr = CL_IOC_MULTICAST_ADDRESS_FORM(0, groupId);
            rc = clIocMulticastDeregisterAll(&multicastAddr);
            if ((rc != CL_OK) && (CL_GET_ERROR_CODE(rc) != CL_ERR_NOT_EXIST))
            {
                clLog(ERROR,GEN,NA,
                        "clIocMulticastDeregisterAll failed for groupId %d with rc 0x%x",
                         groupId,rc);
                rc = CL_GMS_ERR_IOC_DEREGISTRATION;
            }
            else 
            {
                /* Return OK even if it is CL_ERR_NOT_EXIST */
                rc = CL_OK;
            }
        }

        /* Checkout the context handle, put rc value and notify cond */
        _rc = clHandleCheckout(contextHandleDatabase,handle,(void*)&context_info);
        if (_rc != CL_OK)
        {
            clLog(ERROR,GEN,NA,
                    "Bad handle... Context handle checkout "
                     "failed with rc=0x%x",rc);
            return rc;
        }
        contextCondVar.cond = context_info->condVar.cond;
        contextCondVar.mutex = context_info->condVar.mutex;
        context_info->rc = rc;
        _rc = clHandleCheckin(contextHandleDatabase,handle);
        if (_rc != CL_OK)
        {
            clLog(ERROR,GEN,NA,
                    "Context handle checkin failed with rc=0x%x",rc);
        }

        clGmsCsLeave(&contextCondVar);
    }
    return rc;
}


ClRcT   _clGmsEngineGroupJoin(CL_IN     ClGmsGroupIdT       groupId,
                              CL_IN     ClGmsViewNodeT*     node,
                              CL_IN     ClUint64T           contextHandle,
                              CL_IN     ClInt32T            isLocalMsg)
{
    ClRcT                rc = CL_OK;
    ClGmsDbT            *thisViewDb= NULL;
    time_t               t= 0x0;
    ClContextInfoT      *context_info = NULL;
    ClGmsCsSectionT      contextCondVar = {0};
    ClHandleT            handle = (ClHandleT)contextHandle;
    ClRcT                _rc = CL_OK;


    if (node == NULL)
    {
        rc = CL_ERR_NULL_POINTER;
        goto error_return;
    }

    /* If the entire db is being locked, then we should not
     * allow other operations on the db. To enforce this
     * we will just take a lock and unlock the dbMutex.
     */
    clGmsMutexLock(gmsGlobalInfo.dbMutex);
    /* Do nothing */
    clGmsMutexUnlock(gmsGlobalInfo.dbMutex);

    rc = _clGmsViewDbFind(groupId, &thisViewDb);
    if (rc != CL_OK)
    {
        goto error_return;
    }

    CL_ASSERT(thisViewDb != NULL);

    clGmsMutexLock(thisViewDb->viewMutex);

    if (thisViewDb->groupInfo.setForDelete == CL_TRUE)
    {
        clGmsMutexUnlock(thisViewDb->viewMutex);
        rc = CL_ERR_OP_NOT_PERMITTED;
        goto error_return;
    }

    if (time(&t) < 0)
    {
        clLog (ERROR,GROUPS,NA,
                "time() system call failed. System returned error [%s]",strerror(errno));
    }

    node->viewMember.groupMember.joinTimestamp = (ClTimeT)(t*CL_GMS_NANO_SEC);

    rc = _clGmsViewAddNode(groupId, node->viewMember.groupMember.memberId, node);
    if ((rc != CL_OK) && (rc != CL_ERR_ALREADY_EXIST))
    {
        clLog (ERROR,GROUPS,NA, "Member Join failed; return error code [0x%x]!", rc);
        clGmsMutexUnlock(thisViewDb->viewMutex);
        goto error_return;
    }
    else if(rc == CL_ERR_ALREADY_EXIST)
    {
        clLog (INFO,GROUPS,NA, "Given Member already exist; returning success!");

	rc = CL_OK;
        clGmsMutexUnlock(thisViewDb->viewMutex);
        goto error_return;
    }

    thisViewDb->groupInfo.noOfMembers++;

    rc =  _clGmsTrackNotify(groupId);
        
    clGmsMutexUnlock(thisViewDb->viewMutex);
error_return:
    if (isLocalMsg)
    {
        /* Checkout the context handle, put rc value and notify cond */
        _rc = clHandleCheckout(contextHandleDatabase,handle,(void*)&context_info);
        if (_rc != CL_OK)
        {
            clLog(ERROR,GEN,NA,
                    "Bad handle... Context handle checkout "
                     "failed with rc=0x%x",_rc);
            return rc;
        }
        contextCondVar.cond = context_info->condVar.cond;
        contextCondVar.mutex = context_info->condVar.mutex;
        context_info->rc = rc;
        _rc = clHandleCheckin(contextHandleDatabase,handle);
        if (_rc != CL_OK)
        {
            clLog(ERROR,GEN,NA,
                    "Context handle checkin failed with rc=0x%x",_rc);
        }

        clGmsCsLeave(&contextCondVar);
    }

    return rc;
}


ClRcT   _clGmsEngineGroupLeave(CL_IN    ClGmsGroupIdT       groupId,
                               CL_IN    ClGmsMemberIdT      memberId,
                               CL_IN    ClUint64T           contextHandle,
                               CL_IN    ClInt32T            isLocalMsg)
{
    ClRcT                rc = CL_OK;
    ClGmsDbT            *thisViewDb= NULL;
    ClContextInfoT      *context_info = NULL;
    ClGmsCsSectionT      contextCondVar = {0};
    ClHandleT            handle = (ClHandleT)contextHandle;
    ClRcT                _rc = CL_OK;
    ClGmsViewNodeT      *foundNode = NULL;

    /* If the entire db is being locked, then we should not
     * allow other operations on the db. To enforce this
     * we will just take a lock and unlock the dbMutex.
     */
    clGmsMutexLock(gmsGlobalInfo.dbMutex);
    /* Do nothing */
    clGmsMutexUnlock(gmsGlobalInfo.dbMutex);

    rc = _clGmsViewDbFind(groupId, &thisViewDb);
    if (rc != CL_OK)
    {
        goto error_return;
    }

    CL_ASSERT(thisViewDb != NULL);

    clGmsMutexLock(thisViewDb->viewMutex);

    rc = _clGmsViewFindNodePrivate(thisViewDb, memberId, CL_GMS_CURRENT_VIEW,
                        &foundNode);
    if (rc != CL_OK)
    {
        clGmsMutexUnlock(thisViewDb->viewMutex);
        goto error_return;
    }
    CL_ASSERT(foundNode != NULL);

    rc = _clGmsViewDeleteNode(groupId, memberId);
    if (rc != CL_OK)
    {
        clGmsMutexUnlock(thisViewDb->viewMutex);
        goto error_return;
    }
    thisViewDb->groupInfo.noOfMembers--;

    rc =  _clGmsTrackNotify(groupId);
        
    clGmsMutexUnlock(thisViewDb->viewMutex);
error_return:
    if (isLocalMsg)
    {
        /* Checkout the context handle, put rc value and notify cond */
        _rc = clHandleCheckout(contextHandleDatabase,handle,(void*)&context_info);
        if (_rc != CL_OK)
        {
            clLog(ERROR,GEN,NA,
                    "Bad handle... Context handle checkout "
                     "failed with rc=0x%x",rc);
            return rc;
        }
        contextCondVar.cond = context_info->condVar.cond;
        contextCondVar.mutex = context_info->condVar.mutex;
        context_info->rc = rc;
        _rc = clHandleCheckin(contextHandleDatabase,handle);
        if (_rc != CL_OK)
        {
            clLog(ERROR,GEN,NA,
                    "Context handle checkin failed with rc=0x%x",rc);
        }

        clGmsCsLeave(&contextCondVar);
    }

    return rc;
}

ClRcT   _clGmsRemoveAllMembersOnShutdown (CL_IN  ClGmsNodeIdT   nodeId)
{
    ClRcT                 rc = CL_OK;
    ClUint32T             i = 0;
    ClUint32T             j = 0;
    ClGmsGroupIdT         groupId = 0;
    ClGmsMemberIdT        memberId[256] = {0};
    ClUint32T             membTBDel = 0;
    ClUint32T             index = 0;
    ClGmsDbT             *thisViewDb= NULL;
    ClGmsViewNodeT       *foundNode = NULL;
    ClCntNodeHandleT     *gmsOpaque = NULL;

    /* Take lock on the database */
    clGmsMutexLock(gmsGlobalInfo.dbMutex);
    /* We are assuming the index 0 will always be taken by cluster */
    for (i=1; i < gmsGlobalInfo.config.noOfGroups; i++)
    {
        if ((gmsGlobalInfo.db[i].view.isActive == CL_TRUE) &&
                (gmsGlobalInfo.db[i].viewType == CL_GMS_GROUP) &&
                (gmsGlobalInfo.db[i].groupInfo.noOfMembers > 0))
        {
            /* These 3 conditions should indicate that the group is
             * not empty and it is active.
             */
            thisViewDb = &gmsGlobalInfo.db[i];
            clGmsMutexLock(thisViewDb->viewMutex);

            groupId = thisViewDb->groupInfo.groupId;

            /* Get the list of members that have the nodeId of the node
             * being shutdown. We will later call viewDelete for all these
             * members.
             */

            rc = _clGmsDbGetFirst(thisViewDb, CL_GMS_CURRENT_VIEW, &gmsOpaque,
                    (void **)&foundNode);
            j = 0;
            membTBDel=0;
            while ((rc == CL_OK) && 
                   (j < thisViewDb->groupInfo.noOfMembers)  &&
                   (foundNode != NULL))
            {
                if (foundNode->viewMember.groupMember.memberAddress.iocPhyAddress.nodeAddress == nodeId)
                {
                    memberId[membTBDel++] = foundNode->viewMember.groupMember.memberId;
                }
                rc = _clGmsDbGetNext(thisViewDb, CL_GMS_CURRENT_VIEW, &gmsOpaque,
                        (void **)&foundNode);
                j++;
            }

            for (index = 0; index < membTBDel ; index++)
            {
                rc = _clGmsViewDeleteNode(groupId, memberId[index]);
                if (rc != CL_OK)
                {
                    clGmsMutexUnlock(thisViewDb->viewMutex);
                    goto error_return;
                }
                thisViewDb->groupInfo.noOfMembers--;
                _clGmsTrackNotify(groupId);
            }

            clGmsMutexUnlock(thisViewDb->viewMutex);
        }
    }

error_return:
    clGmsMutexUnlock(gmsGlobalInfo.dbMutex);

    return rc;
}

ClRcT   _clGmsRemoveMemberOnCompDeath(CL_IN ClGmsMemberIdT     memberId)
{
    ClRcT                 rc = CL_OK;
    ClUint32T             i = 0;
    ClUint32T             j = 0;
    ClGmsGroupIdT          groupId = 0;
    ClGmsDbT             *thisViewDb= NULL;
    ClGmsViewNodeT       *foundNode = NULL;
    ClCntNodeHandleT     *gmsOpaque = NULL;
    ClHandleT             dummyHandle = 0;

    /* Take lock on the database */
    clGmsMutexLock(gmsGlobalInfo.dbMutex);
    /* We are assuming the index 1 will always be taken by cluster */
    for (i=1; i < gmsGlobalInfo.config.noOfGroups; i++)
    {
        if ((gmsGlobalInfo.db[i].view.isActive == CL_TRUE) &&
                (gmsGlobalInfo.db[i].viewType == CL_GMS_GROUP) &&
                (gmsGlobalInfo.db[i].groupInfo.noOfMembers > 0))
        {
            /* These 3 conditions should indicate that the group is
             * not empty and it is active.
             */
            thisViewDb = &gmsGlobalInfo.db[i];
            groupId = thisViewDb->groupInfo.groupId;

            rc = _clGmsDbGetFirst(thisViewDb, CL_GMS_CURRENT_VIEW, &gmsOpaque,
                    (void **)&foundNode);
            j = 0;
            while ((rc == CL_OK) && (j < thisViewDb->groupInfo.noOfMembers) 
                    && (foundNode != NULL))
            {
                if (foundNode->viewMember.groupMember.memberId == memberId)
                {
                    clGmsMutexUnlock(gmsGlobalInfo.dbMutex);

                    rc = _clGmsEngineGroupLeave(groupId,memberId,dummyHandle,0);
                    if (rc != CL_OK)
                    {
                        clLogMultiline(ERROR,GEN,NA,
                                "Removing group member with memberId = %d"
                                 " in groupId = %d, on comp death failed with "
                                 "rc = 0x%x\n",memberId,groupId,rc);
                    }
                    else {
                        clLog(INFO,GROUPS,NA,
                                "Removing group memeber with memberId = %d"
                                " on groupId = %d on comp death is successful",
                                memberId,groupId);
                    }
                    goto done_return;
                }
                rc = _clGmsDbGetNext(thisViewDb, CL_GMS_CURRENT_VIEW, &gmsOpaque,
                                            (void **)&foundNode);
                j++;
            }
        }
    }

    clGmsMutexUnlock(gmsGlobalInfo.dbMutex);

done_return:

    return rc;
}

ClRcT  
_clGmsEngineGroupInfoSync(ClGmsGroupSyncNotificationT *syncNotification)
{
    ClRcT       rc = CL_OK;
    ClUint32T   i = 0;
    ClGmsGroupInfoT *thisGroupInfo = NULL;
    ClGmsDbT   *thisViewDb = NULL;
    ClUint32T   groupId = 0;
    #if 1
    ClUint32T   tempgroupId = 0;
    #endif
    time_t      t= 0x0;
    ClGmsViewNodeT  *thisNode = NULL;

    if (readyToServeGroups == CL_TRUE)
    {
        clLog(INFO,GEN,NA,
                "This sync message is not intended for this node\n");
        return rc;
    }

    clGmsMutexLock(gmsGlobalInfo.dbMutex);
    clGmsMutexLock(gmsGlobalInfo.nameIdMutex);
    clLog(DBG,GEN,NA,
            "Processing the sync message.");
    /* Create the groups as per the number of groups */
    for (i = 0; i < syncNotification->noOfGroups; i++)
    {
        thisGroupInfo = &syncNotification->groupInfoList[i];
        CL_ASSERT(thisGroupInfo != NULL);

         #if 1
	// added conditional check to ensure the groupId 0 is not set for synch after node-id synch update is called
	if (thisGroupInfo->groupId == 0)
	{
	    clLog(DBG,GEN,NA, "Found group Id 0 being called for _clGmsEngineGroupInfoSync for create group; skipping and continuing.");
	    continue;
	}
        #endif

        groupId = thisGroupInfo->groupId;
        CL_ASSERT(groupId != 0);

        rc = _clGmsDbCreateOnIndex(groupId,gmsGlobalInfo.db,&thisViewDb);
        CL_ASSERT(rc == CL_OK);

        thisViewDb->view.bootTime = (ClTimeT)(t*CL_GMS_NANO_SEC);
        thisViewDb->view.lastModifiedTime = (ClTimeT)(t*CL_GMS_NANO_SEC);
        thisViewDb->view.isActive   = CL_TRUE;
        thisViewDb->viewType        = CL_GMS_GROUP;
        thisViewDb->view.id         = groupId;
        strncpy(thisViewDb->view.name.value,thisGroupInfo->groupName.value,
                thisGroupInfo->groupName.length);
        thisViewDb->view.name.length = thisGroupInfo->groupName.length;

        /* copy the group Info into thisViewDb */
        memcpy(&thisViewDb->groupInfo,thisGroupInfo,sizeof(ClGmsGroupInfoT));

        #if 1
	// added check so that duplicate information is not added
	rc = _clGmsNameIdDbFind(&gmsGlobalInfo.groupNameIdDb, &thisGroupInfo->groupName, &tempgroupId);
        if ( CL_GET_ERROR_CODE( rc ) == CL_ERR_NOT_EXIST )
	{	
    	    clLog(DBG,GEN,NA, "Name-Id entry is not present for group [%s] Id [%d]", (char *) &thisGroupInfo->groupName.value, groupId);
	    /* Create an entry into name-id pair db */
            rc = _clGmsNameIdDbAdd(&gmsGlobalInfo.groupNameIdDb, &thisGroupInfo->groupName,groupId);
            CL_ASSERT(rc == CL_OK);
	}
	else
	{
    	    clLog(DBG,GEN,NA, "Name-Id entry is present for group [%s] Id [%d]; return [0x%x] skipping!", (char *) &thisGroupInfo->groupName.value, groupId, rc);
	    rc = CL_OK;
	}
	#endif
    }
    clGmsMutexUnlock(gmsGlobalInfo.nameIdMutex);

    /* Add the group members */
    for (i = 0; i < syncNotification->noOfMembers; i++)
    {
	#if 0
        // added conditional check to ensure the groupId 0 is not set for synch after node-id synch update is called
        if (thisGroupInfo->groupId == 0)
        {
            clLog(DBG,GEN,NA, "Found group Id 0 being called for _clGmsEngineGroupInfoSync for tracking; skipping and continuing.");
            continue;
        }
	#endif

        thisNode = (ClGmsViewNodeT*)clHeapAllocate(sizeof(ClGmsViewNodeT));
        CL_ASSERT(rc == CL_OK);
        memcpy(thisNode,&syncNotification->groupMemberList[i], sizeof(ClGmsViewNodeT));
        CL_ASSERT(thisNode != NULL);

        groupId = thisNode->viewMember.groupData.groupId;
        clLog(DBG,GEN,NA, "Sync update for group Id [%d]", groupId);

        rc = _clGmsViewDbFind(groupId, &thisViewDb);

        CL_ASSERT((rc == CL_OK) && (thisViewDb != NULL));

        thisNode->viewMember.groupMember.joinTimestamp = (ClTimeT)(t*CL_GMS_NANO_SEC);
        rc = _clGmsViewAddNode(groupId, thisNode->viewMember.groupMember.memberId, thisNode);
    	clLog(DBG,GEN,NA, "added member ID [%d] to group [%d]", thisNode->viewMember.groupMember.memberId, groupId);
        CL_ASSERT((rc == CL_OK) || (rc == CL_ERR_ALREADY_EXIST));
    }

    /* Do a track notify to make sure that JOIN_LEFT_VIEW is emptied */
    for (i = 0; i < syncNotification->noOfGroups; i++)
    {
        thisGroupInfo = &syncNotification->groupInfoList[i];
        CL_ASSERT(thisGroupInfo != NULL);

	#if 0
        // added conditional check to ensure the groupId 0 is not set for synch after node-id synch update is called
        if (thisGroupInfo->groupId == 0)
        {
            clLog(DBG,GEN,NA, "Found group Id 0 being called for _clGmsEngineGroupInfoSync for tracking; skipping and continuing.");
            continue;
        }
	#endif

        groupId = thisGroupInfo->groupId;
        CL_ASSERT(groupId != 0);

        rc =  _clGmsTrackNotify(groupId);
        if (rc != CL_OK)
        {
            clLog(ERROR,GEN,NA,
                    "Track notification for groupId = %d failed with rc = 0x%x\n",
                     groupId, rc);
        }
    }
    clLog(DBG,GEN,NA,
            "Sync message processing is done.");

    // Once synch in new node is compalted it will set for GMS service
    readyToServeGroups = CL_TRUE;    
   
    clGmsMutexUnlock(gmsGlobalInfo.dbMutex);
    return rc;
}

ClRcT   _clGmsEngineMcastMessageHandler(ClGmsGroupMemberT *groupMember,
                                        ClGmsGroupInfoT   *groupData,
                                        ClUint32T          userDataSize,
                                        ClPtrT             data)
{
    ClRcT                 rc = CL_OK;
    ClGmsDbT             *thisViewDb= NULL;
    ClGmsViewNodeT       *foundNode = NULL;
    ClCntNodeHandleT     *gmsOpaque = NULL;
    ClGmsGroupMcastCallbackDataT callback = {0};
    ClIocAddressT         address = {{0}};

    /* Take lock on the database */
    clGmsMutexLock(gmsGlobalInfo.dbMutex);
    /* We are assuming the index 0 will always be taken by cluster */
    thisViewDb = &gmsGlobalInfo.db[groupData->groupId];
    clGmsMutexLock(thisViewDb->viewMutex);
    if (thisViewDb->groupInfo.noOfMembers > 0)
    {
        rc = _clGmsDbGetFirst(thisViewDb, CL_GMS_CURRENT_VIEW, &gmsOpaque,
                (void **)&foundNode);
        while ((rc == CL_OK) && 
                (foundNode != NULL))
        {
            address = foundNode->viewMember.groupMember.memberAddress;
            if (address.iocPhyAddress.nodeAddress == gmsGlobalInfo.nodeAddr.iocPhyAddress.nodeAddress)
            {

                memset(&callback, 0, sizeof(callback));
                callback.gmsHandle = foundNode->viewMember.groupMember.handle;
                callback.groupId = groupData->groupId;
                callback.memberId = groupMember->memberId;
                callback.dataSize = userDataSize;
                callback.data = data;

                rc = cl_gms_group_mcast_callback(address,&callback);
                if (rc != CL_OK)
                {
                    clLogError("","","Mcast callback RMD failed with rc 0x%x",rc);
                    clGmsMutexUnlock(thisViewDb->viewMutex);
                    goto error_exit;
                }
            }

            /* Send the callback rmd */
            rc = _clGmsDbGetNext(thisViewDb, CL_GMS_CURRENT_VIEW, &gmsOpaque,
                    (void **)&foundNode);
        }

        clGmsMutexUnlock(thisViewDb->viewMutex);
    }

error_exit:
    clGmsMutexUnlock(gmsGlobalInfo.dbMutex);

    return rc;
}
