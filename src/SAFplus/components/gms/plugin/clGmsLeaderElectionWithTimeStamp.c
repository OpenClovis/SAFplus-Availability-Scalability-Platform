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
 * File        : clGmsUserPlugin.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file implements a users proprietary leader election algorithm .
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <clClmApi.h>
#include <clDebugApi.h>

static ClRcT 
computeLeaderDeputyWithLowestTimestamp(
        ClGmsClusterNotificationBufferT *buffer,
        ClGmsNodeIdT                    *leaderNodeId,
        ClGmsNodeIdT                    *deputyNodeId 
        ){

    ClRcT                rc = CL_OK;
    ClGmsClusterMemberT *leader=NULL , *deputy = NULL, *currentNode = NULL;
    ClUint32T            i=0x0;

    *leaderNodeId = *deputyNodeId = CL_GMS_INVALID_NODE_ID;

    CL_ASSERT( buffer->numberOfItems !=0x0 && buffer->notification && buffer );


    
    /* Assume the first 2 nodes are the leader and the deputy and rearrange as
     *  we progress to the end of the buffer */
    for ( i = 0x0 ; i < buffer->numberOfItems ; i++ ){

        currentNode = &(buffer->notification[i].clusterNode);
        if( currentNode->credential != 
                CL_GMS_INELIGIBLE_CREDENTIALS
          ){
            if(!leader ){
                leader = currentNode;
                continue;
            }
            deputy = currentNode;
            break;
        }
    }

    if(!leader ){
              printf ("\nInvalid RING Configuration detected .......");
        return rc;
    }

    if(!deputy ){
        *leaderNodeId = leader->nodeId;
         return rc;
    }


    currentNode = NULL;
    if((deputy -> credential > leader -> credential) ||
            ((leader -> credential == deputy -> credential ) &&
             ( deputy ->bootTimestamp < leader ->bootTimestamp ))){

        currentNode = deputy;
        deputy      = leader;
        leader      = currentNode;
    }


  
    for ( i++ ; i< buffer->numberOfItems ; i++ ){
        currentNode = &(buffer->notification[i].clusterNode);
        if( currentNode -> credential != CL_GMS_INELIGIBLE_CREDENTIALS){
            if( currentNode -> credential > leader -> credential ){
                deputy = leader;
                leader = currentNode;
            }
            else if ( currentNode -> credential < leader -> credential ){
                if( currentNode -> credential > deputy -> credential ){
                    deputy = currentNode;
                }
            }
            else{
                if( currentNode -> bootTimestamp < leader->bootTimestamp ){
                    deputy = leader;
                    leader = currentNode;
                }
            }
        }
    }

    CL_ASSERT( leader && deputy );

    *leaderNodeId = leader->nodeId;
    *deputyNodeId = deputy->nodeId;
    return rc;
}

/*FIXME: This file as of now implements the same default algorithm used by the
 * GMS Infrastructure. Need to write a different realistic algorithm.
 */
ClRcT 
LeaderElectionAlgorithm (
        ClGmsClusterNotificationBufferT buffer,
        ClGmsNodeIdT            *leaderNodeId,
        ClGmsNodeIdT            *deputyNodeId, 
        ClGmsClusterMemberT     *memberJoinedOrLeft ,
        ClGmsGroupChangesT      cond,
        ClBoolT                 splitBrain)
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

    CL_ASSERT( buffer.numberOfItems != 0x0  && buffer.notification );

    *leaderNodeId = *deputyNodeId = CL_GMS_INVALID_NODE_ID;
    
    switch( cond ){

        case CL_GMS_MEMBER_JOINED:
        case CL_GMS_MEMBER_LEFT:
            rc = computeLeaderDeputyWithLowestTimestamp(
                    &buffer,
                    leaderNodeId,
                    deputyNodeId
                    );
            return rc;

        default:
              printf ("\nInvalid condition (neither join nor left encountered");
            return rc;
    }

}
