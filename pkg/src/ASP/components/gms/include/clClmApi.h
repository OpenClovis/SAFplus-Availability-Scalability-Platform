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
 * File        : clGmsApi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This client function implements the SA Forum compliant cluster membership
 * management.
 *
 * Component Manager (CPM) uses these functions to manage its node in the cluster.
 * This service assumes that there is only one cluster in the system.
 * 
 * An instance of this service (instantiated by the clGmsInitialize()
 * function and identified by the gmsHandle) can view and manage the cluster. 
 * If a node joins/leaves/kicked out of the cluster then all the other nodes 
 * in the cluster and its group components are updated with the current status.
 *  
 *********************************************************************************/

/*********************************************************************************/
/****************************** Group Management Service**************************/
/*********************************************************************************/
/*                                                                               */ 
/*  clGmsClusterJoin                                                             */
/*  clGmsClusterJoinAsync                                                        */
/*  clGmsClusterLeave                                                            */
/*  clGmsClusterLeaveAsync                                                       */
/*  clGmsClusterLeaderElect                                                      */
/*  clGmsClusterMemberEject                                                      */
/*                                                                               */
/**********************************************************************************/

/**
 *  \file
 *  \brief Header file of SA Forum compliant Group Membership Service.
 *  \ingroup gms_apis
 */

/**
 *  \addtogroup gms_apis
 *  \{
 */




#ifndef _CL_GMS_API_H
#define _CL_GMS_API_H

# ifdef __cplusplus
extern "C" {
# endif

#include <clCommon.h>
#include <clClmTmsCommon.h>

/*=========================================================================
 * Callback Functions:
 *=======================================================================*/

/**
 ************************************
 *  \brief Callback for indicating that member was expelled from the cluster.
 *
 *  \par Header File:
 *  clGmsApi.h
 *
 *  \param reasonCode (in) Indicates the reason for expelling the node from
 *  the cluster. The reasonCodes defined as of now are -
 *  \arg CL_GMS_MEMBER_EJECT_REASON_UNKNOWN = 0
 *  \arg CL_GMS_MEMBER_EJECT_REASON_API_REQUEST =1
 *
 *  \retval  none
 *
 *  \par Description:
 *  This ClGmsClusterMemberEjectCallback callback function is getting called when
 *  a cluster member is ejected by an administrative operation.
 *
 *  \par Library File:
 *  ClGms
 *
 *  \sa none
 *
 */
typedef void (*ClGmsClusterMemberEjectCallbackT) (
         CL_IN ClGmsMemberEjectReasonT   reasonCode);

/**
 *  This structure contains the cluster managing callbacks provided at the
 *  joining time by the member. The structure contains the eject callback
 *  which is called when the member is ejected from the cluster, the callback
 *  is invoked after the member is ejected and the reason for ejection is
 *  passed as argument to the callback.
 */
typedef struct {

/**
 * Pointer to the Eject Callback funtion.
 */ 
    	ClGmsClusterMemberEjectCallbackT clGmsMemberEjectCallback;

} ClGmsClusterManageCallbacksT;

/**
 *  Signature of the leader election algorithm used in the GMS engine . 
 *  Leader election algorithm is implemented as plugin (dynamically loadable shared
 *  object). Leader election algorithm is invoked upon any changes in the
 *  cluster ( member joining or member leaving ) or upon the invocation of the
 *  clGmsClusterLeaderElect function.  Algorithm is given information regarding
 *  The current view of the cluster and the condition in which the
 *  algorithm is invoked. Condition can be
 *  \c CL_GMS_MEMBER_JOIN or \c CL_GMS_MEMBER_LEFT and the pointer to the node is
 *  passed to the algorithm. 
 */ 
typedef ClRcT (*ClGmsLeaderElectionAlgorithmT)(
        ClGmsClusterNotificationBufferT buffer,
        ClGmsNodeIdT            *leaderNodeId,
        ClGmsNodeIdT            *deputyNodeId, 
        ClGmsClusterMemberT     *memberJoinedOrLeft,
        ClGmsGroupChangesT       cond 
        );


/*=========================================================================
 * API Functions:
 *=======================================================================*/

/*
 ************************************
 *  \brief Joins the cluster as a member. 
 * 
 *  \par Header File:
 *  clGmsApi.h
 *
 *  \param gmsHandle The handle, obtained through the clGmsInitialize() function,
 *   designating this particular initialization of the Group Membership Service.
 *  \param clusterManageCallbacks Callbacks for managing the cluster. 
 *  \param credentials This is an integer value specifying the
 *  leadership credibility of the node. Larger the value higher is the
 *  possibility of the node becoming a leader. Member with creditials
 *  \c CL_GMS_INELIGIBLE_CREDENTIALS cannot participate in the leader election.
 *  \param timeout If the cluster join is not completed within this time, 
 *  then the join request is timed out. 
 *  \param nodeId Node ID of the member that will join the cluster. 
 *  \param nodeName Name of the node that will join the cluster. 
 *
 *
 *  \retval CL_ERR_INVALID_HANDLE If the handle passed to the function is not valid.
 *  The handle passed should have been obtained from the clGmsInitialize() function.
 *  \retval CL_ERR_TIMEOUT If the join request timed out.
 *  \retval CL_ERR_INVALID_PARAMETER If any of the input parameters are invalid.
 *  \retval CL_ERR_NULL_POINTER If either of the parameters \e clusterManageCallbacks
 *  or \e nodeName are NULL.
 *  \retval CL_ERR_TRY_AGAIN: GMS server is not ready to process the request.

 *
 *  \par Description:
 *  This function is used to include a node into the the cluster as a member.
 *  Success  or failure is reported  via return value  
 *  Members who have registered for tracking, get notified by tracking callback.
 *
 *  \par Library File:
 *  ClGms
 *
 *  \sa clGmsClusterLeave()
 *
 */
extern ClRcT clGmsClusterJoin(
        CL_IN const ClGmsHandleT                        gmsHandle,
        CL_IN const ClGmsClusterManageCallbacksT* const clusterManageCallbacks,
        CL_IN const ClGmsLeadershipCredentialsT         credentials,
        CL_IN const ClTimeT                             timeout,
        CL_IN const ClGmsNodeIdT                        nodeId,
        CL_IN const ClNameT*                      const nodeName);


/*
 ************************************
 *  \brief Asynchronously joins the cluster as a member. 
 * 
 *  \par Header File:
 *  clGmsApi.h
 *
 *  \param gmsHandle The handle, obtained through the clGmsInitialize() function,
 *   designating this particular initialization of the Group Membership Service 
 *  \param clusterManageCallbacks Callbacks for managing the cluster. 
 *  \param credentials This is an integer value specifying the
 *  leadership credibility of the node. Larger the value higher is the
 *  possibility of the node becoming a leader. Member with creditials
 *  \c CL_GMS_INELIGIBLE_CREDENTIALS cannot participate in the leader election.
 *  \param timeout If the cluster join is not completed within this time, 
 *  then the join request is timed out. 
 *  \param nodeId Node ID of the member that will join the cluster. 
 *  \param nodeName Name of the node that will join the cluster.
 *
 *
 *  \retval CL_ERR_INVALID_HANDLE If the handle passed to the function is not valid.
 *  The handle passed should have been obtained from the clGmsInitialize() function.
 *  \retval CL_GMS_ERR_JOIN_DENIED If the node is already part of the cluster.
 *  \retval CL_ERR_INVALID_PARAMETER If any of the input parameters are invalid.
 *  \retval CL_ERR_NULL_POINTER If either of the parameters \e clusterManageCallbacks
 *  or \e nodeName are NULL.
 *
 *  \par Description:
 *  This function is used to asynchronously include a node into the cluster as a member.
 *  Success is reported via return value. 
 *  Members that have registered for tracking get notified by tracking
 *  callback.
 *
 *  \par Library File:
 *  ClGms
 * 
 *  \sa clGmsClusterJoin()
 *
 */
extern ClRcT clGmsClusterJoinAsync(
        CL_IN const ClGmsHandleT                         gmsHandle,
        CL_IN const ClGmsClusterManageCallbacksT*  const clusterManageCallbacks,
        CL_IN const ClGmsLeadershipCredentialsT          credentials,
        CL_IN const ClGmsNodeIdT                         nodeId,
        CL_IN const ClNameT*                       const nodeName);


/*
 ************************************
 *  \brief Leaves the cluster.  
 * 
 *  \par Header File:
 *  clGmsApi.h
 *
 *  \param gmsHandle The handle, obtained through the clGmsInitialize() function,
 *   designating this particular initialization of the Group Membership Service 
 *  \param timeout If the cluster leave operation is not completed within this time, 
 *  then the leave request is timed out.
 *  \param nodeId Node ID of the member that is leaving the cluster.  
 *
 *  \retval CL_ERR_INVALID_HANDLE If the handle passed to the function is not valid.
 *  The handle passed should have been obtained from the clGmsInitialize() function..
 *  \retval CL_ERR_INVALID_PARAMETER If any of the input parameters are invalid.
 *
 *  \par Description:
 *  This function is used to make a node leave the cluster. Once the node leaves the cluster 
 *  and the groups/components will be expelled and
 *  a reason for expulsion will be returned through their
 *  callback functions.
 *
 *  \par Library File:
 *  ClGms
 * 
 *  \sa clGmsClusterJoin()
 *
 */
extern ClRcT clGmsClusterLeave(
        CL_IN ClGmsHandleT                      gmsHandle,
        CL_IN ClTimeT                           timeout,
        CL_IN ClGmsNodeIdT                      nodeId);


/*
 ************************************
 *  \brief Leaves the cluster asynchronously.  
 * 
 *  \par Header File:
 *  clGmsApi.h
 *
 *  \param gmsHandle The handle, obtained through the clGmsInitialize() function,
 *   designating this particular initialization of the Group Membership Service
 *  \param nodeId  Node ID of the member to be ejected out.
 *
 *  \retval CL_ERR_INVALID_HANDLE If the handle passed to the function is not valid.
 *  The handle passed should have been obtained from the clGmsInitialize() function.
 *  \retval CL_ERR_INVALID_PARAMETER If any of the input parameters are invalid.
 *
 *  \par Description:
 *  This function is used to make a node leave the cluster asynchronously. Once the node leaves the cluster 
 *  and the groups/components will be expelled and
 *  a reason for expulsion will be returned through their
 *  callback functions.
 *
 *  \par Library File:
 *  ClGms
 * 
 *  \sa clGmsClusterLeave() 
 *
 */
extern ClRcT clGmsClusterLeaveAsync(
        CL_IN ClGmsHandleT                      gmsHandle,
        CL_IN ClGmsNodeIdT                      nodeId);


/**
 ************************************
 *  \brief Initiate leader election synchronously. 
 * 
 *  \par Header File:
 *  clGmsApi.h
 *
 *  \param gmsHandle The handle, obtained through the clGmsInitialize() function,
 *   designating this particular initialization of the Group Membership Service
 *
 *  \retval CL_ERR_INVALID_HANDLE If the handle passed to the function is not valid.
 *  The handle passed should have been obtained from the clGmsInitialize() function.
 *
 *  \par Description:
 *  This function is used to initiate leader election synchronously. The elected leader 
 *  is announced via the tracking callback. Typically it is invoked when a node leaves or
 *  joins, or on any event which would alter the leadership of the
 *  cluster. The algorithm will be run by the GMS server engine, and a leader and
 *  deputy leader are elected.
 *  Here the API runs synchronously means that the election is done as part of the 
 *  API processing itself. However the results are given through a cluster track callback.
 *
 *  \par Library File:
 *  ClGms
 *
 */
extern ClRcT clGmsClusterLeaderElect(
        CL_IN       ClGmsHandleT                      gmsHandle,
        CL_IN       ClGmsNodeIdT                      preferredLeader,
        CL_INOUT    ClGmsNodeIdT                     *leader,
        CL_INOUT    ClGmsNodeIdT                     *deputy,
        CL_INOUT    ClBoolT                          *leadershipChanged);



/**
 ************************************
 *  \brief Forcibly removes a member from the cluster. 
 * 
 *  \par Header File:
 *  clGmsApi.h
 *
 *  \param gmsHandle The handle, obtained through the clGmsInitialize() function,
 *   designating this particular initialization of the Group Membership Service
 *  \param nodeId Node ID of the member to be ejected out.
 *  \param reason Reason for ejecting the member out of the cluster.
 *   A member can be ejected from the cluster either upon request, 
 *  or for an unknown reason. \e reasonCode can have 2 values.
 *  \arg CL_GMS_MEMBER_EJECT_REASON_UNKNOWN = 0
 *  \arg CL_GMS_MEMBER_EJECT_REASON_API_REQUEST =1.
 *
 *  \retval CL_ERR_INVALID_HANDLE If the handle passed to the function is not valid.
 *  The handle passed should have been obtained from the clGmsInitialize() function.
 *  \retval CL_ERR_INVALID_PARAMETER If the node ID is not a valid node ID or if the
 *  reason is not valid.
 * 
 * 
 *  \par Description:
 *  This function is used to remove a member forcibly from the cluster. All the
 *  process groups or group members if any on the node are expelled from their
 *  respective process groups and a reason is given for ejecting them out. 
 *  The tracking members of the cluster are notified through the tracking
 *  callback.
 *
 *  \par Library File:
 *  ClGms
 * 
 *  \sa clGmsClusterLeaveAsync(), clGmsClusterLeaderElect()
 *
 */
extern ClRcT clGmsClusterMemberEject(
        CL_IN ClGmsHandleT                      gmsHandle,
        CL_IN ClGmsNodeIdT                      nodeId,
        CL_IN ClGmsMemberEjectReasonT           reason);


#ifdef  __cplusplus
}
#endif

#endif  /* _CL_GMS_CLUSTER_MANAGE_API_H_ */

/** \} */


