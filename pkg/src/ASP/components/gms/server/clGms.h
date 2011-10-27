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
 * File        : clGms.h
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *
 * Contains internal global data structures and function prototypes for the 
 * GMS component.
 *  
 *
 *****************************************************************************/

#ifndef _CL_GMS_H_
#define _CL_GMS_H_

# ifdef __cplusplus
extern "C" {
# endif

#include <netinet/in.h>

#include <clCommon.h>
#include <clEoApi.h>
#include <clCpmApi.h>
#include <clGmsDb.h>
#include <clGmsCommon.h>
#include <clClmApi.h>
#include <clVersionApi.h>
#include <clSAClientSelect.h>
#include <clHandleApi.h>
#include <clCkptExtApi.h>

/* Gms Server Instance Logging Options */
#define LOG_TO_FILE    ((0x01)<<0)
#define LOG_TO_CONSOLE ((0x01)<<1)    
#define MAX_FILE_NAME   1024
#define MAX_PATH_SIZE   786
#define OPTION_STR_SIZE 4


extern ClBoolT gClTotemRunning;

/* Stores the config info from the xml file */

typedef struct {
    /* Cluster Name from the config file */
    ClNameT	                clusterName;

    /* Max number of groups allowed on this node */
    ClUint16T               noOfGroups;

    /* Network addressed to be passed to openais to bind to */
    struct sockaddr_in      bind_net;

    /* Interface name given in the config file */
    ClCharT                 ifName[CL_MAX_NAME_LENGTH];

    /* Multicast address specified in the config file */
    struct sockaddr_in      mcast_addr;

    /* Multicast port number specified in the config file */
    ClInt32T                mcast_port;

    /* This node information */
    ClGmsClusterMemberT     thisNodeInfo;

    /* Leader election algorithm specified for the cluster in the
     * config file */
    ClGmsLeaderElectionAlgorithmT *leaderAlgDb;

    /* Version database supported */
    ClVersionDatabaseT versionsSupported;

    /* User ID the user running the ASP */
    ClCharT                 aisUserId[64];

    /* Group ID of the user running the ASP */
    ClCharT                 aisGroupId[64];

    /* Openais log option. It can be any of 'stderr', 'file',
     * 'syslog' or 'none'. Default is 'none' */
    ClCharT                 aisLogOption[CL_MAX_NAME_LENGTH];

    /* Preferred Active System controller node. */
    ClCharT                 preferredActiveSCNodeName[CL_MAX_NAME_LENGTH];

    /* Boot election timeout in seconds. Default is 5 seconds and it doesnt
     * have any effect on payload nodes. */
    ClUint32T               bootElectionTimeout;

    ClUint32T               leaderSoakInterval;
}  ClGmsConfigT;


/* Different states of the GMS server. This is the value 
 * passed to the SNMP when queried. Although only first 5 states are 
 * defined in the SNMP more internal states will be added to
 * the list whenever needed.
 */

typedef enum {

    CL_GMS_STATE_RUNNING        = 1,
    CL_GMS_STATE_STOP           = 2,
    CL_GMS_STATE_STARTING_UP    = 3,
    CL_GMS_STATE_SHUTING_DOWN   = 4,
    CL_GMS_STATE_UNAVAILABLE    = 5,
    CL_GMS_STATE_IN_SYNC        = 6

} ClGmsOpStateT;

/* GMS Admin state */

typedef enum {

    CL_GMS_RUNNING              = 0,
    CL_GMS_STOPPED              = 1,
    CL_GMS_UNAVAILABLE          = 2

} ClGmsAdminStateT;

/* Track Checkpoint metadata */
typedef struct ClGmsCkptMetaData {
    ClUint32T	*perGroupInfo;
    ClUint32T	currentNoOfGroups;
} ClGmsCkptMetaDataT;

/* GMS Global data structure. Accessable from anywhere in GMS code.
 * Make sure you lock it when you use change it.
 */

typedef struct {

    /* Holds data parsed from config file */
    ClGmsConfigT         config;

    /* Holds all the group views in the cluster. view 0 is a special
     * group which holds the cluster membership information.
     */
    ClGmsDbT             *db;

    /* Lock used for sync'ing access to this data structure.*/
    ClOsalMutexIdT       dbMutex;

    /* The db which holds the index for all the groups. */
    ClCntHandleT         groupNameIdDb;

    /* Mutex to lock name ID db */
    ClOsalMutexIdT       nameIdMutex;

    /* Holds the current state of the GMS server.
     */
    ClGmsOpStateT        opState;

    /* This node address and port number */
    ClIocAddressT        nodeAddr;

    /* Execution object of the gms EO */ 
    ClEoExecutionObjT   *gmsEoObject;

    /* Component handle allocated by the CPM Library */
    ClCpmHandleT         cpmHandle;

    /* GMS Component Name */
    ClNameT              gmsComponentName;

    /* Ckpt svn handle */
    ClCkptSvcHdlT        ckptSvcHandle;
} ClGmsNodeT;

/* Structure to hold the context information which is used as cookie
   for synchronization between GmsEngine and API handler */
typedef struct ClContextInfo {
    ClGmsCsSectionT condVar;    /* Conditional variable created by API handler */
    ClRcT           rc;         /* rc value from engine */
} ClContextInfoT;

/* Track data to be checkpointed */
typedef struct ClGmsTrackCkptData {
    ClGmsGroupIdT   groupId;
    ClGmsHandleT    gmsHandle;
    ClUint8T        trackFlag;
    ClIocAddressT   iocAddress;
} ClGmsTrackCkptDataT;


/* Global struct for holding GMS internal state and config variables */
extern ClGmsNodeT gmsGlobalInfo;

extern void *pluginHandle;

/*Global variable to indiate whether boot time election timer has completed or
 * not.
 */
extern ClBoolT         bootTimeElectionDone;

/*
 * Mutex element to protect the joining of multiple nodes.
 */
extern ClGmsCsSectionT joinCs;

/* Join mutex for groups */
extern ClGmsCsSectionT groupJoinCs;

/* Variable to use to serve group functionality */
extern ClBoolT  readyToServeGroups;

/* Handle database used to store the API context for synchronization */
extern ClHandleDatabaseHandleT contextHandleDatabase;

/*
 * Intializes the internal data structures of GMS and gets it
 * ready for prime time
 */

ClRcT _clGmsIntialize(
                    CL_IN   ClCharT  *);

/* Gets the parameter for current node */

void _clGmsGetThisNodeInfo(
                    CL_IN   ClGmsClusterMemberT  *);

/* Sets the parameter for current node */

void _clGmsSetThisNodeInfo(
                    CL_IN const  ClGmsNodeIdT                nodeId,
                    CL_IN const  ClNameT* const              nodeName,
                    CL_IN const  ClGmsLeadershipCredentialsT  credential);

/*
 * Cleans up the data structures initialized by the above function
 */

ClRcT _clGmsFinalize(void);

/* Parses the xml file and populates the gmsConfig data structure
 */

ClRcT _clGmsParseConfig(ClCharT *);


/* loads the plugin in to the GMS address space and stores the leader election
 *  algorithm in the GMS configuration data base */ 
void 
_clGmsLoadUserAlgorithm( 
        ClUint32T groupid , 
        char      *pluginPath 
        );

extern ClRcT 
_clGmsDefaultLeaderElectionAlgorithm(
        ClGmsClusterNotificationBufferT buffer,
        ClGmsNodeIdT            *leaderNodeId,
        ClGmsNodeIdT            *deputyNodeId, 
        ClGmsClusterMemberT     *memberJoinedOrLeft ,
        ClGmsGroupChangesT      cond 
        );

void 
_clGmsServiceInitialize ( const int argc , char* const argv[] );

int getNameString(ClNameT  *name,ClCharT   *retStr);

#ifdef  __cplusplus
}
#endif

#endif  /* _CL_GMS_H_ */


