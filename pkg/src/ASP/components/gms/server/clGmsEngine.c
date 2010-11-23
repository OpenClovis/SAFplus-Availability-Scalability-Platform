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
#include <clGmsDb.h>
#include <clGmsMsg.h>
#include <clGmsApiHandler.h>
#include <clGmsRmdServer.h>

# define CL_MAX_CREDENTIALS     (~0U)

static      ClTimerHandleT  timerHandle = NULL;
ClBoolT     bootTimeElectionDone = CL_FALSE;


/*
   timerCallback
   -------------
   Get the current view from the database .
   Invoke the leader election algorithm .
   mark that boottime election is done. 
 */
static ClRcT 
timerCallback( void *arg ){

    ClRcT rc = CL_OK;
    ClGmsNodeIdT leaderNodeId = CL_GMS_INVALID_NODE_ID;
    ClGmsNodeIdT deputyNodeId = CL_GMS_INVALID_NODE_ID;
    ClGmsViewT   *view = NULL;
    
    clLog(INFO,LEA,NA,
            "Running boot time leader election after 5 secs of GMS startup");

    rc = _clGmsViewFindAndLock( 0x0 , &view );
    CL_ASSERT( (rc == CL_OK) && (view != NULL));

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

    if (view->leader != leaderNodeId ){
        view->leader = leaderNodeId;
        view->leadershipChanged = CL_TRUE;
        view->deputy = deputyNodeId;
        rc = clEoMyEoObjectSet ( gmsGlobalInfo.gmsEoObject );
        CL_ASSERT( rc == CL_OK );
        if (_clGmsTrackNotify ( 0x0 ) != CL_OK)
        {
            clLog(ERROR,LEA,NA,
                    "_clGmsTrackNotify failed in leader election timer callback");
        }
    }

    bootTimeElectionDone = CL_TRUE;

    // comemnted to ensure during synch only the value is set
    //readyToServeGroups = CL_TRUE;
    // if there is no leader in the group or there is only 1 node in the cluster group, set to true 
    clLog(INFO,LEA,NA, "leader [%d] & noOfViewMemebers [%d] is found!", view->leader, view->noOfViewMembers);
    if (view->leader || view->noOfViewMembers == 1)
    {
    	clLog(INFO,LEA,NA,
            "There is no leader in the default group; hence setting in current node readyToServeGroups as True");
	readyToServeGroups = CL_TRUE;
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

ClRcT
_clGmsEngineStart()
{
    ClRcT       rc = CL_OK;
    ClTimerTimeOutT timeOut = { gmsGlobalInfo.config.bootElectionTimeout , 0x0 };
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
    clLog(INFO,GEN,NA,
            "Starting boot time election timer for [%d] secs",timeOut.tsSec);
    rc = clTimerCreateAndStart(
            timeOut,
            CL_TIMER_ONE_SHOT,
            CL_TIMER_TASK_CONTEXT,
            timerCallback,
            NULL,
            &timerHandle 
            );

    if(rc != CL_OK)
    {
        clLogMultiline(EMER,GEN,NA,
                "Boot time leader election timer creation failed. rc [0x%x]\nBooting Aborted...",rc);
        exit(0);
    }


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
    
    return rc;
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
            eligibleNodes[i]->isPreferredLeader)
        {
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

    clLog(INFO,LEA,NA,
            "Results of leader election: Leader [%d], Deputy [%d]",
            *leaderNodeId,*deputyNodeId);
    clLogNotice("---","---",
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
        clLog(ERROR, CLM,NA,"Leader election failed. rc = 0x%x", rc);
        goto done_return;
    }


    clLog(DBG,CLM,NA,
            "Leader election is done. Now updating the leadership status");
    rc  = _clGmsViewDbFind(groupId, &thisViewDb);

    if (rc != CL_OK)
    {
        goto done_return;
    }
    CL_ASSERT(thisViewDb != NULL);

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

            viewNode->viewMember.clusterMember.isCurrentLeader = CL_TRUE;
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
ClRcT _clGmsEngineClusterJoin(
                CL_IN   const ClGmsGroupIdT   groupId,
                CL_IN   const ClGmsNodeIdT    nodeId,
                CL_IN   ClGmsViewNodeT* const node)
{
    ClRcT              rc = CL_OK, add_rc = CL_OK;
    ClGmsViewT        *thisClusterView = NULL;
    ClGmsNodeIdT       currentLeader = CL_GMS_INVALID_NODE_ID;
    ClGmsNodeIdT       newLeader = CL_GMS_INVALID_NODE_ID;
    ClGmsNodeIdT       newDeputy = CL_GMS_INVALID_NODE_ID;
    ClGmsNodeIdT       currentDeputy = CL_GMS_INVALID_NODE_ID;
    ClUint32T          len1 = 0;
    ClUint32T          len2 = 0;

    clLog(INFO,CLM,NA,
            "GMS engine cluster join is invoked for node Id [%d] for group [%d]", nodeId, groupId);

    /* See this node was the preferred leader. If so then set the tag to TRUE */
    len1 = strlen(gmsGlobalInfo.config.preferredActiveSCNodeName);
    len2 = node->viewMember.clusterMember.nodeName.length;
    if ((len1 == len2) &&
        (!strncmp(node->viewMember.clusterMember.nodeName.value, 
                gmsGlobalInfo.config.preferredActiveSCNodeName,len1)))
    {
        /* This node is a preferred Leader. So set ifPreferredLeader flag to TRUE */
        node->viewMember.clusterMember.isPreferredLeader = CL_TRUE;
        clLog(DBG,CLM,NA,
                "Node [%s] is marked as preferred leader in the in the config file"
                " So marking this node as preferred leader",
                gmsGlobalInfo.config.preferredActiveSCNodeName);
    }

    rc = _clGmsViewFindAndLock(groupId, &thisClusterView);

    if ((rc != CL_OK) || (thisClusterView == NULL))
    {
        clLog(ERROR,CLM,NA,
                "Could not get current GMS view. Join failed. rc 0x%x",rc);
        return rc;
    }

    currentLeader = thisClusterView->leader;
    currentDeputy = thisClusterView->deputy;

    add_rc = _clGmsViewAddNode(groupId, nodeId, node);

    if ((add_rc != CL_OK) && (add_rc != CL_GMS_RC(CL_ERR_ALREADY_EXIST)))
    {
        clLog(ERROR,CLM,NA,
                "Error while adding new node to GMS view. rc 0x%x",rc);
        goto ENG_ADD_ERROR;
    }
    else if ( add_rc == CL_GMS_RC(CL_ERR_ALREADY_EXIST))
    {
        clLog(INFO,CLM,NA,
                "Node already exists in GMS view. Returning OK");
        add_rc = CL_OK;
        goto ENG_ADD_ERROR;
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
    }

    thisClusterView->deputy = newDeputy;
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
    else {
        clLog(INFO,CLM,NA, 
                "Could not find node with IP %s gms databse",nodeIp);
        /* Returning CL_OK because this might also have caused due to
         * inference of multicast ports with other nodes running on
         * the network, or also when active node goes down, CPM gives
         * explicit cluster leave and the node is deleted before getting
         * this call from openais. */
        return CL_OK;
    }
}

ClRcT _clGmsEngineClusterLeave(
        CL_IN   const ClGmsGroupIdT   groupId,
        CL_IN   const ClGmsNodeIdT    nodeId)
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

    rc = _clGmsViewDeleteNode(groupId, nodeId);

    if (rc != CL_OK)
        goto unlock_and_exit;

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

        if( nodeId == thisClusterView->leader ){
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


unlock_and_exit:

    if (_clGmsViewUnlock(groupId) != CL_OK)
    {
        clLog(ERROR,GEN,NA,
                "_clGmsViewUnlock failed");
    }

    if (rc)
        return rc;
    /*Delete the callback timer used while joining.*/
    if ((timerHandle != NULL) && 
            (nodeId == gmsGlobalInfo.config.thisNodeInfo.nodeId))
    {
        rc = clTimerDelete ( &timerHandle );

        if(rc != CL_OK)
        {
           clLog(ERROR,CLM,NA,
                     "Initial Leader Election Soak Timer deletion Failed");
        }
        timerHandle = NULL;
    }

    clLog(INFO,CLM,NA, 
            "Cluster node [%d] left the cluster",nodeId);
    return rc;
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
                    "Context handle checkin failed with rc=0x%x",rc);
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
    ClUint32T   tempgroupId = 0;
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

	if (thisGroupInfo->groupId == 0)
	{
	    clLog(DBG,GEN,NA, "Found group Id 0 being called for _clGmsEngineGroupInfoSync for create group; skipping and continuing.");
	    continue;
	}

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

	#if 0
        /* Create an entry into name-id pair db */
        rc = _clGmsNameIdDbAdd(&gmsGlobalInfo.groupNameIdDb,
                               &thisGroupInfo->groupName,groupId);
	#endif
	
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

        CL_ASSERT(rc == CL_OK);
    }
    clGmsMutexUnlock(gmsGlobalInfo.nameIdMutex);

    /* Add the group members */
    for (i = 0; i < syncNotification->noOfMembers; i++)
    {
        thisNode = (ClGmsViewNodeT*)clHeapAllocate(sizeof(ClGmsViewNodeT));
        CL_ASSERT(rc == CL_OK);
        memcpy(thisNode,&syncNotification->groupMemberList[i], sizeof(ClGmsViewNodeT));
        CL_ASSERT(thisNode != NULL);

        groupId = thisNode->viewMember.groupData.groupId;

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
      
    // VV on 26 Oct 2010; condition added to ensure sync will be executed
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
