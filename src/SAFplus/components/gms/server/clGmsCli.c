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
 * File        : clGmsCli.c
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *
 * Implements debug cli commands for testing.
 *
 *****************************************************************************/

#include <string.h>
#include <errno.h>
#include <clCommonErrors.h>
#include <clGmsCli.h>
#include <clOsalApi.h>
#include <clGmsErrors.h>
#include <clLogApi.h>
#include <clGms.h>
#include <clGmsMsg.h>
#include <clGmsApiHandler.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* Function declarations */

static void _clGmsCliMakeError (
            CL_INOUT   ClCharT**  const     ret,
            CL_IN      const ClCharT* const str)
{
    ClUint32T size;

    if (ret == NULL)
    {
        return;
    }

    size = (ClUint32T)strlen(str);

    *ret = (ClCharT *)clHeapAllocate(size);

    if (*ret)
       strncpy(*ret, str, size); 

    return;
} 

/* Converts the input string and sets error string
 * in ret to return  if invalid string is passed.
 */

static ClRcT  _clGmsCliGetNumeric(
            CL_IN    const   ClCharT* const argv_str,
            CL_OUT   ClUint64T* const num)
{
    ClInt64T cnum;

    if (num == NULL)
        return CL_ERR_NULL_POINTER;

    *num = 0;
    errno = 0;
 
    cnum = (ClUint64T)strtoll(argv_str, NULL, 10);

        /* FIXME: Is errno checking allowed? */

    if ((errno == EINVAL) || (errno == ERANGE))
        return CL_GMS_RC(CL_ERR_INVALID_PARAMETER);

    if (cnum < 0)
        return CL_GMS_RC(CL_ERR_INVALID_PARAMETER);

    *num = (ClUint64T) cnum;

    return CL_OK;
}

/* Gets all the Cluster members in the cluster/Group list.
 * Input is group id. Group id 0 is for cluster. */


#define VIEW_LIST_USAGE "Usage: memberList <group_id>\n"\
     "\tThis CLI is used to list the current members of a given group\n"\
     "\t<group-id> : Group Id [+ve integer between 0 to 2^32-1] \n"\
     "\t             Provide group-id '0' for the cluster\r"
static ClRcT gmsCliGetViewMemberList(
            CL_IN   ClUint32T argc, 
            CL_IN   ClCharT** argv, 
            CL_OUT  ClCharT** ret)
{
    ClRcT          rc = CL_OK;
    ClUint64T      num = 0;
    ClGmsGroupIdT  groupId = CL_GMS_INVALID_GROUP_ID;

    if (argc != 2 )
    {
        _clGmsCliMakeError( ret, VIEW_LIST_USAGE );
        return CL_OK;
    }
    rc = _clGmsCliGetNumeric(argv[1], (ClUint64T*)&num);

    if (rc != CL_OK) 
    {
        _clGmsCliMakeError(ret, "Invalid Group Id passed\n\0");
        return CL_OK;   /* CLI command itself succeeded but input
                         * is wrong. So return OK. */
    }
    groupId = (ClGmsGroupIdT) num;

    rc = _clGmsViewCliPrint(groupId, ret);

    return rc;
}

#define TRACK_LIST_USAGE "Usage: trackMemberList <group_id>\n"\
    "\tThis CLI is used to list the tracking nodes for the given group id\n"\
    "\t<group-id> : Group Id [+ve integer between 0 to 2^32-1] \n\r"

static ClRcT gmsCliGetTrackList(
            CL_IN   ClUint32T argc, 
            CL_IN   ClCharT** argv, 
            CL_OUT  ClCharT** ret)
{
    ClRcT           rc = CL_OK;
    ClGmsGroupIdT   groupId = CL_GMS_INVALID_GROUP_ID;

    if (argc != 2)
    {
        _clGmsCliMakeError( ret, TRACK_LIST_USAGE );
        return CL_OK;
    }
    groupId = (ClGmsGroupIdT)strtol(argv[1], NULL, 10);

    /* FIXME: Is errno checking allowed? */

    if ((errno == EINVAL) || (errno == ERANGE))
    {
        _clGmsCliMakeError(ret, "Invalid groupID passed.\n\0");
        return CL_OK;
    }

    rc = _clGmsTrackCliPrint(groupId, ret);

    return rc;
}

#define INFO_USAGE "Usage : thisNodeInfo\n\r"

static ClRcT 
gmsCliShowGmsInfo(
            CL_IN   ClUint32T argc, 
            CL_IN   ClCharT** argv, 
            CL_OUT  ClCharT** ret
            )
{
    ClRcT   rc = CL_OK;
    ClCharT timeBuffer[256] = {0};
    ClTimeT ti = 0;

     if (argc > 1)
     {
         _clGmsCliMakeError(ret, INFO_USAGE);
         return CL_OK;
     }

    /* Allocate maximum possible */ 
    *ret = clHeapAllocate(1020);
    if( *ret == NULL ){
        clLog (ERROR,GEN,NA,
                "Memory allocation failed");
        return CL_ERR_NO_MEMORY;
    }
    _clGmsCliPrint(ret ,"\nClovis Group Membership Service");
    _clGmsCliPrint(ret ,"\n-------------------------------");
    gmsGlobalInfo.config.clusterName.value[gmsGlobalInfo.config.clusterName.length+1]='\0';
    _clGmsCliPrint(ret, "\n\nDeployment Configuration");
    _clGmsCliPrint(ret, "\n--------------------------------------");
    _clGmsCliPrint(ret ,"\nName Of Cluster          : %s ",
            gmsGlobalInfo.config.clusterName.value);
    _clGmsCliPrint(ret ,"\nNumber Of Groups Allowed : %d ",
            gmsGlobalInfo.config.noOfGroups);
    _clGmsCliPrint(ret ,"\nNetwork Address Using    : %s ",
            inet_ntoa(gmsGlobalInfo.config.bind_net.sin_addr));
    _clGmsCliPrint(ret ,"\nMulticast Address Using  : %s ",
            inet_ntoa(gmsGlobalInfo.config.mcast_addr.sin_addr));
    _clGmsCliPrint(ret ,"\nMulticast Port Boundto   : %d ",
            gmsGlobalInfo.config.mcast_port);
    _clGmsCliPrint(ret ,"\nGMS software version     : %c.%d.%d",
            gmsGlobalInfo.config.thisNodeInfo.gmsVersion.releaseCode,
            gmsGlobalInfo.config.thisNodeInfo.gmsVersion.majorVersion,
            gmsGlobalInfo.config.thisNodeInfo.gmsVersion.minorVersion
            );
    _clGmsCliPrint(ret, "\n--------------------------------------");
    _clGmsCliPrint(ret, "\n\nThis Node Information");
    _clGmsCliPrint(ret, "\n--------------------------------------");
    _clGmsCliPrint(ret, "\nNode Id                  : 0x%x ",
            gmsGlobalInfo.config.thisNodeInfo.nodeId 
            );
    _clGmsCliPrint(ret, "\nInitial View Number      : 0x%x ",
            gmsGlobalInfo.config.thisNodeInfo.initialViewNumber
            );
    gmsGlobalInfo.config.thisNodeInfo.nodeName.value
        [gmsGlobalInfo.config.thisNodeInfo.nodeName.length+1]='\0';
    _clGmsCliPrint(ret, "\nNode Name                : %s ",
            gmsGlobalInfo.config.thisNodeInfo.nodeName.value
            );
    _clGmsCliPrint(ret, "\nMembership State         : %s ",
            gmsGlobalInfo.config.thisNodeInfo.memberActive?"ACTIVE":"INACTIVE"
            );

    ti = gmsGlobalInfo.config.thisNodeInfo.bootTimestamp/CL_GMS_NANO_SEC;
    _clGmsCliPrint(ret, "\nBoot Timestamp           : %s",
            ctime_r((const time_t*)&ti,timeBuffer));

    _clGmsCliPrint(ret, "Credentials              : %d \n",
            gmsGlobalInfo.config.thisNodeInfo.credential
            );
    
    return rc;
}

static ClRcT gmsCliGroupsInfoListGet (
                    CL_IN   ClUint32T argc,
                    CL_IN   ClCharT** argv,
                    CL_OUT  ClCharT** ret)
{
    ClGmsGroupInfoT *groupInfoList  = NULL;
    ClUint32T        index          = 0;
    ClUint32T        numberOfGroups = 0;
    ClCharT          name[256]      = "";
    ClCharT          timeBuffer[256]= {0};
    ClTimeT          ti             = 0;
    ClInt32T maxBytes = 0;
    ClInt32T curBytes = 0;

    if (argc > 1)
    {
        _clGmsCliMakeError(ret, "Usage: allGroupsInfo\r");
        return CL_OK;
    }

    *ret = NULL;
    
    /* Take the lock on the database */
    clGmsMutexLock(gmsGlobalInfo.dbMutex);
    for (index = 0; index < gmsGlobalInfo.config.noOfGroups; index++)
    {
        if ((gmsGlobalInfo.db[index].view.isActive == CL_TRUE) &&
                (gmsGlobalInfo.db[index].viewType == CL_GMS_GROUP))
        {
            numberOfGroups++;
            groupInfoList = realloc(groupInfoList,
                                    sizeof(ClGmsGroupInfoT)*numberOfGroups);
            if (groupInfoList == NULL)
            {
                clGmsMutexUnlock(gmsGlobalInfo.dbMutex);
                return CL_ERR_NO_MEMORY;
            }
            memcpy(&groupInfoList[numberOfGroups-1],&gmsGlobalInfo.db[index].groupInfo, 
                    sizeof(ClGmsGroupInfoT));
        }
    }
    clGmsMutexUnlock(gmsGlobalInfo.dbMutex);

    clDebugPrintExtended(ret, &maxBytes, &curBytes, 
                         "-------------------------------------------------------------------------\n");
    clDebugPrintExtended(ret, &maxBytes, &curBytes,
                         "Total No Of Groups : %d\n",numberOfGroups);
    clDebugPrintExtended(ret, &maxBytes, &curBytes,
                         "-------------------------------------------------------------------------\n");
    if (numberOfGroups == 0)
    {
        goto done_ret;
    }

    clDebugPrintExtended(ret, &maxBytes, &curBytes,
                         "GroupName     GId  noOfMembers  setForDelete  IocMCAddr       creationTime\n");
    clDebugPrintExtended(ret, &maxBytes, &curBytes,
                         "-------------------------------------------------------------------------\n");
    for (index = 0; index < numberOfGroups; index++)
    {
        getNameString(&groupInfoList[index].groupName, name);
        ti = groupInfoList[index].creationTimestamp/CL_GMS_NANO_SEC;
        clDebugPrintExtended(ret, &maxBytes, &curBytes,
                             "%-13s %3d  %11d  %12s  %16llx %s",
                             name, groupInfoList[index].groupId, groupInfoList[index].noOfMembers,
                             groupInfoList[index].setForDelete == CL_TRUE ? "Yes": "No",
                             groupInfoList[index].iocMulticastAddr,
                             ctime_r((const time_t*)&ti,timeBuffer));
    }

done_ret:
    free(groupInfoList);
    return CL_OK;
}


#define GROUP_INFO_USAGE "Usage: groupInfo <group_name>\n"\
     "\tThis CLI is used to get the meta information of a given group\n\n" \
     "\t<group_name> : Group Name string \n\n" \
     "\tNOTE: This API is not valid for a cluster group\r"
static ClRcT gmsCliGetGroupInfo (
                    CL_IN   ClUint32T argc,
                    CL_IN   ClCharT** argv,
                    CL_OUT  ClCharT** ret)
{
    ClNameT          groupName      = {0};
    ClGmsGroupIdT    groupId        = 0;
    ClGmsGroupInfoT  groupInfo      = {{0}};
    ClCharT          timeBuffer[256]= {0};
    ClTimeT          ti             = 0;
    ClRcT            rc             = CL_OK;
    ClGmsDbT        *thisViewDb     = NULL;


    /* Allocate maximum possible */ 
    *ret = clHeapAllocate(3000);
    if( *ret == NULL ){
        clLog (ERROR,GEN,NA,
                "Memory allocation failed");
        return CL_ERR_NO_MEMORY;
    }

    memset(*ret,0,3000);

    if (argc != 2)
    {
        _clGmsCliMakeError( ret, GROUP_INFO_USAGE );
        return CL_OK;
    }

    groupName.length = strlen(argv[1]);
    if (groupName.length <= 0)
    {
        _clGmsCliMakeError(ret, "Invalid group name provided");
        return CL_OK;
    }

    strncpy(groupName.value,argv[1],groupName.length);

    /* Take the lock on the database */
    clGmsMutexLock(gmsGlobalInfo.nameIdMutex);

    rc = _clGmsNameIdDbFind(&gmsGlobalInfo.groupNameIdDb, &groupName, &groupId);

    clGmsMutexUnlock(gmsGlobalInfo.nameIdMutex);

    if (rc != CL_OK)
    {
        _clGmsCliMakeError(ret, "Given Group does not exist");
        return rc;
    }

    rc = _clGmsViewDbFind(groupId,&thisViewDb);
    if (rc != CL_OK)
    {
        return rc;
    }

    clGmsMutexLock(thisViewDb->viewMutex);
    memcpy(&groupInfo,&thisViewDb->groupInfo,sizeof(ClGmsGroupInfoT));
    clGmsMutexUnlock(thisViewDb->viewMutex);

    _clGmsCliPrint(ret, "------------------------------------------------\n");
    _clGmsCliPrint(ret, "Group Info for      : %s\n",argv[1]);
    _clGmsCliPrint(ret, "------------------------------------------------\n");

    _clGmsCliPrint(ret, "Group ID            : %d\n",
            groupInfo.groupId);

    _clGmsCliPrint(ret, "Is IOC Group        : %s\n",
            groupInfo.groupParams.isIocGroup == CL_TRUE ? "Yes" : "No");

    _clGmsCliPrint(ret, "No Of Members       : %d\n",
            groupInfo.noOfMembers);

    _clGmsCliPrint(ret, "Marked for Delete   : %s\n",
            groupInfo.setForDelete == CL_TRUE ? "Yes" : "No");

    _clGmsCliPrint(ret, "IOC MCast Address   : 0x%llx\n",
            groupInfo.iocMulticastAddr);

    ti = groupInfo.creationTimestamp/CL_GMS_NANO_SEC;
    _clGmsCliPrint(ret, "Creation Time Stamp : %s",
            ctime_r((const time_t*)&ti,timeBuffer));

    ti = groupInfo.lastChangeTimestamp/CL_GMS_NANO_SEC;
    _clGmsCliPrint(ret, "Last Changed Time   : %s\n",
            ctime_r((const time_t*)&ti,timeBuffer));

    return CL_OK;
}



#define LEADER_ELECT_USAGE "Usage: leaderElect <preferredLeader>\n"\
     "\tThis CLI is used to initiate an explicit leader election.\n\n" \
     "\t<preferredLeader> : Node ID of the preferred Leader node. \n\n" \
     "\tNOTE: This CLI is used to initiate an explicit leader election.\r" \
     "\t      The CLI will first see if the preferredLeader is already \r" \
     "\t      the cluster leader, then it will just return. Otherwise, \r" \
     "\t      if the preferredLeader is a deputy node, then this CLI \r" \
     "\t      will return in a swithover of the node. So the deputy  \r" \
     "\t      will become the new leader. Lastly if the preferredLeader \r" \
     "\t      value is neither an existing leader or the deputy, an error \r" \
     "\t      is returned\r" 
static ClRcT gmsLeaderElect (
                    CL_IN   ClUint32T argc,
                    CL_IN   ClCharT** argv,
                    CL_OUT  ClCharT** ret)
{
    ClRcT    rc = CL_OK;
    ClGmsClusterLeaderElectRequestT         req = {0};
    ClGmsClusterLeaderElectResponseT        res = {0};


    /* Allocate maximum possible */ 
    *ret = clHeapAllocate(1020);
    if( *ret == NULL ){
        clLog (ERROR,GEN,NA,
                "Memory allocation failed");
        return CL_ERR_NO_MEMORY;
    }
    memset(*ret,0,1020);

    if (argc != 2)
    {
        _clGmsCliMakeError( ret, LEADER_ELECT_USAGE );
        return CL_OK;
    }

    req.gmsHandle = 0;
    req.preferredLeaderNode = atoi(argv[1]);
    req.clientVersion = gmsGlobalInfo.config.versionsSupported.versionsSupported[0] ;

    rc = clGmsClusterLeaderElectHandler(&req, &res);
    if ((rc != CL_OK) && (rc != CL_ERR_ALREADY_EXIST))
    {
        _clGmsCliPrint(ret, "ClusterLeaderElect failed with rc 0x%x \n",rc);
        return rc;
    }

    if (rc == CL_ERR_ALREADY_EXIST)
    {
        _clGmsCliPrint(ret, "Node [%d] is already the leader node\n",req.preferredLeaderNode);
    }

    _clGmsCliPrint(ret, "New leader = %d\nNew Deputy = %d\nLeadership Changed = %s\n",
            res.leader,res.deputy,(res.leadershipChanged == CL_TRUE) ? "Yes" : "No");

    return CL_OK;
}
/* Disabling below CLIs as they are not for generic usage */
#if 0
#define TRACK_ADD_USAGE \
        "Usage: track_add_node <viewId> <GMSHandle> <NodeAddress> <PortId> "\
         "<trackFlags>\n"\
         "\tThis CLI is used to add a node to the tracking list for the given View ID\n"\
         "\t<viewId> : Starting view Id for the tracking node.\n"\
         "\t<GMSHandle> : Handle value received during GMS Initialize\n"\
         "\t<NodeAddress> : Address of the tracking node to be added\n"\
         "\t<PortId> : Port ID on the tracking node on which it would receive"\
         "the track messages.\n\r" 

static ClRcT   gmsCliAddTrackNode(
            CL_IN   ClUint32T argc, 
            CL_IN   ClCharT** argv, 
            CL_OUT  ClCharT** ret)
{ 
    ClRcT           rc = CL_OK;
    ClUint64T       num = 0;
    ClGmsGroupIdT   groupId = CL_GMS_INVALID_GROUP_ID;
    ClGmsHandleT    gmsHandle = CL_GMS_INVALID_HANDLE;
    ClIocAddressT   iocAddress = {{0}};
    ClUint8T        trackFlags = 0;
    ClGmsTrackNodeT *trackNode=NULL;
    ClGmsTrackNodeKeyT  key = {0};

    

    if (argc != 6)
    {
        _clGmsCliMakeError(ret, TRACK_ADD_USAGE);

        return CL_OK;
    }

    rc = _clGmsCliGetNumeric(argv[1], (void *)&num);
    
    if (rc != CL_OK) 
    {
        _clGmsCliMakeError(ret, "Invalid View Id passed\n\0");
        return CL_OK;   /* CLI command itself succeeded but input
                         * is wrong. So return OK. */
    }

    groupId = (ClGmsGroupIdT) num;

    rc = _clGmsCliGetNumeric(argv[1], (void *)&num);
    
    if (rc != CL_OK) 
    {
        _clGmsCliMakeError(ret, "Invalid handle passed\n\0");
        return CL_OK;   /* CLI command itself succeeded but input
                         * is wrong. So return OK. */
    }

    gmsHandle = (ClGmsHandleT) num;

    rc = _clGmsCliGetNumeric(argv[1], (void *)&num);
    
    if (rc != CL_OK) 
    {
        _clGmsCliMakeError(ret, "Invalid node address passed\n\0");
        return CL_OK;   /* CLI command itself succeeded but input
                         * is wrong. So return OK. */
    }

    iocAddress.iocPhyAddress.nodeAddress = (ClIocNodeAddressT) num;

    rc = _clGmsCliGetNumeric(argv[1], (void *)&num);
    
    if (rc != CL_OK) 
    {
        _clGmsCliMakeError(ret, "Invalid port Id passed\n\0");
        return CL_OK;   /* CLI command itself succeeded but input
                         * is wrong. So return OK. */
    }

    iocAddress.iocPhyAddress.portId = (ClIocNodeAddressT) num;

    rc = _clGmsCliGetNumeric(argv[1], (void *)&num);
    
    if (rc != CL_OK) 
    {
        _clGmsCliMakeError(ret, "Invalid track flags passed\n\0");
        return CL_OK;   /* CLI command itself succeeded but input
                         * is wrong. So return OK. */
    }

    trackFlags = (ClUint8T) num;

    trackNode = (ClGmsTrackNodeT *) clHeapAllocate(sizeof(ClGmsTrackNodeT*));

    if (!trackNode)
    {
        _clGmsCliMakeError(ret, "trackNode memory allocation failed\n\0");
        return CL_OK;
    }

    trackNode->handle     = gmsHandle;
    trackNode->address    = iocAddress;
    trackNode->trackFlags = trackFlags;

    key.handle  = gmsHandle;
    key.address = iocAddress;

    rc = _clGmsTrackAddNode(groupId, key, trackNode, 0);

    return rc;
}

#define TRACK_DELETE_USAGE \
       "Usage: track_delete_node <viewId> <GMSHandle> <HostAddress> <portId>\n"\
        "\tThis CLI is used to delete a tracking node for the given view ID\n"\
        "\t<viewId> : View ID of the tracking node.\n"\
        "\t<GMSHandle> : GMS handle value received during GMS Initialization\n"\
        "\t<HostAddress> : Address of the node to be deleted\n"\
        "\t<portId> : Port ID on which the tracking node was listening.\n\r"

static ClRcT   gmsCliDelTrackNode(
            CL_IN   ClUint32T argc, 
            CL_IN   ClCharT** argv, 
            CL_OUT  ClCharT** ret)
{ 
    ClRcT   rc = CL_OK;
    ClUint64T       num = 0;
    ClGmsGroupIdT   groupId = CL_GMS_INVALID_GROUP_ID;
    ClGmsHandleT    gmsHandle = CL_GMS_INVALID_HANDLE;
    ClIocAddressT   iocAddress = {{0}};
    ClGmsTrackNodeKeyT  key = {0};

    

    if (argc != 6)
    {
        _clGmsCliMakeError(ret, TRACK_DELETE_USAGE);

        return CL_OK;
    }

    rc = _clGmsCliGetNumeric(argv[1], (void *)&num);
    
    if (rc != CL_OK) 
    {
        _clGmsCliMakeError(ret, "Invalid View Id passed\n\0");
        return CL_OK;   /* CLI command itself succeeded but input
                         * is wrong. So return OK. */
    }

    groupId = (ClGmsGroupIdT) num;

    rc = _clGmsCliGetNumeric(argv[1], (void *)&num);
    
    if (rc != CL_OK) 
    {
        _clGmsCliMakeError(ret, "Invalid handle passed\n\0");
        return CL_OK;   /* CLI command itself succeeded but input
                         * is wrong. So return OK. */
    }

    gmsHandle = (ClGmsHandleT) num;

    rc = _clGmsCliGetNumeric(argv[1], (void *)&num);
    
    if (rc != CL_OK) 
    {
        _clGmsCliMakeError(ret, "Invalid node address passed\n\0");
        return CL_OK;   /* CLI command itself succeeded but input
                         * is wrong. So return OK. */
    }

    iocAddress.iocPhyAddress.nodeAddress = (ClIocNodeAddressT) num;

    rc = _clGmsCliGetNumeric(argv[1], (void *)&num);
    
    if (rc != CL_OK) 
    {
        _clGmsCliMakeError(ret, "Invalid port Id passed\n\0");
        return CL_OK;   /* CLI command itself succeeded but input
                         * is wrong. So return OK. */
    }

    iocAddress.iocPhyAddress.portId = (ClIocNodeAddressT) num;

    key.handle  = gmsHandle;
    key.address = iocAddress;

    rc = _clGmsTrackDeleteNode(groupId, key);

    return rc;
}

#define VIEW_ADD_USAGE "Usage : view_add <viewId> <viewName>\n"\
    "\tThis CLI is used to create a new view for the given view ID and view Name\n"\
    "\t<viewId> : View ID for the new View\n"\
    "\t<viewName> : Name for the new View\n\r"

static ClRcT   gmsCliAddView(
            CL_IN   ClUint32T argc, 
            CL_IN   ClCharT** argv, 
            CL_OUT  ClCharT** ret)
{
    ClRcT   rc = CL_OK;
    ClUint64T       num = 0;
    ClGmsGroupIdT   groupId = CL_GMS_INVALID_GROUP_ID;
    ClNameT         name = {0};

    if (argc != 3)
    {
        _clGmsCliMakeError(ret, VIEW_ADD_USAGE);

        return CL_OK;
    }

    rc = _clGmsCliGetNumeric(argv[1], (ClUint64T*)&num);
    
    if (rc != CL_OK) 
    {
        _clGmsCliMakeError(ret, "Invalid View Id passed\n\0");
        return CL_OK;   /* CLI command itself succeeded but input
                         * is wrong. So return OK. */
    }

    groupId = (ClGmsGroupIdT)num;

    strncpy(name.value, argv[2], strlen(argv[2]));
    name.length = strlen(argv[2]);

    rc = _clGmsViewCreate(name, groupId);

    return rc;
}

#define VIEW_NODE_ADD_USAGE \
     "Usage:view_add_node <viewId> <nodeId> <nodeName> <IOC Addr> <IOC Port>"\
     "<credentials> <viewNumber>\n"\
     "\tThis CLI is used to add a node for the view list for the given ViewID\n"\
     "\t<viewId> : View ID on which the node would get the cluster/group view\n"\
     "\t<nodeId> : Node ID of the node being added to the view database\n"\
     "\t<nodeName> : Node Name of the node being added to the view database\n"\
     "\t<IOC Addr> : IOC Address of the node being added to the viewdatabase\n"\
     "\t<IOC Port> : IOC Port on which the node listens for view updates\n"\
     "\t<credentials> : Leadership credentials of the node being added\n"\
     "\t<viewNumber> : View Number\n\r"
 
static ClRcT   gmsCliAddViewNode(
            CL_IN   ClUint32T argc, 
            CL_IN   ClCharT** argv, 
            CL_OUT  ClCharT** ret)
{ 
    ClRcT           rc = CL_OK;
    ClGmsViewNodeT  *viewNode = NULL;
    ClUint64T       num = 0;
    ClGmsGroupIdT   groupId = CL_GMS_INVALID_GROUP_ID;
    ClGmsMemberIdT  memberId = 0;
    ClIocAddressT   iocAddress = {{0}};
    ClGmsLeadershipCredentialsT credentials = CL_GMS_INELIGIBLE_CREDENTIALS;
    ClUint64T       viewNumber = 0; 
 

    

    if (argc != 8)
    {
        _clGmsCliMakeError(ret, VIEW_NODE_ADD_USAGE);

        return CL_OK;
    }

    rc = _clGmsCliGetNumeric(argv[1], (void *)&num);
    
    if (rc != CL_OK) 
    {
        _clGmsCliMakeError(ret, "Invalid View Id passed\n\0");
        return CL_OK;   /* CLI command itself succeeded but input
                         * is wrong. So return OK. */
    }

    groupId = (ClGmsGroupIdT) num;

    rc = _clGmsCliGetNumeric(argv[2], (void *)&num);
    
    if (rc != CL_OK) 
    {
        _clGmsCliMakeError(ret, "Invalid Node Id passed\n\0");
        return CL_OK;   /* CLI command itself succeeded but input
                         * is wrong. So return OK. */
    }
   
    memberId = (ClGmsMemberIdT) num;
 
    rc = _clGmsCliGetNumeric(argv[4], (ClUint64T*)&num);

    if (rc != CL_OK) 
    {
        _clGmsCliMakeError(ret, "Invalid IOC address passed\n\0");
        return CL_OK;   /* CLI command itself succeeded but input
                         * is wrong. So return OK. */
    }

    iocAddress.iocPhyAddress.nodeAddress = (ClIocNodeAddressT) num; 
    rc = _clGmsCliGetNumeric(argv[5], (ClUint64T*)&num);

    if (rc != CL_OK) 
    {
        _clGmsCliMakeError(ret, "Invalid port Id passed\n\0");
        return CL_OK;   /* CLI command itself succeeded but input
                         * is wrong. So return OK. */
    }

    iocAddress.iocPhyAddress.portId = (ClIocPortT) num;

    /* FIXME: Assuming credentials is some kind of integer */
    rc = _clGmsCliGetNumeric(argv[6], (ClUint64T*)&num);

    if (rc != CL_OK) 
    {
        _clGmsCliMakeError(ret, "Invalid credentials passed\n\0");
        return CL_OK;   /* CLI command itself succeeded but input
                         * is wrong. So return OK. */
    }

    credentials = (ClGmsLeadershipCredentialsT) num;

    rc = _clGmsCliGetNumeric(argv[6], (ClUint64T*)&num);

    if (rc != CL_OK) 
    {
        _clGmsCliMakeError(ret, "Invalid credentials passed\n\0");
        return CL_OK;   /* CLI command itself succeeded but input
                         * is wrong. So return OK. */
    }

    viewNumber = num;

    viewNode = (ClGmsViewNodeT *)clHeapAllocate(sizeof(ClGmsViewNodeT));

    if (!viewNode)  return CL_GMS_RC(CL_ERR_NO_MEMORY);

    memset(viewNode, 0 , sizeof(viewNode));

    if (groupId != CL_GMS_CLUSTER_ID)
    {
        viewNode->viewMember.groupMember.memberId = memberId;
        viewNode->viewMember.groupMember.initialViewNumber = 
                                                    viewNumber;
        viewNode->viewMember.groupMember.memberAddress = iocAddress;

        strncpy(viewNode->viewMember.groupMember.memberName.value,
                                    argv[3], strlen(argv[3]));
        viewNode->viewMember.groupMember.memberName.length = strlen(argv[3]);
        viewNode->viewMember.groupMember.credential = credentials;

        rc = _clGmsViewAddNode(groupId, memberId, viewNode);
        
    }
    else
    {
        viewNode->viewMember.clusterMember.nodeId = memberId;
        viewNode->viewMember.clusterMember.initialViewNumber = 
                                                        viewNumber;
        viewNode->viewMember.clusterMember.nodeAddress = iocAddress;

        strncpy(viewNode->viewMember.clusterMember.nodeName.value,
                                    argv[3], strlen(argv[3]));
        viewNode->viewMember.clusterMember.nodeName.length = strlen(argv[3]);
        viewNode->viewMember.clusterMember.credential = credentials;

        rc = _clGmsViewAddNode(groupId, memberId, viewNode);
    }

    return rc;
}

#define VIEW_NODE_DELETE_USAGE   \
        "Usage: view_delete_node <viewId> <nodeId>\n"\
         "\tThis CLI is used to delete the node with given nodeId from the"\
         "view database\n"\
         "\t<viewId> : View ID of the node\n"\
         "\t<nodeId> : Node ID of the node\n\r"
        
static ClRcT   gmsCliDelViewNode(
            CL_IN   ClUint32T argc, 
            CL_IN   ClCharT** argv, 
            CL_OUT  ClCharT** ret)
{ 
    ClRcT   rc = CL_OK;
    ClUint64T       num = 0;
    ClGmsGroupIdT   groupId = CL_GMS_INVALID_GROUP_ID;
    ClGmsMemberIdT  memberId = 0;

    

    if (argc != 3)
    {
        _clGmsCliMakeError(ret, VIEW_NODE_DELETE_USAGE);

        return CL_OK;
    }

    rc = _clGmsCliGetNumeric(argv[1], (void *)&num);
    
    if (rc != CL_OK) 
    {
        _clGmsCliMakeError(ret, "Invalid View Id passed\n\0");
        return CL_OK;   /* CLI command itself succeeded but input
                         * is wrong. So return OK. */
    }

    groupId = (ClGmsGroupIdT) num;

    rc = _clGmsCliGetNumeric(argv[2], (void *)&num);
    
    if (rc != CL_OK) 
    {
        _clGmsCliMakeError(ret, "Invalid Node Id passed\n\0");
        return CL_OK;   /* CLI command itself succeeded but input
                         * is wrong. So return OK. */
    }
   
    memberId = (ClGmsMemberIdT) num;

    rc = _clGmsViewDeleteNode(groupId, memberId);

    return rc;   
}

/* Sets the debug mode for GMS */


static ClRcT gmsCliSetDebug(
            CL_IN   ClUint32T argc, 
            CL_IN   ClCharT** argv, 
            CL_OUT  ClCharT** ret)
{
    ClRcT   rc = CL_OK;

    

    _clGmsCliMakeError(ret, "command not supported at this time.\n");

    return rc;
}

#endif 

/* Command List for GMS */

ClDebugFuncEntryT gmsCliCommandsList[GMS_TOTAL_CLI_COMMANDS] = {
    {
        .fpCallback =  gmsCliShowGmsInfo, 
        .funcName   =  "thisNodeInfo",
        .funcHelp   =  "View the information about local GMS node"
    },

    {
        .fpCallback =  gmsCliGetViewMemberList, 
        .funcName   =  "memberList", 
        .funcHelp   =  "List all the members in the cluster/given group."
    },

    {
        .fpCallback =  gmsCliGetTrackList, 
        .funcName   =  "trackMemberList", 
        .funcHelp   =  "List all the track members info on a cluster/given group"
    },

    {
        .fpCallback =  gmsCliGroupsInfoListGet,
        .funcName   =  "allGroupsInfo",
        .funcHelp   =  "List the metadata of all the Generic Groups"
    },

    {
        .fpCallback =  gmsCliGetGroupInfo,
        .funcName   =  "groupInfo",
        .funcHelp   =  "List all the metadata of a given generic group"
    },

    {
        .fpCallback =  gmsLeaderElect,
        .funcName   =  "leaderElect",
        .funcHelp   =  "Initiate an explicit leader election"
    }
    /* 
     * Disabling these CLIs as they are useful only while
     * debugging the gms database issues. But not for generic use.*/
#if 0
    {
        .fpCallback = gmsCliAddTrackNode,
        .funcName   = "track_add_node",
        .funcHelp   = "Adding tracking node to a view's tracking database"
    },

    {
        .fpCallback = gmsCliDelTrackNode,
        .funcName   = "track_delete_node",
        .funcHelp   = "Deleting tracking node from a view's tracking database"
    },

    {
        .fpCallback = gmsCliAddView,
        .funcName   = "view_add",
        .funcHelp   = "Add a view "
    },

    {
        .fpCallback = gmsCliAddViewNode,
        .funcName   = "view_add_node",
        .funcHelp   = "Adding view node"
    },

    {
        .fpCallback = gmsCliDelViewNode,
        .funcName   = "view_delete_node",
        .funcHelp   = "Delete view node (view_node_delete <viewId> <nodeId>)"
    },

    {
        .fpCallback =  gmsCliSetDebug, 
        .funcName   =  "debug",
        .funcHelp   =  "Enable(1)/Disable(0) debug messages"
    },
#endif 
};

ClDebugModEntryT clModTab = { "GMS", "gms", gmsCliCommandsList, "Group Membership Service" };
