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
 * File        : clGmsCommon.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file contains the function call types used by the client library to
 * call the server and for the server to call its peer. 
 *  
 *
 *****************************************************************************/

#ifndef _CL_GMS_COMMON_H_
#define _CL_GMS_COMMON_H_

# ifdef __cplusplus
extern "C" {
# endif

#include <clCommon.h>
#include <clOsalApi.h>
#include <clDebugApi.h>
#include <errno.h>

#ifdef CL_GMS_SERVER 
#include <clLogApi.h>
#include <clGmsUtils.h>
#endif 

#include <clClmTmsCommon.h>
#include <clClmApi.h>
#include <clTmsApi.h>
#include <clEoApi.h>

/* CL_LOG_AREA parameters indicating, cluster service, group service
    * and also leader election algorithm */
#define CLM         "CLM"       /* Log area for cluster service */
#define GROUPS      "GRP"       /* Log area for group service */
#define LEA         "LEA"       /* Log area for leader election algorithm */
#define GEN         "GEN"       /* Log area for other general logging */
#define CKP         "CKP"       /* Log area for other general logging */
#define OPN         "OPN"       /* Log area for logging in OpenAIS context */

/* Creating shortcuts for log severity macros */
#define EMER        CL_LOG_EMERGENCY
#define ALERT       CL_LOG_ALERT
#define CRITICAL    CL_LOG_CRITICAL
#undef ERROR
#define ERROR       CL_LOG_ERROR
#define WARN        CL_LOG_WARNING
#define NOTICE      CL_LOG_NOTICE
#define INFO        CL_LOG_INFO
#define DBG         CL_LOG_DEBUG
#undef TRACE
#define TRACE       CL_LOG_TRACE

/* Shortcuts for log context */
#define NA          CL_LOG_CONTEXT_UNSPECIFIED
#define DB          "DB"
#define AIS          "AIS"

 

    


/*=============================================================================
 * Type/constant definitions:
 *===========================================================================*/

/*
 * Cluster/group function interfaces in the server that client can use
 * to interfac with the server. The ASYNC function types are omitted
 * because client has to handle the async part locally and call the
 * corresponding normal function types to call the server.
 */

#define    CL_GMS_CLIENT_RESERVED                     CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,0)
    /* Service session APIs */
#define    CL_GMS_CLIENT_INITIALIZE                   CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,1)
#define    CL_GMS_CLIENT_FINALIZE                     CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,2)
    /* Passive APIs */
#define    CL_GMS_CLIENT_CLUSTER_TRACK                CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,3)
#define    CL_GMS_CLIENT_CLUSTER_TRACK_STOP           CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,4)
#define    CL_GMS_CLIENT_CLUSTER_MEMBER_GET           CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,5)
#define    CL_GMS_CLIENT_CLUSTER_MEMBER_GET_ASYNC     CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,6)
#define    CL_GMS_CLIENT_GROUP_LIST_GET               CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,7)
#define    CL_GMS_CLIENT_GROUP_TRACK                  CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,8)
#define    CL_GMS_CLIENT_GROUP_TRACK_STOP             CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,9)
#define    CL_GMS_CLIENT_GROUP_MEMBER_GET             CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,10)
#define    CL_GMS_CLIENT_GROUP_MEMBER_GET_ASYNC       CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,11)
    /* Cluster manage APIs */
#define    CL_GMS_CLIENT_CLUSTER_JOIN                 CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,12)
#define    CL_GMS_CLIENT_CLUSTER_LEAVE                CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,13)
#define    CL_GMS_CLIENT_CLUSTER_LEADER_ELECT         CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,14)
#define    CL_GMS_CLIENT_CLUSTER_MEMBER_EJECT         CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,15)
    /* Group manage APIs */
#define    CL_GMS_CLIENT_GROUP_CREATE                 CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,16)
#define    CL_GMS_CLIENT_GROUP_DESTROY                CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,17)
#define    CL_GMS_CLIENT_GROUP_JOIN                   CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,18)
#define    CL_GMS_CLIENT_GROUP_LEAVE                  CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,19)
#define    CL_GMS_CLIENT_GROUP_LEADER_ELECT           CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,20)
#define    CL_GMS_CLIENT_GROUP_MEMBER_EJECT           CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,21)
#define    CL_GMS_SERVER_TOKEN                        CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,22)
#define    CL_GMS_SERVER_MCAST                        CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,23)
#define    CL_GMS_CLIENT_GROUP_INFO_GET               CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,24)
#define    CL_GMS_COMP_UP_NOTIFY                      CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,25)
#define    CL_GMS_CLIENT_GROUP_MCAST_SEND             CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,26)

/*
 * Function IDs to identify client callback function that the server needs to
 * call.
 */
#define    CL_GMS_CLIENT_RESERVED_CALLBACK             CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,0)
#define    CL_GMS_CLIENT_CLUSTER_TRACK_CALLBACK        CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,1)
#define    CL_GMS_CLIENT_CLUSTER_MEMBER_GET_CALLBACK   CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,2)
#define    CL_GMS_CLIENT_GROUP_TRACK_CALLBACK          CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,3)
#define    CL_GMS_CLIENT_GROUP_MEMBER_GET_CALLBACK     CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,4)
#define    CL_GMS_CLIENT_CLUSTER_MEMBER_EJECT_CALLBACK CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,5)
#define    CL_GMS_CLIENT_GROUP_MEMBER_EJECT_CALLBACK   CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,6)
#define    CL_GMS_CLIENT_GROUP_MCAST_CALLBACK          CL_EO_GET_FULL_FN_NUM(CL_EO_NATIVE_COMPONENT_TABLE_ID,7)

/*
 * Default RMD behavior for GMS
 */
#define CL_GMS_RMD_DEFAULT_TIMEOUT        CL_RMD_DEFAULT_TIMEOUT
#define CL_GMS_RMD_DEFAULT_RETRIES        CL_RMD_DEFAULT_RETRIES

/*=============================================================================
 * Type definitions for RMD client --> server function arguments:
 *===========================================================================*/
/*-----------------------------------------------------------------------------
 * Cluster Track API
 *---------------------------------------------------------------------------*/
typedef struct
{
    ClGmsHandleT        gmsHandle;          /* GMS Handle of requestor */
    ClUint8T            trackFlags;         /* Requested tracking flags */
    ClBoolT             sync;               /* Indicate a synchronous request */
    /* FIXME: The following should not be needed if the server could figure
     * out the clients address from the RMD call somehow.
     */
    ClIocAddressT       address;            /* Self address of client */
    ClVersionT          clientVersion;
} ClGmsClusterTrackRequestT;

typedef struct
{
    ClRcT               rc;                 /* Remote return code */
    ClGmsClusterNotificationBufferT buffer; /* Returned notification buffer,
                                             * Only used if trackFlags included
                                             * CL_GMS_TRACK_CURRENT
                                             */
    ClVersionT          serverVersion;
} ClGmsClusterTrackResponseT;

typedef struct
{
    ClRcT               rc;                 /* Remote return code */
    ClGmsHandleT        gmsHandle;          /* GMS Handle of requestor */
    ClGmsClusterNotificationBufferT buffer; /* Returned notification buffer */
    ClUint32T           numberOfMembers;    /* Current membership count */
} ClGmsClusterTrackCallbackDataT;

/*-----------------------------------------------------------------------------
 * Cluster Track Stop API
 *---------------------------------------------------------------------------*/
typedef struct
{
    ClGmsHandleT        gmsHandle;          /* GMS Handle of requestor */
    /* FIXME: The following should not be needed if the server could figure
     * out the clients address from the RMD call somehow.
     */
    ClIocAddressT       address;            /* Self address of client */
    ClVersionT          clientVersion;
} ClGmsClusterTrackStopRequestT;

typedef struct
{
    ClRcT               rc;                 /* Remote return code */
    ClVersionT          serverVersion;
} ClGmsClusterTrackStopResponseT;

/*-----------------------------------------------------------------------------
 * Cluster Member Get API
 *---------------------------------------------------------------------------*/
typedef struct
{
    ClGmsHandleT        gmsHandle;          /* GMS Handle of requestor */
    ClGmsNodeIdT        nodeId;             /* Id of node of interest */
    ClVersionT          clientVersion;
} ClGmsClusterMemberGetRequestT;

typedef struct
{
    ClRcT               rc;                 /* Remote return code */
    ClGmsClusterMemberT member;             /* All data on cluster node */
    ClVersionT          serverVersion;
} ClGmsClusterMemberGetResponseT;

/*-----------------------------------------------------------------------------
 * Cluster Member Get Async API
 *---------------------------------------------------------------------------*/
typedef struct
{
    ClGmsHandleT        gmsHandle;          /* GMS Handle of requestor */
    ClGmsNodeIdT        nodeId;             /* Id of node of interest */
    ClInvocationT       invocation;         /* Correlator */
    /* FIXME: The following should not be needed if the server could figure
     * out the clients address from the RMD call somehow.
     */
    ClIocAddressT       address;            /* Self address of client */
    ClVersionT          clientVersion;
} ClGmsClusterMemberGetAsyncRequestT;

typedef struct
{
    ClRcT               rc;                 /* Remote return code */
    ClVersionT          serverVersion;
} ClGmsClusterMemberGetAsyncResponseT;

typedef struct
{
    ClRcT               rc;                 /* Remote return code */
    ClGmsHandleT        gmsHandle;          /* GMS Handle of requestor */
    ClInvocationT       invocation;         /* Correlator */
    ClGmsClusterMemberT member;             /* All data on cluster node */
} ClGmsClusterMemberGetCallbackDataT;

/*-----------------------------------------------------------------------------
 * Group Track API
 *---------------------------------------------------------------------------*/
typedef struct
{
    ClGmsHandleT        gmsHandle;          /* GMS Handle of requestor */
    ClGmsGroupIdT       groupId;            /* Id of the requested group */
    ClUint8T            trackFlags;         /* Requested tracking flags */
    ClBoolT             sync;               /* Indicate if this is a sync call */
    /* FIXME: The following should not be needed if the server could figure
     * out the clients address from the RMD call somehow.
     */
    ClIocAddressT       address;            /* Self address of client */
    ClVersionT          clientVersion;
} ClGmsGroupTrackRequestT;

typedef struct
{
    ClRcT               rc;                 /* Remote return code */
    ClGmsGroupNotificationBufferT buffer;   /* Returned notification buffer,
                                             * Only used if trackFlags included
                                             * CL_GMS_TRACK_CURRENT
                                             */
    ClVersionT          serverVersion;
} ClGmsGroupTrackResponseT;

typedef struct
{
    ClRcT               rc;                 /* Remote return code */
    ClGmsHandleT        gmsHandle;          /* GMS Handle of requestor */
    ClGmsGroupIdT       groupId;            /* Id of the requested group */
    ClGmsGroupNotificationBufferT buffer;   /* Returned notification buffer */
    ClUint32T           numberOfMembers;    /* Current membership count */
} ClGmsGroupTrackCallbackDataT;

/*-----------------------------------------------------------------------------
 * Group Track Stop API
 *---------------------------------------------------------------------------*/
typedef struct
{
    ClGmsHandleT        gmsHandle;          /* GMS Handle of requestor */
    ClGmsGroupIdT       groupId;            /* Id of the requested group */
    /* FIXME: The following should not be needed if the server could figure
     * out the clients address from the RMD call somehow.
     */
    ClIocAddressT       address;            /* Self address of client */
    ClVersionT          clientVersion;
} ClGmsGroupTrackStopRequestT;

typedef struct
{
    ClRcT rc;                               /* Remote return code */
    ClVersionT          serverVersion;
} ClGmsGroupTrackStopResponseT;

/*-----------------------------------------------------------------------------
 * Group Member Get API
 *---------------------------------------------------------------------------*/
typedef struct
{
    ClGmsHandleT        gmsHandle;          /* GMS Handle of requestor */
    ClGmsGroupIdT       groupId;            /* Id of the requested group */
    ClGmsMemberIdT      memberId;           /* Id of requested member */
    ClVersionT          clientVersion;
} ClGmsGroupMemberGetRequestT;

typedef struct
{
    ClRcT               rc;                 /* Remote return code */
    ClGmsGroupMemberT   member;             /* All data on group member */
    ClVersionT          serverVersion;
} ClGmsGroupMemberGetResponseT;

/*-----------------------------------------------------------------------------
 * Group Member Get Async API
 *---------------------------------------------------------------------------*/
typedef struct
{
    ClGmsHandleT        gmsHandle;          /* GMS Handle of requestor */
    ClGmsGroupIdT       groupId;            /* Id of the requested group */
    ClGmsMemberIdT      memberId;           /* Id of requested member */
    ClInvocationT       invocation;         /* Correlator */
    /* FIXME: The following should not be needed if the server could figure
     * out the clients address from the RMD call somehow.
     */
    ClIocAddressT       address;            /* Self address of client */
    ClVersionT          clientVersion;
} ClGmsGroupMemberGetAsyncRequestT;

typedef struct
{
    ClRcT               rc;                 /* Remote return code */
    ClGmsGroupMemberT   member;             /* All data on group member */
    ClVersionT          serverVersion;
} ClGmsGroupMemberGetAsyncResponseT;

typedef struct
{
    ClRcT               rc;                 /* Remote return code */
    ClGmsHandleT        gmsHandle;          /* GMS Handle of requestor */
    ClInvocationT       invocation;         /* Correlator */
    ClGmsGroupIdT       groupId;            /* Id of the requested group */
    ClGmsGroupMemberT   member;             /* All data on cluster node */
} ClGmsGroupMemberGetCallbackDataT;

/*-----------------------------------------------------------------------------
 * Cluster Join API
 *---------------------------------------------------------------------------*/
typedef struct
{
    ClGmsHandleT        gmsHandle;          /* GMS Handle of requestor */
    ClGmsGroupIdT       groupId;            /* Id of the requested group */
    ClGmsLeadershipCredentialsT credentials;/* Leadership credentials */
    ClGmsNodeIdT        nodeId;             /* Node id of the joining node */
    ClNameT             nodeName;           /* Node name of joining node */
    ClBoolT             sync;               /* Indicate if this is a sync req */
    ClIocAddressT       address;         /* ioc address of the eo */
    ClVersionT          clientVersion;
} ClGmsClusterJoinRequestT;

typedef struct
{
    ClRcT rc;                               /* Remote return code */
    ClVersionT          clusterVersion;
    ClVersionT          serverVersion;
} ClGmsClusterJoinResponseT;

/*-----------------------------------------------------------------------------
 * Cluster Leave API
 *---------------------------------------------------------------------------*/
typedef struct
{
    ClGmsHandleT        gmsHandle;          /* GMS Handle of requestor */
    ClGmsGroupIdT       groupId;            /* Id of the requested group */
    ClGmsNodeIdT        nodeId;             /* Node id of the leaving node */
    ClBoolT             sync;               /* Indicate if this is a sync call */
    ClVersionT          clientVersion;
} ClGmsClusterLeaveRequestT;

typedef struct
{
    ClRcT rc;                               /* Remote return code */
    ClVersionT          serverVersion;
} ClGmsClusterLeaveResponseT;

/*-----------------------------------------------------------------------------
 * Cluster Leader Elect API
 *---------------------------------------------------------------------------*/
typedef struct
{
    ClGmsHandleT        gmsHandle;          /* GMS Handle of requestor */
    ClGmsNodeIdT        preferredLeaderNode; /* Preferred Node Name */
    ClVersionT          clientVersion;
} ClGmsClusterLeaderElectRequestT;

typedef struct
{
    ClRcT rc;                               /* Remote return code */
    ClGmsNodeIdT        leader;
    ClGmsNodeIdT        deputy;
    ClBoolT             leadershipChanged;
    ClVersionT          serverVersion;
} ClGmsClusterLeaderElectResponseT;

/*-----------------------------------------------------------------------------
 * Cluster Member Eject API
 *---------------------------------------------------------------------------*/

typedef struct
{
    ClGmsHandleT        gmsHandle;          /* GMS Handle of requestor */
    ClGmsNodeIdT        nodeId;             /* ID of node to be ejected */
    ClGmsMemberEjectReasonT reason;         /* Reason for ejection */
    ClVersionT          clientVersion;
} ClGmsClusterMemberEjectRequestT;

typedef struct
{
    ClRcT rc;                               /* Remote return code */
    ClVersionT          serverVersion;
} ClGmsClusterMemberEjectResponseT;

typedef struct
{
    ClGmsHandleT        gmsHandle;          /* GMS Handle for context */
    ClGmsMemberEjectReasonT reason;         /* Reason for ejection */
} ClGmsClusterMemberEjectCallbackDataT;

/*-----------------------------------------------------------------------------
 * Group Open API
 *---------------------------------------------------------------------------*/
typedef struct
{
    ClGmsHandleT        gmsHandle;          /* GMS Handle of requestor */
    ClGmsGroupNameT     groupName;          /* Name of the requested group */
    ClGmsGroupParamsT   groupParams;        /* Desired group parameters */
    ClVersionT          clientVersion;      /* Client GMS version */
} ClGmsGroupCreateRequestT;

typedef struct
{
    ClRcT rc;                               /* Remote return code */
    ClGmsGroupIdT       groupId;            /* Group ID corresponding to groupName */
    ClGmsGroupParamsT   groupParams;        /* Return the parameters if the group 
                                               already existed */
    ClVersionT          serverVersion;
} ClGmsGroupCreateResponseT;

/*-----------------------------------------------------------------------------
 * Group Close API
 *---------------------------------------------------------------------------*/
typedef struct
{
    ClGmsHandleT        gmsHandle;          /* GMS Handle of requestor */
    ClGmsGroupIdT       groupId;            /* Reference to group */
    ClVersionT          clientVersion;
} ClGmsGroupDestroyRequestT;

typedef struct
{
    ClRcT rc;                               /* Remote return code */
    ClVersionT          serverVersion;
} ClGmsGroupDestroyResponseT;

/*-----------------------------------------------------------------------------
 * Group Join API
 *---------------------------------------------------------------------------*/

typedef struct
{
    ClGmsHandleT        gmsHandle;          /* GMS Handle of requestor */
    ClGmsGroupIdT       groupId;            /* Reference to group to join */
    ClGmsMemberIdT      memberId;           /* Self Member ID */
    ClGmsMemberNameT    memberName;         /* Self Member Name */
    ClIocAddressT       memberAddress;      /* MemberIOC address and EO port */
    ClGmsLeadershipCredentialsT credentials;/* Leadership credentials */
    ClBoolT             sync;               /* Indicate if this is a sync req */
    ClVersionT          clientVersion;
} ClGmsGroupJoinRequestT;

typedef struct
{
    ClRcT rc;                               /* Remote return code */
    ClVersionT          serverVersion;
} ClGmsGroupJoinResponseT;

/*-----------------------------------------------------------------------------
 * Group Leave API
 *---------------------------------------------------------------------------*/
typedef struct
{
    ClGmsHandleT        gmsHandle;          /* GMS Handle of requestor */
    ClGmsGroupIdT       groupId;            /* Reference to group to leave */
    ClGmsMemberIdT      memberId;
    ClBoolT             sync;               /* Indicate if this is a sync req */
    ClVersionT          clientVersion;
} ClGmsGroupLeaveRequestT;

typedef struct
{
    ClRcT rc;                               /* Remote return code */
    ClVersionT          serverVersion;
} ClGmsGroupLeaveResponseT;

/*-----------------------------------------------------------------------------
 * Group Info List Get API
 *---------------------------------------------------------------------------*/
typedef struct
{
    ClGmsHandleT        gmsHandle;          /* GMS Handle of requestor */
    ClVersionT          clientVersion;
} ClGmsGroupsInfoListGetRequestT;

typedef struct
{
    ClRcT               rc;                 /* Remote return code */
    ClGmsGroupInfoListT groupsList;   
    ClVersionT          serverVersion;
} ClGmsGroupsInfoListGetResponseT;


/*-----------------------------------------------------------------------------
 * Group Info Get API
 *---------------------------------------------------------------------------*/
typedef struct
{
    ClGmsHandleT        gmsHandle;          /* GMS Handle of requestor */
    ClGmsGroupNameT     groupName;
    ClVersionT          clientVersion;
} ClGmsGroupInfoGetRequestT;

typedef struct
{
    ClRcT               rc;                 /* Remote return code */
    ClGmsGroupInfoT     groupInfo;   
    ClVersionT          serverVersion;
} ClGmsGroupInfoGetResponseT;

/*-----------------------------------------------------------------------------
 * Group Info Get API
 *---------------------------------------------------------------------------*/
typedef struct
{
    ClUint32T           compId;          /* GMS Handle of requestor */
    ClVersionT          clientVersion;
} ClGmsCompUpNotifyRequestT;

typedef struct
{
    ClRcT               rc;                 /* Remote return code */
    ClVersionT          serverVersion;
} ClGmsCompUpNotifyResponseT;

/*-----------------------------------------------------------------------------
 * Group Mcast Message Send API
 *---------------------------------------------------------------------------*/
typedef struct
{
    ClGmsGroupIdT       groupId;            /* Reference to group to leave */
    ClGmsMemberIdT      memberId;
    ClBoolT             sync;               /* Indicate if this is a sync req */
    ClVersionT          clientVersion;
    ClUint32T           dataSize;           /* User data size */
    ClPtrT              data;               /* User data to be multicasted */
} ClGmsGroupMcastRequestT;

typedef struct
{
    ClRcT rc;                               /* Remote return code */
    ClVersionT          serverVersion;
} ClGmsGroupMcastResponseT;

typedef struct 
{
    ClGmsHandleT        gmsHandle;          /* GMS Handle of group member */
    ClGmsGroupIdT       groupId;            /* group id on which this message is sent */
    ClGmsMemberIdT      memberId;           /* Sender Id */
    ClUint32T           dataSize;           /* Message data size */
    ClPtrT              data;               /* Mcast message */
} ClGmsGroupMcastCallbackDataT;

/*-----------------------------------------------------------------------------
 * Group Leader Elect API
 *---------------------------------------------------------------------------*/
typedef struct
{
    ClGmsHandleT        gmsHandle;          /* GMS Handle of requestor */
    ClGmsGroupIdT       groupId;            /* Reference to group */
    ClVersionT          clientVersion;
} ClGmsGroupLeaderElectRequestT;

typedef struct
{
    ClRcT rc;                               /* Remote return code */
    ClVersionT          serverVersion;
} ClGmsGroupLeaderElectResponseT;

/*-----------------------------------------------------------------------------
 * Group Member Eject API
 *---------------------------------------------------------------------------*/
typedef struct
{
    ClGmsHandleT        gmsHandle;          /* GMS Handle of requestor */
    ClGmsGroupIdT       groupId;            /* Reference to group */
    ClGmsMemberIdT      memberId;           /* Member to eject */
    ClGmsMemberEjectReasonT reason;         /* Reason code */
    ClVersionT          clientVersion;
} ClGmsGroupMemberEjectRequestT;

typedef struct
{
    ClRcT rc;                               /* Remote return code */
    ClVersionT          serverVersion;
} ClGmsGroupMemberEjectResponseT;

typedef struct
{
    ClGmsHandleT        gmsHandle;          /* GMS Handle for context */
    ClGmsGroupIdT       groupId;        /* Reference to group */
    ClGmsMemberEjectReasonT reason;         /* Reason for ejection */
} ClGmsGroupMemberEjectCallbackDataT;



typedef struct 
{
    ClVersionT clientVersion;
}ClGmsClientInitRequestT;


typedef struct 
{
    ClRcT rc;                               /* Remote return code */
    ClVersionT serverVersion;
}ClGmsClientInitResponseT;


typedef struct 
{
    ClOsalMutexIdT mutex;               /* mutex to protect the critical
                                           section*/
    ClOsalCondIdT  cond;                /* condtional variable to wait on */

}ClGmsCsSectionT;



/*=============================================================================
 * Common Utility functions used by server and client code base  
 *===========================================================================*/


    /*
       Osal functions should never return invalid return codes if properly
       initialize , it doesnt make any sense to proceed if mutexlock or
       conditional wait fails with invalid return code so we assert and exit
       out .
     */

/*
   clGmsMutexCreate
   ----------------
   Wrapper to create the mutex , asserts on failure .
   */

static void inline 
clGmsMutexCreate( ClOsalMutexIdT *pmutex  ){

    ClRcT _rc = CL_OK;

    _rc = clOsalMutexCreate ( pmutex );
    if(_rc != CL_OK ){
#ifdef CL_GMS_SERVER         
        clLog(ERROR,GEN,NA,
                 "\nclOsalMutexCreate failed with rc [0x%x]",_rc );
#endif 
        CL_ASSERT(_rc == CL_OK );
    }
}

static void inline 
clGmsMutexDelete( ClOsalMutexIdT mutex  ){

    ClRcT _rc = CL_OK;

    _rc = clOsalMutexDelete ( mutex );
    if(_rc != CL_OK ){
#ifdef CL_GMS_SERVER         
        clLog(ERROR,GEN,NA,
                 "\nclOsalMutexCreate failed with rc [0x%x]",_rc );
#endif 
        CL_ASSERT(_rc == CL_OK );
    }
}



/*
   clGmsMutexLock 
   --------------
   Wrapper to lock the mutex , asserts on failure .
   */


static void inline  
clGmsMutexLock( ClOsalMutexIdT mutex ){   

    ClRcT _rc = CL_OK;                 

    _rc = clOsalMutexLock( mutex );
    if(_rc != CL_OK ){            
#ifdef CL_GMS_SERVER 
        clLog(ERROR,GEN,NA,
                 "clOsalMutexLock failed with rc [0x%x]",_rc );                                  
#endif 
        CL_ASSERT(_rc == CL_OK );                       
    }                                                  
        return;
}




/*
   clGmsMutexUnlock
   -----------------
   Wrapper to unlock the mutex , asserts on failure .
   */


static void inline  
clGmsMutexUnlock( ClOsalMutexIdT mutex ){                

    ClRcT _rc = CL_OK;                    

    _rc = clOsalMutexUnlock( mutex ); 
    if(_rc != CL_OK ){         
#ifdef CL_GMS_SERVER 
        clLog(ERROR,GEN,NA,
                 "clOsalMutexLock failed with rc [0x%x]",_rc );                                  
#endif 
        CL_ASSERT(_rc == CL_OK );                       
    }                                                  
        return;
}





/*
   clGmsCondCreate
   ---------------
   Wrapper to create the conditional variable , asserts on failure .
   */

static void inline 
clGmsCondCreate( ClOsalCondIdT *pcond ){
    
    ClRcT _rc = CL_OK;

    _rc = clOsalCondCreate( pcond );
    if( _rc != CL_OK ){
#ifdef CL_GMS_SERVER 
        clLog(ERROR,GEN,NA,
                 "\nclOsalCondCreate failed with rc [0x%x]",_rc  );
#endif 
        CL_ASSERT( _rc == CL_OK );
    }
}


/*
   clGmsCondWait
   --------------
   Wrapper to wait on the conditional variable returns CL_ERR_TIMEOUT on
   timeout and asserts on any other failure. 
   */

static inline ClRcT 
clGmsCondWait ( 
        ClOsalCondIdT   cond , 
        ClOsalMutexIdT  mutex , 
        ClTimerTimeOutT timeout 
        ){

    ClRcT _rc = CL_OK;

    _rc = clOsalCondWait ( cond , mutex , timeout );
    if( _rc != CL_OK ){
        if( CL_GET_ERROR_CODE(_rc) == CL_ERR_TIMEOUT ){
            return _rc;
        }
#ifdef CL_GMS_SERVER 
        clLog(ERROR,GEN,NA,
                 "\nclOsalCondWait failed with rc [0x%x]",
                 _rc );
#endif 
        CL_ASSERT( _rc == CL_OK );
        return _rc;
    }
    return _rc;
}







/*
   clGmsCondSignal
   ---------------
   Wrapper to signal the conditional variable , asserts on failure .
   */
static void inline 
clGmsCondSignal ( 
        ClOsalCondIdT cond
        ){

    ClRcT _rc = CL_OK;

    _rc = clOsalCondSignal ( cond );
    if ( _rc != CL_OK ){
#ifdef CL_GMS_SERVER 
        clLog(ERROR,GEN,NA,
                 "\nclOsalCondSignal failed with rc [0x%x]",_rc  );
#endif 
        CL_ASSERT( _rc == CL_OK );
    }
    return;
}


/* 
   Critical Section Wrappers:
   --------------------------
   Critical section wrappers are used to create , enter , leave the critical
   sections . Critical section functions clGmsCsEnter and clGmsLeave should be
   called from different thread contexts (often called as producer and
   consumer threads ) . Code Enters the critical section by calling
   clGmsCsEnter and blocks for the signal, code comes out of the Critical
   section when another thread calls clGmsCsLeave. 
 */

/* 
    NOTE: These 2 functions are called from different thread contexts and the
    clGmsCsEnter is called prior to the clGmsCsLeave. 
 */



/*
   clGmsCsCreate
   --------------
   Function to create the critical section . 
   */
static inline void 
clGmsCsCreate( ClGmsCsSectionT *pcs ){

    clGmsMutexCreate( &pcs->mutex );
    clGmsCondCreate ( &pcs->cond  );
    return;
}



/*
   clGmsCsEnter
   ------------
   Function to enter the critical section , returns CL_ERR_TIMEOUT if timesout
   waiting for the leave. asserts on any other failure .
   */
static inline ClRcT 
clGmsCsEnter  ( ClGmsCsSectionT *pcs ,ClTimerTimeOutT timeout ){

    ClRcT _rc = CL_OK;

    clGmsMutexLock ( pcs->mutex );
    _rc = clGmsCondWait  ( pcs->cond , pcs-> mutex , timeout );
    clGmsMutexUnlock ( pcs -> mutex );
    return _rc;

}



/*
   clGmsCsLeave
   -------------
   Function to signal the thread to leave the critical section , asserts on
   failure.
   */
static void inline 
clGmsCsLeave ( ClGmsCsSectionT *pcs ){

    clGmsMutexLock ( pcs ->mutex );
    clGmsCondSignal ( pcs->cond );
    clGmsMutexUnlock ( pcs -> mutex );
    return;
}

extern int errno;

#ifdef  __cplusplus
}
#endif

#endif  /* _CL_GMS_COMMON_H_ */
