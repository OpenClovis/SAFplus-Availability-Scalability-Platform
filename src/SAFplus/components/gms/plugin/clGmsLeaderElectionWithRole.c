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
 * File        : clGmsLeaderElectionWithRole.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file implements a user specific leader election algorithm.
 * After compiling this file, a .so file is created under $ASP_LIB directory
 * which can be used to direct GMS to run this leader election algorithm
 * overriding the default leader election algorithm.
 * 
 * To instruct GMS to lead this algorithm, please add following lines into
 * clGmsConfig.xml file.
 *
 * <group>
 *      <id>0</id>
 *      <pluginPath>libClGmsLeaderElectionWithRolePlugin.so</pluginPath>
 * </group>
 *
 * Note that the directory containing .so file should be in LD_LIBRARY_PATH
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <clLogApi.h>
#include <clClmApi.h>
#include <clDebugApi.h>

static ClRcT 
findLeaderAndDeputy(
                                       ClGmsClusterNotificationBufferT *buffer,
                                       ClGmsNodeIdT                    *leaderNodeId,
                                       ClGmsNodeIdT                    *deputyNodeId )
{

    ClRcT                rc = CL_OK;
    ClGmsClusterMemberT *eligibleNodes[16] = {0}; /* Only 2 SC nodes are possible in an ASP
                                                   * cluster. But considering 16 node array
                                                   * just to avoid segfaults in faulty 
                                                   * configurations */
    ClGmsClusterMemberT *currentNode = NULL;
    ClGmsClusterMemberT *node1 = NULL;
    ClGmsClusterMemberT *node2 = NULL;
    ClGmsClusterMemberT *otherNode = NULL;
    ClUint32T            index=0x0;
    ClUint32T            noOfEligibleNodes = 0;
    ClCharT             *env=NULL;
    ClUint32T            thisNodeId = 0;

    *leaderNodeId = *deputyNodeId = CL_GMS_INVALID_NODE_ID;

    CL_ASSERT( buffer != NULL && buffer->numberOfItems != 0 && buffer->notification != NULL);
    CL_ASSERT( leaderNodeId != NULL && deputyNodeId != NULL );

    thisNodeId  =  clIocLocalAddressGet();

    /* Assume the first 2 nodes are the leader and the deputy and rearrange as
     *  we progress to the end of the buffer */
    for (index = 0x0; index < buffer->numberOfItems; index++ )
    {
        currentNode = &(buffer->notification[index].clusterNode);
        if (currentNode->credential != CL_GMS_INELIGIBLE_CREDENTIALS)
        {
            eligibleNodes[noOfEligibleNodes++] = currentNode;
        }
    }

    if(noOfEligibleNodes == 0)
    {
        clLogMultiline(WARN,NA,NA,
                       "Could not elect any leader from the current cluster view.\n"
                       "Possibly no system controller is running");
        return rc;
    }

    CL_ASSERT(noOfEligibleNodes <= 2);

    /* Among the eligible nodes, check if there is already an existing leader
     */
    if (noOfEligibleNodes == 1) 
    {
        /* We have only 1 eligible node in the cluster.
         * So simply elect this guy as leader and return
         */
        *leaderNodeId = eligibleNodes[0]->nodeId;
        clLog(NOTICE,NA,NA,
              "Only one SC node [%d] running in the cluster. Electing itself as leader",
              *leaderNodeId);
        goto election_done;
    }

    /* We have 2 eligible nodes. So we need to look into
     * several conditions */
    node1 = eligibleNodes[0];
    node2 = eligibleNodes[1];

    otherNode = node2;
    if(node2->nodeId == thisNodeId)
    {
        otherNode = node1;
    }

    clLog(DBG,NA,NA,
          "2 SC nodes found %d and %d",node1->nodeId, node2->nodeId);

    if(node1->isCurrentLeader)
    {
        if(node2->isCurrentLeader)
        {
            clLogMultiline(NOTICE,NA,NA,
                    "Node [%d] and Node [%d] both are set to be existing leaders"
                    "This could be a split brain scenario. Selecting the node with"
                    "highest node ID as the leader",node1->nodeId, node2->nodeId);
            *leaderNodeId = CL_MAX(node1->nodeId, node2->nodeId);
            *deputyNodeId = CL_MIN(node1->nodeId, node2->nodeId);
        } else {
            clLog(NOTICE,NA,NA,
                    "Node [%d] is already the leader. Not re-electing the leader",
                    node1->nodeId);
            *leaderNodeId = node1->nodeId;
            *deputyNodeId = node2->nodeId;
        } 
        goto election_done;
    }
    else if(node2->isCurrentLeader)
    {
        clLog(NOTICE,NA,NA,
                "Node [%d] is already the leader. Not re-electing the leader",
                node2->nodeId);
        *leaderNodeId = node2->nodeId;
        *deputyNodeId = node1->nodeId;
        goto election_done;
    }
    else
    {
        /* Didnt find an existing leader. This means we are in cluster
         * startup phase. So we need to consider ROLE or ROLE_ACTIVE
         * env variables */
        clLog(NOTICE,NA,NA,
              "Did not find an existing leader node. Looking for ROLE env variable");
        if( (env=getenv("ROLE")) )
        {
            /*
             * Double check for this node id. so that ROLE doesn't show up on payloads
             */
            CL_ASSERT(thisNodeId == node1->nodeId || thisNodeId == node2->nodeId);
            if(!strncasecmp(env, "active", 6))
            {
                clLog(NOTICE,NA,NA, "This node has ROLE set to ACTIVE");
                *leaderNodeId = thisNodeId;
                *deputyNodeId = otherNode->nodeId;
                goto election_done;
            } 
            if(!strncasecmp(env, "standby", 7))
            {
                clLog(NOTICE,NA,NA, "This node has ROLE set to STANDBY");
                *deputyNodeId = thisNodeId;
                *leaderNodeId = otherNode->nodeId;
                goto election_done;
            } 
            clLogMultiline(CRITICAL,LEA,NA,
                           "Improper value [%s] for ROLE environment variable. "
                           "Expected values are \"active\" or \"standby\" (Case insensitive)",env);
            CL_ASSERT(0);
        }
        else  
        {
            clLog(NOTICE,NA,NA,
                  "ROLE env not set. Looking for CL_LEADER_NODE_ID env variable");
            env = getenv("CL_LEADER_NODE_ID");
            if (env == NULL)
            {
                clLogMultiline(CRITICAL,LEA,NA,
                               "CL_LEADER_NODE_ID or ROLE environment variable is not set. Cannot "
                               "elect any leader in the cluster");
                CL_ASSERT(0);
            } 
            else 
            {
                *leaderNodeId = atoi(env);
                CL_ASSERT( *leaderNodeId == node1->nodeId || *leaderNodeId == node2->nodeId );
                clLog(NOTICE,LEA,NA,
                      "CL_LEADER_NODE_ID is set to %d", *leaderNodeId);
                *deputyNodeId = (*leaderNodeId == node1->nodeId)? node2->nodeId: node1->nodeId;
            }
        }
    }

election_done:
    /* We must have found a leader by now */
    CL_ASSERT(*leaderNodeId != CL_GMS_INVALID_NODE_ID);

    clLog(NOTICE,LEA,NA,
          "Leader election complete. Leader [%d], Deputy [%d]",
          *leaderNodeId, *deputyNodeId);
    return rc;
}

ClRcT 
LeaderElectionAlgorithm (
        ClGmsClusterNotificationBufferT buffer,
        ClGmsNodeIdT            *leaderNodeId,
        ClGmsNodeIdT            *deputyNodeId, 
        ClGmsClusterMemberT     *memberJoinedOrLeft,
        ClGmsGroupChangesT      cond,
        ClBoolT                 splitBrain)
{

    ClRcT rc = CL_OK;

    CL_ASSERT( buffer.numberOfItems != 0x0  && buffer.notification );

    *leaderNodeId = *deputyNodeId = CL_GMS_INVALID_NODE_ID;
    
    switch( cond )
    {
        case CL_GMS_MEMBER_JOINED:
        case CL_GMS_MEMBER_LEFT:
            return findLeaderAndDeputy(
                    &buffer,
                    leaderNodeId,
                    deputyNodeId
                    );
        default:
            clLogError (LEA, NA, "Invalid condition (neither join nor left encountered");
            break;
    }
    return rc;
}
