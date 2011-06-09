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
 * File        : clGmsView.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * Contains the data structures and function prototypes for view database
 * handlers and view utilities.
 *****************************************************************************/

#ifndef _CL_GMS_VIEW_H_
#define _CL_GMS_VIEW_H_

# ifdef __cplusplus
extern "C" {
# endif

#include <clCommon.h>
#include <clGmsCommon.h>

/* Union of view types */

typedef struct {
    ClGmsClusterMemberT clusterMember;
    ClUint64T           contextHandle;   
    ClGmsGroupMemberT   groupMember;
    ClGmsGroupInfoT     groupData;  /* Used for group create and destroy */
} ClGmsViewMemberT;

/* View Node Info. */

typedef struct clGmsViewNode {
    ClGmsViewMemberT    viewMember; 
    ClUint32T           trackFlags;
    ClUint32T           flags;
} ClGmsViewNodeT;


/* View Structure */

typedef struct {

    ClNameT                     name;

    /* id = 0 represents the cluster */
    ClGmsGroupIdT               id;

    /* This Cluster/Group boot time */
    ClTimeT                     bootTime;

    /* Time this view was modified last */
    ClTimeT                     lastModifiedTime;


    /* View Number of this Cluster/Group. Provided by the
     * leader of the group when this cluster/group joins
     * the cluster/group */
    ClUint64T                   viewNumber;

    /* Set if atleast one node joins this
     * cluster or group */

    ClBoolT                     isActive;

    /* Storing the cluster/group leader and deputy
     * at the top of the cluster/group for easy access.
    */

    ClGmsNodeIdT                leader;
    ClGmsNodeIdT                deputy;

    ClBoolT                     leadershipChanged;

    /* Used to store the current view number change to
     * give to the newly joined nodes if this node is
     * a leader.
     */

    ClUint64T                   currentViewNumberChange;

    /* Number of Active members in this Cluster/Group */

    ClUint32T                   noOfViewMembers;


} ClGmsViewT;

/* structure to hold the values of all the group nodes
 * during SYNC operation
 */
typedef struct clGmsGroupSyncNotification {
    /*
     * Total number of groups in the system
     */
    ClUint32T           noOfGroups;
    /*
     * Groups Metadata list
     */
    ClGmsGroupInfoT     *groupInfoList;
    /*
     * Total number of group members in the system
     */
    ClUint32T           noOfMembers;
    /*
     * Group Members list
     */
    ClGmsViewNodeT      *groupMemberList;

} ClGmsGroupSyncNotificationT;


/* There is only one cluster in a system. Names are not passed from
 * the client when cluster is created. SO assume a name and id for it.
 */

#define CL_GMS_CLUSTER_NAME     "cluster0\0"

#define CL_GMS_CLUSTER_ID       0

#define CL_GMS_NANO_SEC         1000000000ULL


/* View compare enums */

typedef enum {
    CL_GMS_VIEW_EQUAL     = 0,
    CL_GMS_VIEW_NOT_EQUAL = 1
} ClGmsViewCompareT;


 
// created to check for matching group name
ClRcT _clGmsViewClusterGroupFind( CL_IN const char *name);


/* View Db Handlers. */

/* Creates a view and returns a handle which the caller has to use
 * for later access to the view.
 */

ClRcT      _clGmsViewCreate(
                    CL_IN   ClNameT         name,
                    CL_IN   ClGmsGroupIdT   groupId);


ClRcT      _clGmsViewAddNode(
                    CL_IN   ClGmsGroupIdT   groupId,
                    CL_IN   ClGmsNodeIdT    nodeId,
                    CL_IN   ClGmsViewNodeT  *node);

ClRcT      _clGmsViewAddNodeExtended(
                    CL_IN   ClGmsGroupIdT   groupId,
                    CL_IN   ClGmsNodeIdT    nodeId,
                    CL_IN   ClGmsViewNodeT  **node);

ClRcT      _clGmsViewDeleteNode(
                    CL_IN   ClGmsGroupIdT    groupId,
                    CL_IN   ClGmsNodeIdT    nodeId);

ClRcT      _clGmsViewDeleteNodeExtended(
                    CL_IN   ClGmsGroupIdT    groupId,
                    CL_IN   ClGmsNodeIdT    nodeId,
                    CL_IN   ClBoolT viewCache);

/* This API does not lock the node. */

ClRcT      _clGmsViewFindNode(
                    CL_IN   ClGmsGroupIdT   groupId,
                    CL_IN   ClGmsNodeIdT    nodeId,
                    CL_IN   ClGmsViewNodeT  **node);

/* Gets the node for the given node id from the database and 
 * locks the node so that no one else can modify it.
 */
ClRcT       _clGmsViewNodeLockAndGet(
                    CL_IN   ClGmsGroupIdT       groupId,
                    CL_IN   ClGmsNodeIdT        nodeId,
                    CL_IN   ClGmsViewNodeT      **node);

/* Puts back the node into the view database and unlock it */

ClRcT       _clGmsViewNodeUlockAndPut(
                    CL_IN   ClGmsGroupIdT       groupId,
                    CL_IN   ClGmsNodeIdT        nodeId,
                    CL_IN   ClGmsViewNodeT      *node);

/* This function does not lock the view */

ClRcT      _clGmsViewFind(
                    CL_IN   ClGmsGroupIdT    groupId, 
                    CL_IN   ClGmsViewT      **thisView);

/* This function gets the view and locks it */

ClRcT      _clGmsViewFindAndLock(
                    CL_IN   ClGmsGroupIdT    groupId, 
                    CL_IN   ClGmsViewT      **thisView);


ClRcT   _clGmsViewUnlock(
                CL_IN   ClGmsGroupIdT   groupId);

ClRcT      _clGmsViewDestroy(
                    CL_IN   ClGmsGroupIdT    groupId);


/*
 * Utility APIs called from clGmsApiHandler.c & clGmsCli.c.
 * This function takes a void ptr of Cluster or Group Track
 * response structure. Internally using the groupId it'll
 * figure out how to fill the structure.
 */

ClRcT   _clGmsViewGetCurrentView(
                    CL_IN   ClGmsGroupIdT               groupId,
                    CL_OUT  void                         *res);

/*
 * This does the same as above, but it already assumes that the view is
 * locked already.
 */
ClRcT   _clGmsViewGetCurrentViewLocked(
                    CL_IN   ClGmsGroupIdT               groupId,
                    CL_OUT  void                         *res);

/* Used by the CLI to get the list of views in a view in the form
   of a string to get printed */

ClRcT      _clGmsViewCliPrint(
                    CL_IN   ClGmsGroupIdT    groupId,
                    CL_IN   ClCharT         **ret);

/* Hash table callbacks - Internal functions */

ClUint32T  _clGmsViewHashCallback(
                        CL_IN   ClCntKeyHandleT userKey);

ClInt32T   _clGmsViewKeyCompareCallback(
                        CL_IN   ClCntKeyHandleT  key1,
                        CL_IN   ClCntKeyHandleT  key2);

void       _clGmsViewDeleteCallback(
                        CL_IN   ClCntKeyHandleT  userKey, 
                        CL_IN   ClCntDataHandleT data);

void       _clGmsViewDestroyCallback(
                        CL_IN   ClCntKeyHandleT  userKey, 
                        CL_IN   ClCntDataHandleT data);


/* Called during GMS Start */

void _clGmsViewDbCreate ();
ClRcT      _clGmsViewInitialize();

ClRcT      _clGmsViewFinalize(void);

#ifdef  __cplusplus
}
#endif
            
#endif /* _CL_GMS_VIEW_H_ */
