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
 * File        : clGmsDb.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

#ifndef _CL_GMS_DB_H_
#define _CL_GMS_DB_H_

# ifdef __cplusplus
extern "C"
{
# endif

#include <clCntApi.h>
#include <clGmsView.h>
#include <clGmsTrack.h>
#include <clOsalApi.h>
#include <clCksmApi.h>

/* For the hash table */

#define CL_GMS_MAX_NUM_OF_BUCKETS 50

/* Cluster & Group types. Used for only DB create function.
 */

typedef enum {

    CL_GMS_CLUSTER  = 0,
    CL_GMS_GROUP    = 1

} ClGmsDbViewTypeT;


/* Database to hold the view and tracking information.
 */

typedef struct {
 
    ClGmsViewT                  view;

    /* View type to indicate its a cluster or
     * a group */

    ClGmsDbViewTypeT            viewType;

    /* Holds the view and track database
     */
#define CL_GMS_MAX_DB   3

    ClCntHandleT                htbl[CL_GMS_MAX_DB];

    /* Mutex that should be locked whenever an operation
     * done on this Cluster/Group. Per member locking
     * will be implemented in the future to improve
     * performance. */

    ClOsalMutexIdT              viewMutex;
    ClOsalMutexIdT              trackMutex;

    /* Maintain the group information */
    ClGmsGroupInfoT                 groupInfo;

} ClGmsDbT;

/* Currently the cluster and group database are same. But later
 * when we move to a multi-cluster GMS then cluster and group
 * database has to be split.
 */

typedef enum {

    CL_GMS_CURRENT_VIEW      = 0,   /* Holds the current view */
    CL_GMS_TRACK             = 1,   /* Holds the track stuff */
    CL_GMS_JOIN_LEFT_VIEW    = 2,   /* Holds nodes that joined/left */
    CL_GMS_NAME_ID_DB        = 3    /* Holds the groupName - Id pair */

} ClGmsDbTypeT;

/* Hash table callback functions for handling keys, destroying and deleting
 * entires in the hash table.
 */

typedef struct {

    ClUint32T                   gmsNumOfBuckets;
    ClCntHashCallbackT          gmsHashCallback;
    ClCntKeyCompareCallbackT    gmsHashKeyCompareCallback;
    ClCntDeleteCallbackT        gmsHashDeleteCallback;
    ClCntDeleteCallbackT        gmsHashDestroyCallback;

} ClGmsDbHTbleParamsT;



typedef struct {

    ClGmsDbHTbleParamsT  htbleParams;

} ClGmsDbTrackParamsT;

typedef struct {

    ClGmsDbHTbleParamsT   htbleParams;
    ClGmsDbTrackParamsT   *trackDbParams;

} ClGmsDbViewParamsT;


/* View and Track  database has different keys */

typedef union {
    ClGmsTrackNodeKeyT     track;
    ClGmsNodeIdT           nodeId;
    ClNameT                name;
} ClGmsDbKeyT;

typedef struct ClGmsNodeIdPair {
    ClGmsGroupIdT          groupId;
    ClNameT                name;
} ClGmsNodeIdPairT;

/* Get the Db hash Key */

static inline ClRcT _clGmsDbGetKey(
             CL_IN      ClGmsDbTypeT    type,
             /* Suppressing coverity warning for pass by value with below comment */
             // coverity[pass_by_value]
             CL_IN      ClGmsDbKeyT     key, 
             CL_OUT     ClCntKeyHandleT *cntKey)
{
    ClRcT           rc = CL_OK;
    ClUint32T       cksmKey = 0;

    if (type == CL_GMS_TRACK)
    {
        *cntKey = (ClCntKeyHandleT)(ClWordT)(key.track.handle ^ 
                key.track.address.iocPhyAddress.portId);
    }
    else if (type == CL_GMS_NAME_ID_DB)
    {
        rc = clCksm32bitCompute((ClUint8T *)key.name.value, key.name.length,
                &cksmKey);
        if (rc == CL_OK)
        {
            *cntKey = (ClCntKeyHandleT)(ClWordT)cksmKey;
        }
    }
    else 
    {
        *cntKey = (ClCntKeyHandleT)(ClWordT)key.nodeId;
    }

    return rc;
}


/*
 * Initializes the view database which holds both
 * cluster/group and track information.
 */

ClRcT _clGmsDbOpen(
            CL_IN       ClUint64T   numOfGroups,
            CL_INOUT    ClGmsDbT    **gmsDb);


/*
 * Creates a hash table and returns the handle for the hash table in
 * db pointer. This function is called whenever there is a new group
 * is added to the view list.
 */

ClRcT _clGmsDbCreate(
           CL_IN        ClGmsDbT          *gmsDb,
           CL_OUT       ClGmsDbT          **gmsElement);


ClRcT   _clGmsDbCreateOnIndex(
        CL_IN        ClUint64T             cntIndex,
        CL_IN        ClGmsDbT*  const      gmsDb,
        CL_OUT       ClGmsDbT** const      gmsElement);
/*
 * FIXME:
 */

ClRcT  _clGmsDbAdd(
            CL_IN   const ClGmsDbT*  const gmsDb,
            CL_IN   const ClGmsDbTypeT     type,
            CL_IN   const ClGmsDbKeyT      key,
            CL_IN   const void*  const     data);

/* FIXME:
 */
ClRcT  _clGmsDbDelete(
           CL_IN    const ClGmsDbT*  const gmsDb,
           CL_IN    const ClGmsDbTypeT     type,
           CL_IN    const ClGmsDbKeyT      key);

/* 
 * FIXME:
 */

ClRcT _clGmsDbClose(
           CL_IN    ClGmsDbT* const  gmsDb,
           CL_IN    const ClUint64T   numOfGroups);

/*
 * FIXME:
 */

ClRcT _clGmsDbFind(
        CL_IN    const ClGmsDbT*  const  gmsDb,
        CL_IN    const ClGmsDbTypeT     type,
        CL_IN    const ClGmsDbKeyT      key,
        CL_INOUT void** const           data);


/*
 * FIXME:
 */
ClRcT  _clGmsDbGetFirst(
        CL_IN    const ClGmsDbT* const    gmsDb,
        CL_IN    const ClGmsDbTypeT       type,
        CL_OUT   ClCntNodeHandleT** const gmsOpaque,
        CL_INOUT void** const             data);


/* FIXME:
 */
ClRcT   _clGmsDbGetNext(
        CL_IN       const ClGmsDbT*    const   gmsDb,
        CL_IN       const ClGmsDbTypeT         type,
        CL_INOUT    ClCntNodeHandleT** const   gmsOpaque,
        CL_OUT      void**             const   data);


/* Deletes all the nodes in the hash table */

ClRcT  _clGmsDbDeleteAll(
            CL_IN   const ClGmsDbT* const  gmsDb,
            CL_IN   const ClGmsDbTypeT     type);


/* Deletes all the hashtables and frees the database */

ClRcT   _clGmsDbDestroy(
           CL_IN   ClGmsDbT* const  gmsDb);

/* Function to get the index of the database for a given group */
ClRcT   _clGmsDbIndexGet (
           CL_IN        ClGmsDbT*  const      gmsDb,
           CL_IN        ClGmsDbT**  const      gmsElement,
           CL_OUT       ClUint32T*            index);


/* FIXME: Move it to the right file */

ClRcT      _clGmsViewDbFind(
                    CL_IN   ClGmsGroupIdT   groupId,
                    CL_OUT  ClGmsDbT        **thisViewDb);


ClRcT      _clGmsTrackSendCBNotification(
                CL_IN  const  ClGmsDbT * const, 
                CL_IN  const  ClGmsTrackNotifyT * const,
                CL_IN  const  ClGmsTrackNodeT * const,
                CL_IN  const  ClGmsGroupIdT);

/* Internal function used by tracking code to get the view changes
   and changeonly list from the view */

ClRcT   _clGmsViewGetTrackAsync(
            CL_IN   const ClGmsDbT            *thisViewDb,
            CL_OUT  void                *changeList,
            CL_OUT  void                *changeOnlyList);

ClRcT   _clGmsViewFindNodePrivate(
        CL_IN   const ClGmsDbT*  const  thisViewDb,
        CL_IN   const ClGmsNodeIdT      nodeId,
        CL_IN   const ClGmsDbTypeT      type,
        CL_OUT  ClGmsViewNodeT** const  node);


ClRcT   _clGmsNameIdDbCreate (ClCntHandleT  *dbPtr);

ClRcT   _clGmsNameIdDbAdd (ClCntHandleT  *dbPtr,
                           ClNameT       *name,
                           ClGmsGroupIdT  id);

ClRcT   _clGmsNameIdDbDelete(ClCntHandleT  *dbPtr,
                             ClNameT       *name);

ClRcT   _clGmsNameIdDbDeleteAll(ClCntHandleT  *dbPtr);

ClRcT   _clGmsNameIdDbDestroy (ClCntHandleT  *dbPtr);

ClRcT   _clGmsNameIdDbFind(ClCntHandleT  *dbPtr,
                           ClNameT       *name,
                           ClGmsGroupIdT *id);

/* Function used to get the list of members of the given view. */
ClRcT
_clGmsViewGetCurrentViewNotification(
        CL_IN   const ClGmsDbT* const thisViewDb,
        CL_OUT  void**          const noti,
        CL_OUT  ClUint32T*      const noOfItems);

# ifdef __cplusplus
}
# endif

#endif /* _CL_GMS_DB_H_ */
