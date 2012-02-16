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
 * File        : clClmTmsCommon.h
 ******************************************************************************/

/*******************************************************************************
 * Description :
 *
 * This is the viewer API for the Group Membership Service (GMS).  It allows
 * query and asynhronous tracking of GMS groups.  It does not provide for
 * managing groups (creating, deleteing, joining, leaving).  For that purpose
 * separate additional API components are provided.
 *
 * There are two types of GMS groups covered by this service:
 *
 * - Cluster membership service -- A special type of group membership, where
 *   the members are the [compuing] nodes (processors) in the cluster.
 *
 * - Generalized component (or process) groups -- these are groups of
 *   arbitrary software components, running on the various nodes in the
 *   cluster.  These components are commonly referred as group members. 
 *
 * This API is provided by the GMS client library that can be linked to
 * the application (directly or as part of the EO client environment).
 *
 * This API provides a superset of the SA Forum Cluster Membership (CLM)
 * API, as specified in its B.01.01 version.  A separate, SA Forum compliant
 * API is provided also, as a separate header file supported by a separate
 * set of API functions.
 *
 *
 ******************************************************************************/


/******************************************************************************/
/****************************** GMS APIs **************************************/
/******************************************************************************/
/*                                                                            */
/* clGmsInitialize                                                            */
/* clGmsFinalize                                                              */
/* clGmsClusterTrack                                                          */
/* clGmsClusterTrackStop                                                      */
/* clGmsClusterMemberGet                                                      */
/* clGmsClusterMemberGetAsync                                                 */
/* clGmsGroupTrack                                                            */
/* clGmsGroupTrackStop                                                        */
/* clGmsGetGroupInfo                                                          */
/* clGmsGroupsInfoListGet                                                     */
/*                                                                            */
/******************************************************************************/

/**
 *  \file
 *  \brief Header file of Group Membership Service APIs
 *  \ingroup gms_apis
 */

/**
 *  \addtogroup gms_apis
 *  \{
 */




#ifndef _CL_CLM_TMS_COMMON_H
#define _CL_CLM_TMS_COMMON_H

# ifdef __cplusplus
extern "C" {
# endif

#include <clCommon.h>
#include <clIocApi.h>

/*******************************************************************************
 *   MACROS
 ******************************************************************************/

/**
 * Maximum length of the address value
 */
#define CL_GMS_MAX_ADDRESS_LENGTH 64

/**
 * Indicates the local node.
 */
#define CL_GMS_LOCAL_NODE_ID    0XFFFFFFFF

/**
 * Invalid svc handle
 */
#define CL_GMS_INVALID_HANDLE ((ClGmsHandleT)0)

/**
 * Indicates the invalid value of the node Id
 */
#define CL_GMS_INVALID_NODE_ID 0xffffffff

/**
 * The credential value which will not be considered during leader election.
 */
#define CL_GMS_INELIGIBLE_CREDENTIALS 0x0

/**
 * Invalid group ID
 */
#define CL_GMS_INVALID_GROUP_ID -1

/*******************************************************************************
 *   TYPE DEFINITIONS
 ******************************************************************************/
/**
 * Handle for using the GMS API. A handle of this type is assigned during the 
 * initialization of the Group Membership Service, and it designates this 
 * particular initialization. It must be passed as first parameter for all 
 * operations pertaining to the GMS library.
 */
typedef ClHandleT ClGmsHandleT;


/**
 * Node ID -- Unique and consistent identifier of a node.
 */
typedef ClUint32T ClGmsNodeIdT;


/**
 * Credentials for leader election.  Only members with the highest value in the
 * group will be considered as candidates for leadership.
 */
typedef ClUint32T   ClGmsLeadershipCredentialsT;


/**
 *  This type defines the family to which the address of the node
 *  belongs.
 */
typedef enum ClGmsNodeAddressFamilyT {
    /**
     *   CL_GMS_AF_INET stands for IPv4 address.
     */ 
    CL_GMS_AF_INET= 1,

    /**
     *   CL_GMS_AF_INET6 stands for IPv6 address.
     */     
    CL_GMS_AF_INET6 =2 
} ClGmsNodeAddressFamilyT;


/**
 *  Flags for tracking request flag.
 */
typedef enum ClGmsTrackFlagsT {
    /**
     * Returns current view.
     */
    CL_GMS_TRACK_CURRENT        = 0x01, 

    /**
     * To subscribe  for  view notifications that
     * includes current view and the recent change.
     */
    CL_GMS_TRACK_CHANGES        = 0x02, 

    /**
     * To subscribe for delta notifications.
     */
    CL_GMS_TRACK_CHANGES_ONLY   = 0x04  
} ClGmsTrackFlagsT;


/**
 * Enumerator and structure for the node status notification for tracking
 * nodes. Each time a node joines or leaves or reconfigured.
 */
typedef enum ClGmsClusterChangesT {
    /**
     * No change occured on the node since the last view.
     */
    CL_GMS_NODE_NO_CHANGE       = 1,    

    /**
     * Node has joined since last view. 
     */
    CL_GMS_NODE_JOINED          = 2,    

    /**
     * Node has left since last view.
     */
    CL_GMS_NODE_LEFT            = 3,    

    /**
     * Node hes been re-configured since last view. 
     */
    CL_GMS_NODE_RECONFIGURED    = 4     
} ClGmsClusterChangesT;


/**
 * IP Address of the node can be of IPv4 or IPv6.
 */
typedef struct ClGmsNodeAddressT {
    /**
     * Family to which the address of the node belongs. 
     * IP Address of the node can be of IPV4 or IPV6.
     */
    ClGmsNodeAddressFamilyT family;

    /**
     * Length of the IP Address of the node.
     */ 
    ClUint16T length;

    /**
     * value array holds actual value of IP address
     */
    ClUint8T  value[CL_GMS_MAX_ADDRESS_LENGTH];
} ClGmsNodeAddressT;


/**
 * This structure describes one member (or node) of the cluster.
 */
typedef struct ClGmsClusterMemberT {
    /**
     * Unique ID of node.
     */
    ClGmsNodeIdT                nodeId      __attribute__((__aligned__(8)));

    /**
     * Physical IOC address of node.
     */
    ClIocAddressT               nodeAddress     __attribute__((__aligned__(8)));

    /**
     *  Node IP Address.
     */ 
    ClGmsNodeAddressT           nodeIpAddress       __attribute__((__aligned__(8)));

    /**
     * Textual name of node.
     */
    ClNameT                     nodeName        __attribute__((__aligned__(8)));

    /**
     * This is \c TRUE if the node is a member of the cluster
     * For tracking nodes it is not set.
     */
    ClBoolT                     memberActive        __attribute__((__aligned__(8)));

    /**
     * The time at which GMS was started on the node.
     */
    ClTimeT                     bootTimestamp        __attribute__((__aligned__(8)));

    /**
     * The view number when the node joined. 
     */
    ClUint64T                   initialViewNumber        __attribute__((__aligned__(8)));

    /**
     *  This is an integer value specifying the
     *  leadership credibility of the node. Larger the value higher is the
     *  possibility of the node becoming a leader. Member with creditials
     *  \c CL_GMS_INELIGIBLE_CREDENTIALS cannot participate in the leader election
     * . 
     */ 
    ClGmsLeadershipCredentialsT credential        __attribute__((__aligned__(8)));

    /**
     * Indicates if this node is the current leader of the cluster or not
     */
    ClBoolT                     isCurrentLeader        __attribute__((__aligned__(8)));

    /**
     * Indicates if this node is the preferred leader for the cluster. 
     */
    ClBoolT                     isPreferredLeader      __attribute__((__aligned__(8)));

    /**
     * Indicates that the leadership preference is set through CLI and not 
     * through config file. This provides higher preference during leader election.
     */
    ClBoolT                     leaderPreferenceSet          __attribute__((__aligned__(8)));

    /**
     *  Version information of the GMS software running on the node, information is
     *  sent to the other peers in the cluster while joining the cluster . If
     *  there is a version mismatch the node is not allowed to join the Cluster
     */
    ClVersionT gmsVersion        __attribute__((__aligned__(8)));

} ClGmsClusterMemberT        __attribute__((__aligned__(8)));


/**
 * Buffer containing the list of nodes that forms the 
 * current view of cluster.
 */
typedef struct ClGmsClusterNotificationT {
    /**
     * Node profile as described in the datatype ClGmsClusterMemberT.
     */
    ClGmsClusterMemberT   clusterNode        __attribute__((__aligned__(8)));

    /**
     * Describes the change in the cluster view since last notification.
     */
    ClGmsClusterChangesT  clusterChange;

} ClGmsClusterNotificationT        __attribute__((__aligned__(8)));


/**
 * Buffer to convey the view: the list of nodes and their status.
 */
typedef struct ClGmsClusterNotificationBufferT {
    /**
     * Current view number. 
     */
    ClUint64T                   viewNumber        __attribute__((__aligned__(8)));

    /**
     * Length of notification array. 
     */
    ClUint32T                   numberOfItems;  

    /**
     * Array of nodes.
     */
    ClGmsClusterNotificationT  *notification;   

    /**
     * Node ID of current leader. 
     */
    ClGmsNodeIdT                leader;         

    /**
     * Node marked as deputy.
     */
    ClGmsNodeIdT                deputy;         

    /**
     * To check whether the leader has changed since the last view.
     */
    ClBoolT                     leadershipChanged; 

} ClGmsClusterNotificationBufferT        __attribute__((__aligned__(8)));


/**
 * System-wide unique name of the group.
 */
typedef ClNameT ClGmsGroupNameT;        


/**
 * System-wide unique ID of the group.
 */
typedef ClUint32T ClGmsGroupIdT;        


/**
 * Group-unique name of the member.
 */
typedef ClNameT ClGmsMemberNameT;       


/**
 * Group-unique ID of a member.
 */
typedef ClUint32T ClGmsMemberIdT;       


/**
 * Parameters for group  provided during group creation. Currently
 * no paramters are provided. isIocGroup parameter specifies if the group
 * needs to create IOC multicast address.
 * Please note that non-ioc groups are not supported in release 3.0. So
 * this is for future usage.
 */
typedef struct ClGmsGroupParamsT {
    /* Specifies if the group needs to create IOC multicast address
     */
    ClBoolT     isIocGroup;

    /* for future implementations
     */
    char unused [256];
} ClGmsGroupParamsT;
                                                                                                                             

/**
 * Structure containing attributes of a group member
 */
typedef struct ClGmsGroupMemberT {
    /**
     * SVC handle of this group member
     */
    ClGmsHandleT                handle        __attribute__((__aligned__(8)));
    /**
     * Group-unique ID of the member.
     */
    ClGmsMemberIdT              memberId        __attribute__((__aligned__(8)));

    /**
     * IOC Address of the group member application
     */
    ClIocAddressT               memberAddress        __attribute__((__aligned__(8)));

    /**
     * Textual name of the member.
     */
    ClGmsMemberNameT            memberName        __attribute__((__aligned__(8)));

    /**
     * True if the node is a member of group.
     */
    ClBoolT         	        memberActive        __attribute__((__aligned__(8)));

    /**
     * The instant at which the member joined the group.
     */
    ClTimeT                     joinTimestamp        __attribute__((__aligned__(8)));

    /**
     * The view number of the group at the time the member joined.
     */
    ClUint64T                   initialViewNumber        __attribute__((__aligned__(8)));

    /**
     * Credentials for being the leader. The higher the credential, 
     * larger is the possibility of the node being elected as leader.
     */
    ClGmsLeadershipCredentialsT credential        __attribute__((__aligned__(8)));

} ClGmsGroupMemberT        __attribute__((__aligned__(8)));


/**
 * Enumerator and structure of member status notification.
 */
typedef enum ClGmsGroupChangesT {
    /**
     * Member unchanged since last view.
     */
    CL_GMS_MEMBER_NO_CHANGE     = 1,    

    /**
     * Member joined since last view.
     */
    CL_GMS_MEMBER_JOINED        = 2,    

    /**
     * Member has left since last view.
     */
    CL_GMS_MEMBER_LEFT          = 3,    

    /**
     * Member reconfigured since last view.
     */
    CL_GMS_MEMBER_RECONFIGURED  = 4,

    /**
     * Leadership election through API
     */
    CL_GMS_LEADER_ELECT_API_REQUEST  = 5     
} ClGmsGroupChangesT;


/**
 * Buffer containing the list of group members in the requested view
 */
typedef struct ClGmsGroupNotificationT {
    /**
     * Information on the member.
     */
    ClGmsGroupMemberT           groupMember        __attribute__((__aligned__(8)));

    /**
     * Indicates the kind of change in the group membership since last notification.
     */
    ClGmsGroupChangesT          groupChange; 

} ClGmsGroupNotificationT;


/**
 * Buffer to convey the view: the list of group member attributes
 */
typedef struct ClGmsGroupNotificationBufferT {

    /**
     * Current view number.
     */
    ClUint64T                   viewNumber        __attribute__((__aligned__(8)));

    /**
     * Length of notification array.
     */
    ClUint32T                   numberOfItems;  

    /**
     * Array of members.
     */
    ClGmsGroupNotificationT    *notification;   

    /**
     * Member ID of leader.
     */
    ClGmsMemberIdT              leader;         

    /**
     * Member marked as deputy.
     */
    ClGmsMemberIdT              deputy;         

    /**
     * To check whether leader has changed since the last notification
     */
    ClBoolT                     leadershipChanged; 

} ClGmsGroupNotificationBufferT        __attribute__((__aligned__(8)));


/**
 * Structure used to hold the metadata of a group 
 */
typedef struct ClGmsGroupInfoT {
    /**
     * Name of the group
     */
    ClGmsGroupNameT     groupName;

    /**
     * Group Id
     */
    ClGmsGroupIdT       groupId;

    /**
     * Desired group parameters
     */
    ClGmsGroupParamsT   groupParams;

    /**
     * Number of members in the group
     */
    ClUint32T           noOfMembers;

    /**
     * No more joins are allowed
     */
    ClBoolT             setForDelete;

    /**
     * IOC multicast address created by GMS
     */
    ClIocMulticastAddressT iocMulticastAddr;

    /**
     * Time at which group was created.
     */
    ClTimeT                 creationTimestamp;

    /**
     * Time at which the last view changed.
     */
    ClTimeT                 lastChangeTimestamp;

}  ClGmsGroupInfoT;


/**
 * Structure used to pass the meta data on all the existing groups.
 */
typedef struct ClGmsGroupInfoListT {
    /**
     * Holds the value of number of groups
     */
    ClUint32T           noOfGroups;

    /**
     * Array of ClGmsGroupInfoT data
     */
    ClGmsGroupInfoT     *groupInfoList;
} ClGmsGroupInfoListT;


/**
 *  Name of the Function to be defined in the Leader Election algorithm plugin. 
 */
#define LEADER_ELECTION_ALGORITHM "LeaderElectionAlgorithm" 

/* Default boot election timeout */
#define CL_GMS_DEFAULT_BOOT_ELECTION_TIMEOUT 5

/**
 * Reason codes for ejecting the user from cluster/group 
 */
typedef enum {
    /**
     * Reason for the eject not known
     */
    CL_GMS_MEMBER_EJECT_REASON_UNKNOWN = 0,
    /**
     * The member has been ejected due to an explicit
     * invocation of clGmsClusterMemberEject() API
     */
    CL_GMS_MEMBER_EJECT_REASON_API_REQUEST = 1
} ClGmsMemberEjectReasonT;

/**
 * The selection object fd provided by GMS client on 
 * which the user can poll for any callbacks as per SAF symantics.
 */
ClRcT 
clGmsSelectionObjectGet (
        /**
         * GMS Handle obtained from clGmsInitialize
         */
		ClGmsHandleT clmHandle,
        /**
         * Address of selection object element which will be
         * filled by gms client
         */
		ClSelectionObjectT *pSelectionObject
        );

/**
 * Dispatch API as per SAF symantics for dispatching the pending callbacks.
 */
ClRcT 
clGmsDispatch (
        /**
         * GMS client handle obtained from clGmsInitialize
         */
        ClGmsHandleT clmHandle,
        /**
         * Dispatch flags as per SAF symantics
         */
        ClDispatchFlagsT dispatchFlags
        );

/*
 * Funtion called by CPM when the event component comes up
 */
ClRcT clGmsCompUpNotify (
        ClUint32T   compId);

/******************************************************************************
 * Callback Functions:
 *****************************************************************************/
/**
 ************************************
 *  \brief  Callback for tracking cluster view changes.
 *
 *  \par Header File:
 *  clClmTmsCommon.h
 *
 *  \param notificationBuffer (in) Buffer containing the node information of the cluster
 *  \param numberOfMembers (in) Represent the number of nodes' information present
 *  in the notification buffer.
 *  \param rc (in) Return code from the server
 *
 *  \retval  none
 *
 *  \par Description:
 *  This ClGmsClusterTrackCallbackT callback function is invoked in following 2 cases:
 *  \arg If the user is registered for cluster track changes with CL_TRACK_CHANGES or
 *  CL_TRACK_CHANGES_ONLY track flags, then the callback is invoked for every change
 *  in the cluster.
 *  \arg If the user has invoked clGmsClusterTrack API with CL_TRACK_CURRENT flag and the
 *  notification buffer is NULL, then the response is served through an invocation
 *  of this callback.
 *
 *  \par Library File:
 *  ClGms
 *
 *  \sa clGmsInitialize(), clGmsClusterTrack()
 *
 */
typedef void (*ClGmsClusterTrackCallbackT) (
        CL_IN const ClGmsClusterNotificationBufferT *notificationBuffer,
        CL_IN ClUint32T             numberOfMembers,
        CL_IN ClRcT                 rc);            


/**
 ************************************
 *  \brief  Callback for asynchronous cluster member query.
 *
 *  \par Header File:
 *  clClmTmsCommon.h
 *
 *  \param invocation (in) Callback correlator
 *  \param clusterMember (in) Buffer containing the node information
 *  \param rc (in) Return code from the server
 *
 *  \retval  none
 *
 *  \par Description:
 *  This ClGmsClusterMemberGetCallbackT callback function is invoked when an async
 *  request was made to get the given node information by using clGmsClusterMemberGetAsync()
 *  API. The requested member node information is provided in the clusterMember
 *  parameter.
 *
 *  \par Library File:
 *  ClGms
 *
 *  \sa clGmsClusterMemberGet(), clGmsClusterMemberGetAsync()
 *
 */
typedef void (*ClGmsClusterMemberGetCallbackT) (
        CL_IN ClInvocationT         invocation,     
        CL_IN const ClGmsClusterMemberT *clusterMember, 
        CL_IN ClRcT                 rc);            


/**
 ************************************
 *  \brief  Callback for tracking group view changes.
 *
 *  \par Header File:
 *  clClmTmsCommon.h
 *
 *  \param groupId (in) Group ID of the group for which the track request is being served.
 *  \param notificationBuffer (in) Buffer containing the member information of the group
 *  \param numberOfMembers (in) Represent the number of members' information present
 *  in the notification buffer.
 *  \param rc (in) Return code from the server
 *
 *  \retval  none
 *
 *  \par Description:
 *  This ClGmsGroupTrackCallbackT callback function is invoked in following 2 cases:
 *  \arg If the user is registered for group track changes with CL_TRACK_CHANGES or
 *  CL_TRACK_CHANGES_ONLY track flags, then the callback is invoked for every change
 *  in the group.
 *  \arg If the user has invoked clGmsGroupTrack API with CL_TRACK_CURRENT flag and the
 *  notification buffer is NULL, then the response is served through an invocation
 *  of this callback.
 *
 *  \par Library File:
 *  ClGms
 *
 *  \sa clGmsInitialize(), clGmsGroupTrack()
 *
 */
typedef void (*ClGmsGroupTrackCallbackT) (
        CL_IN ClGmsGroupIdT 		groupId,        
        CL_IN const ClGmsGroupNotificationBufferT *notificationBuffer,
        CL_IN ClUint32T             numberOfMembers,
        CL_IN ClRcT                 rc);


/**
 ************************************
 *  \brief  Callback for asynchronous group member query.
 *
 *  \par Header File:
 *  clClmTmsCommon.h
 *
 *  \param invocation (in) Callback correlator
 *  \param groupMember (in) Buffer containing the member information
 *  \param rc (in) Return code from the server
 *
 *  \retval  none
 *
 *  \par Description:
 *  This ClGmsGroupMemberGetCallbackT callback function is invoked when an async
 *  request was made to get the given member information by using clGmsGroupMemberGetAsync()
 *  API. The requested member node information is provided in the clusterMember
 *  parameter.
 *
 *  \par Library File:
 *  ClGms
 *
 *  \sa clGmsClusterMemberGet(), clGmsClusterMemberGetAsync()
 *
 */
typedef void (*ClGmsGroupMemberGetCallbackT) (
        CL_IN ClInvocationT         invocation,     
        CL_IN const ClGmsGroupMemberT *groupMember, 
        CL_IN ClRcT                 rc);


/**
 * This callback structure is provided to the GMS library during Initialization.
 */
typedef struct ClGmsCallbacksT {
    /**
     *  This callback is called when the response comes from the server side for
     *  the async request made by the \e clGmsClusterMemberGetAsync call. The callback
     *  is called with the invocation ID and the information of the member
     *  requested	
     */
    ClGmsClusterMemberGetCallbackT clGmsClusterMemberGetCallback;

    /**
     *  This callback is invoked to notify the registered user about any change in the  
     *  cluster configuration.
     *  The registration for the change notification with the server is done by calling
     *  clGmsClusterTrack() API with appropriate \e trackFlags. The value of trackFlags
     *  can be either \c CL_GMS_TRACK_CHANGES or \c CL_GMS_TRACK_CHANGES_ONLY.
     *  The notification buffer provided in the callback contains the updated member 
     *  information of the cluster.
     */
    ClGmsClusterTrackCallbackT     clGmsClusterTrackCallback;

    /**
     *  This callback is invoked to notify the registered user about any change in the 
     *  membership of a given group.
     *  The registration for change notification with the server is done by calling
     *  clGmsGroupTrack() API with appropriate the \e trackFlags. The value of trackFlags
     *  should be either \c CL_GMS_TRACK_CHANGES or \c CL_GMS_TRACK_CHANGES_ONLY.
     *  The notification buffer provided in the callback contains the updated member 
     *  information for the given group.
     */
    ClGmsGroupTrackCallbackT       clGmsGroupTrackCallback;

    /**
     *  This callback is called when the response comes from the server side for
     *  the async request made by the \e clGmsGroupMemberGetAsync call. The callback
     *  is called with the invocation ID and the information of the member
     *  requested	
     */
    ClGmsGroupMemberGetCallbackT   clGmsGroupMemberGetCallback;
} ClGmsCallbacksT;


/******************************************************************************
 * API FUNCTION DECLARATIONS:
 *****************************************************************************/

/**
 ************************************
 *  \brief Initializes the GMS library and registers the callback functions.
 * 
 *  \par Header File:
 *  clClmTmsCommon.h  
 *
 *  \param gmsHandle (out) GMS service handle created by the library.
 *  This is used in subsequent use of the library in this session.
 *
 *  \param gmsCallbacks Pointer to the array of callback functions provided 
 *  If \e gmsCallbacks is set to NULL, no callback is registered; otherwise,
 *  it is a pointer to a clGmsCallbacksT structure, containing the callback functions
 *  of the process that the Group Membership Service may invoke. Only non-NULL callback
 *  functions in this structure will be registered. This is an optional parameter.
 *
 *  \param version (in/out) It can have the following values: 
 *  \arg On input, this is the version desired by you.  
 *  \arg On return, the library returns the version it supports.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_NOT_INITIALIZED If library was not initialized. 
 *  \retval CL_ERR_NULL_POINTER On passing a NULL pointer.
 *  \retval CL_ERR_VERSION_MISMATCH If the requested version is not compatible with the ASP library.
 *  \retval CL_ERR_NO_RESOURCE If an instance or a new handle cannot be created.
 *
 *  \par Description: 
 *  This function initializes the Group Membership Service (GMS) for the invoking process and
 *  registers the various callback functions and negotiates the version of GMS library being used. 
 *  This function must be invoked prior to the
 *  invocation of any other Group Membership Service functionality. The handle
 *  \e gmsHandle is returned as the reference to this association between the process and
 *  the Group Membership Service. The process uses this handle in subsequent communication
 *  with the GMS.
 * 
 *  \par Library File:
 *  libClGms
 *
 *  
 *  \sa clGmsFinalize()
 *
 */

extern ClRcT clGmsInitialize(
	    CL_OUT   ClGmsHandleT          *gmsHandle,
	    CL_IN    const ClGmsCallbacksT *gmsCallbacks,
	    CL_INOUT ClVersionT            *version);


/**
 ************************************
 *  \brief Cleans up the GMS library.
 * 
 *  \par Header File:
 *  clClmTmsCommon.h
 * 
 *  \param gmsHandle The handle, obtained through the clGmsInitialize() function,
 *   designating this particular initialization of the Group Membership Service.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle.
 *
 *  \par Description:
 *   The clGmsFinalize() function closes the association, represented by the gmsHandle
 *  parameter, between the invoking process and the Group Membership Service. The
 *  process must have invoked clGmsInitialize() before it invokes this function. A process
 *  must invoke this function once for each handle it acquired by invoking
 *  clGmsInitialize(). <br><br>
 *  If the clGmsInitialize() function returns successfully, the clGmsFinalize() function
 *  releases all resources acquired when clGmsInitialize() was called. Moreover, it stops
 *  any tracking associated with the particular handle. Furthermore, it cancels all pending
 *  callbacks related to the particular handle. Note that because the callback invocation
 *  is asynchronous, it is still possible that some callback calls are processed after this
 *  call returns successfully.
 *  After clGmsFinalize() is invoked, the selection object is no longer valid. 
 * 
 *  \note
 *  On successful execution of this function, it releases all the resources allocated  
 *  during the initialization of the library.
 * 
 *  \par Library File:
 *  libClGms
 *
 * 
 *  \sa clGmsInitialize()
 *
 */
extern ClRcT clGmsFinalize(
        CL_IN ClGmsHandleT gmsHandle);

/**
 ************************************
 *  \brief Configures the cluster tracking mode.
 * 
 *  \par Header File:
 *  clClmTmsCommon.h
 *
 *  \param gmsHandle The handle, obtained through the clGmsInitialize() function,
 *   designating this particular initialization of the Group Membership Service.
 *  \param trackFlags Requested tracking mode. 
 *  \param notificationBuffer (in/out) Notification buffer provided by you while making a 
 *  request for the \c CURRENT view. This is an optional parameter.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle.
 *  \retval CL_ERR_INVALID_PARAMETER If \c CL_GMS_TRACK_CURRENT flag is set and notification 
 *  buffer is provided, but size of allocated array is not set (0).
 *  \retval CL_GMS_ERR_INVALID_TRACKFLAGS If both \c CHANGES and \c CHANGES_ONLY flags are set.
 *  \retval CL_ERR_NO_CALLBACK If request was asynchronous but no callback was registered.
 *  \retval CL_ERR_TRY_AGAIN Communication error, try again.
 *  \retval CL_ERR_TIMEOUT Communication request timed out.
 *
 *  \par Description:
 *  This API is used to configure the cluster tracking mode for the caller. It 
 *  can be called subsequently to modify the requested tracking mode.  
 *  This function is used to obtain the current cluster membership as well as to request
 *  notification of changes in the cluster membership or of changes in an attribute of a
 *  cluster node, depending on the value of the \e trackFlags parameter. <br><br> 
 *  These changes are notified via the invocation of the
 *  clGmsClusterTrackCallback() callback function, which must have been supplied
 *  when the process invoked the clGmsInitialize() call.
 *  An application may call clGmsClusterTrack() repeatedly for the same values of
 *  \e gmsHandle, regardless of whether the call initiates a one-time status request or a
 *  series of callback notifications.
 * 
 *  \par Library File:
 *  libClGms
 *   
 *  \sa clGmsClusterTrackStop(), clGmsClusterMemberGet(), clGmsClusterMemberGetAsync()
 *
 */
extern ClRcT clGmsClusterTrack(
        CL_IN    ClGmsHandleT           gmsHandle,
        CL_IN    ClUint8T               trackFlags,
        CL_INOUT ClGmsClusterNotificationBufferT *notificationBuffer);

/**
 ************************************
 *  \brief Stops all the clusters tracking.
 * 
 *  \par Header File:
 *  clClmTmsCommon.h
 *
 *  \param gmsHandle The handle, obtained through the clGmsInitialize() function,
 *   designating this particular initialization of the Group Membership Service.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle.
 *
 *  \par Description:
 *  This API is used to immediately stop the tracking of all the clusters for a given client.
 *  This function stops any further notifications through the handle gmsHandle. Pending
 *  callbacks are removed. This is usually invoked during the shut-down of the application. 
 * 
 *  \par Library File:
 *  libClGms
 * 
 *  \sa clGmsClusterTrack(), clGmsClusterMemberGet(), clGmsClusterMemberGetAsync()
 *
 */
extern ClRcT clGmsClusterTrackStop(
        CL_IN ClGmsHandleT              gmsHandle);

/**
 ************************************
 *  \brief Returns cluster member information.
 * 
 *  \par Header File:
 *  clClmTmsCommon.h
 *
 *  \param gmsHandle The handle, obtained through the clGmsInitialize() function,
 *   designating this particular initialization of the Group Membership Service.
 *  \param nodeId The identifier of the cluster node for which the
 *   \e clusterNode information structure is to be retrieved.
 *  \param timeout The clGmsClusterMemberGet() invocation is considered to have 
 *  failed if it does not complete by the time specified through this parameter.
 *  \param clusterMember (out) A pointer to a cluster node structure that contains 
 *  information about a cluster node. The invoking process provides space for this structure,
 *  and the Group Membership Service fills in the fields of this structure.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle.
 *  \retval CL_ERR_NULL_POINTER If the parameter \e clusterMember passed is a NULL pointer.
 *  \retval CL_ERR_TIMEOUT Communication request timed out.
 *  \retval CL_ERR_INVALID_PARAM Requested node does not exist
 *
 *  \par Description:
 *  This API is used to retrieve the information on a given cluster member and check whether
 *  the node is a member of the cluster. The space for the node must be allocated by you.
 *  This function provides the means for synchronously retrieving information about a
 *  cluster member, identified by the \e nodeId parameter. The cluster node information is
 *  returned in the \e clusterNode parameter. <br><br> 
 *  By invoking this function, a process can obtain the cluster node information for the
 *  node, designated by \e nodeId, and can then check the member field to determine
 *  whether this node is a member of the cluster.
 *  If the constant CL_GMS_LOCAL_NODE_ID is used as \e nodeId, the function returns
 *  information about the cluster node that hosts the invoking process. 
 * 
 *  \par Library File:
 *  libClGms
 * 
 *  \sa clGmsClusterTrack(), clGmsClusterTrackStop(), clGmsClusterMemberGetAsync()
 *
 */
extern ClRcT  clGmsClusterMemberGet(
        CL_IN  ClGmsHandleT             gmsHandle,
        CL_IN  ClGmsNodeIdT             nodeId, 
        CL_IN  ClTimeT                  timeout,
        CL_OUT ClGmsClusterMemberT     *clusterMember);

/**
 ************************************
 *  \brief Returns information on the cluster node asynchronously.
 * 
 *  \par Header File:
 *  clClmTmsCommon.h
 *
 *  \param gmsHandle The handle, obtained through the clGmsInitialize() function,
 *   designating this particular initialization of the Group Membership Service.
 *  \param invocation Correlates the invocation with the corresponding callback.
 *  This parameter allows the invoking process to match this invocation
 *  of clGmsClusterMemberGetAsync() with the corresponding
 *  clGmsClusterMemberGetCallback().
 *  \param nodeId The identifier of the cluster node for which the information
 *   is to be retrieved.
 *
 *  \retval CL_OK The API executed successfully.
 *  \retval CL_ERR_INVALID_HANDLE On passing an invalid handle.
 *  \retval CL_GMS_ERR_NODE_DOES_NOT_EXIST If the requested node does not exist.
 *
 *  \par Description:
 *  This API is used to query the information on a given cluster node. It makes
 *  an asynchronous call to \e ClGmsClusterMemberGetCallbackT(). 
 *  This function requests information, to be provided asynchronously, about the particular
 *  cluster node, identified by the \e nodeId parameter. If \c CL_GMS_LOCAL_NODE_ID is
 *  used as \e nodeId, the function returns information about the cluster node that hosts the
 *  invoking process. <br><br>
 *  The process sets invocation, which it uses subsequently to match
 *  the corresponding callback, clGmsClusterMemberGetCallback(), with this particular
 *  invocation. The clGmsClusterMemberGetCallback() callback function must have been
 *  supplied when the process invoked the clGmsInitialize() call.
 * 
 *  \par Library File:
 *  libClGms
 *   
 *  \sa clGmsClusterTrack(), clGmsClusterTrackStop(), clGmsClusterMemberGet()
 *
 */
extern ClRcT clGmsClusterMemberGetAsync(
        CL_IN ClGmsHandleT              gmsHandle,
        CL_IN ClInvocationT             invocation,
        CL_IN ClGmsNodeIdT              nodeId);


#ifdef  __cplusplus
}
#endif

#endif  /* _CL_CLM_TMS_COMMON_H */


/** \} */


