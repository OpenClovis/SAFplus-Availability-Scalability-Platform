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
 * File        : clGmsTrack.c
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *
 * This file implements server side GMS Track functions.
 * Track info are subset of a view and all are intialized as part of
 * Cluster/Group Intialization. 
 *
 *
 *
 *****************************************************************************/

#include <string.h>
#include <time.h>
#include <clCommonErrors.h>
#include <clBufferApi.h>
#include <clGms.h>
#include <clGmsView.h>
#include <clGmsTrack.h>
#include <clGmsCli.h>
#include <clGmsCommon.h>
#include <clGmsRmdServer.h>
#include <clCntErrors.h>
#include <clGmsErrors.h>
#include <clLogApi.h>


/* Informs all the clients in the db about the current state 
 * of the groups.No need to do a view lock as it is already locked
 * by _clGmsViewAddNode() which calls this function. Still need
 * to do a track lock.
 */

ClRcT  _clGmsTrackNotify(
                CL_IN   const ClGmsGroupIdT    groupId)
{
    ClRcT               rc = CL_OK;
    ClCntNodeHandleT    *gmsOpaque = NULL; 
    ClGmsTrackNodeT     *trackNode = NULL;
    ClGmsDbT            *thisViewDb = NULL;
    ClGmsTrackNotifyT   changeList = { NULL, 0};
    ClGmsTrackNotifyT   changeOnlyList = { NULL, 0};

    rc = _clGmsViewDbFind(groupId, &thisViewDb);

    if (rc != CL_OK)
            return rc;

    CL_ASSERT(thisViewDb != NULL);

    /* FIXME: need a track lock? */
     clGmsMutexLock(thisViewDb->trackMutex);

    rc = _clGmsViewGetTrackAsync(thisViewDb, &changeList, &changeOnlyList); 

    if (rc != CL_OK) goto TRACK_NOTIFY_ERROR;
    rc = _clGmsDbGetFirst(thisViewDb, CL_GMS_TRACK, &gmsOpaque, 
                                                (void **)&trackNode);

    if (rc != CL_OK) goto TRACK_NOTIFY_ERROR;

    while((rc == CL_OK) && (trackNode != NULL)) 
    {
        if (((trackNode->trackFlags & CL_GMS_TRACK_CHANGES) ==
                    CL_GMS_TRACK_CHANGES) && (changeList.entries > 0))
        {
          rc =  _clGmsTrackSendCBNotification(thisViewDb, &changeList,
                                                 trackNode,groupId);
        }
        else if ((trackNode->trackFlags & CL_GMS_TRACK_CHANGES_ONLY) == 
                    CL_GMS_TRACK_CHANGES_ONLY)
        {
          rc =  _clGmsTrackSendCBNotification(thisViewDb, &changeOnlyList, 
                                                 trackNode,groupId); 
        }

        if (rc != CL_OK 
            && 
            CL_GET_ERROR_CODE(rc) != CL_IOC_ERR_COMP_UNREACHABLE
            &&
            CL_GET_ERROR_CODE(rc) != CL_IOC_ERR_HOST_UNREACHABLE)
        {
            goto TRACK_NOTIFY_ERROR;
        }

        rc = _clGmsDbGetNext(thisViewDb, CL_GMS_TRACK, &gmsOpaque, 
                                                        (void **)&trackNode);
    }

    /* Reset the leadership change flag after the track callbacks */

    thisViewDb->view.leadershipChanged = CL_FALSE;

TRACK_NOTIFY_ERROR:

    clHeapFree(changeList.buffer);

    /* FIXME: Used realloc for this. So freeing it using free */
    free(changeOnlyList.buffer);
    
    clGmsMutexUnlock(thisViewDb->trackMutex);

    return rc;
}

/* Prints all the entries in the group.
 */

ClRcT   _clGmsTrackCliPrint(
                CL_IN   const ClGmsGroupIdT       groupId,
                CL_IN   ClCharT** const           ret)
{
    ClRcT                     rc = CL_OK;
    ClGmsTrackNodeT          *node = NULL;
    ClBufferHandleT           msg;
    ClCntNodeHandleT         *hdl = NULL;
    ClGmsDbT                 *thisViewDb=NULL;
    char                      timeBuffer[256] = "";
    ClUint32T                 len = 0;
    char                      trackFlags[100] = "";
    ClTimeT                   ti = {0};


    if (ret == NULL)
    {
        return CL_ERR_NULL_POINTER;
    }

    rc = clDebugPrintInitialize(&msg);

    if (rc != CL_OK)
    {
        return rc;
    }

    rc = _clGmsViewDbFind(groupId, &thisViewDb);

    if (rc != CL_OK)
    {
        return rc;
    }

    if (thisViewDb == NULL)
    {
        return CL_ERR_UNSPECIFIED;
    }

    clGmsMutexLock(thisViewDb->trackMutex);

    rc = clDebugPrint( msg , "-----------------------------------------------------------------------\n");
    rc = clDebugPrint(msg , "Cluster/Group Name : %s\n", 
            thisViewDb->view.name.value);
    if (rc != CL_OK)
    {
        goto ERROR_EXIT;
    }

    ti = thisViewDb->view.bootTime/CL_GMS_NANO_SEC;
    rc = clDebugPrint(msg , "Boot Timestamp     : %s", 
            ctime_r ((const time_t*)&ti, timeBuffer ));
    if (rc != CL_OK)
    {
        goto ERROR_EXIT;
    }

    rc = clDebugPrint( msg , "-----------------------------------------------------------------------\n");
    if (rc != CL_OK)
    {
        goto ERROR_EXIT;
    }

    rc = clDebugPrint( msg , "GMSHandle  IocAddress  IocPort  CkptDsId  TrackFlags\n");
    if (rc != CL_OK)
    {
        goto ERROR_EXIT;
    }
    rc = clDebugPrint( msg , "-----------------------------------------------------------------------\n");


    rc = _clGmsDbGetFirst(thisViewDb, CL_GMS_TRACK, &hdl, (void **)&node);

    while ((rc == CL_OK) && (node != NULL))
    {
        switch (node->trackFlags)
        {
            case 1: strncpy(trackFlags,"CL_GMS_TRACK_CURRENT",99);
                    break;
            case 2: strncpy(trackFlags,"CL_GMS_TRACK_CHANGES",99);
                    break;
            case 3: strncpy(trackFlags,"CL_GMS_TRACK_CURRENT | CL_GMS_TRACK_CHANGES",99);
                    break;
            case 4: strncpy(trackFlags,"CL_GMS_TRACK_CHANGES_ONLY",99);
                    break;
            case 5: strncpy(trackFlags,"CL_GMS_TRACK_CURRENT | CL_GMS_TRACK_CHANGES_ONLY",99);
                    break;
            case 6: strncpy(trackFlags,"CL_GMS_TRACK_CHANGES | CL_GMS_TRACK_CHANGES_ONLY",99);
                    break;
            case 7: strncpy(trackFlags,"CL_GMS_TRACK_CURRENT | CL_GMS_TRACK_CHANGES | CL_GMS_TRACK_CHANGES_ONLY",99);
                    break;
        }
        rc = clDebugPrint( msg , "%-9lld  0x%-8x  0x%-5x  %-8d  %s\n", 
                node->handle,
                node->address.iocPhyAddress.nodeAddress, 
                node->address.iocPhyAddress.portId,
                node->dsId,
                trackFlags);

        if (rc != CL_OK)
        {
            goto ERROR_EXIT;
        }

        rc = _clGmsDbGetNext(thisViewDb, CL_GMS_TRACK, &hdl, (void **)&node);
    }

    rc = clBufferLengthGet(msg, &len);
    if (rc != CL_OK)
    {
        goto ERROR_EXIT; 
    }

    *ret = (ClCharT *) clHeapAllocate(len);

    if (*ret == NULL) 
    {
        rc = CL_GMS_RC(CL_ERR_NULL_POINTER);
        goto ERROR_EXIT;
    }

ERROR_EXIT:

    clDebugPrintFinalize( &msg ,ret );
    clGmsMutexUnlock(thisViewDb->trackMutex);

    return rc;
}

/* Adds an entry to the track database.
 */

ClRcT   _clGmsTrackAddNode(
                CL_IN    const ClGmsGroupIdT           groupId,
                CL_IN    const ClGmsTrackNodeKeyT      key,
                CL_IN    ClGmsTrackNodeT* const        node,
				CL_INOUT ClUint32T*					   dsId)
{
    ClRcT               rc = CL_OK;
    ClGmsDbT            *thisViewDb = NULL; 
    ClGmsDbKeyT         dbKey = {{0}};
    ClGmsTrackNodeT     *foundNode=NULL;
	ClUint32T			oldDsId = 0;

    

    CL_ASSERT((node != NULL));

    rc = _clGmsViewDbFind(groupId, &thisViewDb);

    if (rc != CL_OK)
    {
        return rc;
    }

    if (thisViewDb == NULL)
    {
        return CL_ERR_UNSPECIFIED;
    }

    clGmsMutexLock(thisViewDb->trackMutex);

    rc = _clGmsTrackFindNode(groupId, key, &foundNode);

    if ((rc != CL_CNT_RC(CL_ERR_NOT_EXIST)) && (rc != CL_OK))
    {
         goto TRACK_ADD_ERROR;
    }
    
    if (foundNode != NULL)
    {
        clLog (WARN,GEN,NA,
                "Duplicate track request. Track Node already exists");
		/* In this case, we need to retrive
		 * the old checkpoint dsId and then update the node */
		oldDsId = foundNode->dsId;

		/* Now update the new node structure with old dsId */
		node->dsId = oldDsId;
        *foundNode  = *node;
		
		/* Return the caller with oldDsId */
		*dsId = oldDsId;

        /* Free the passed node. */
        clHeapFree((void*)node);
        rc = CL_OK;
        goto TRACK_ADD_ERROR;
    }

    dbKey.track = key;

    rc = _clGmsDbAdd(thisViewDb, CL_GMS_TRACK, dbKey, (void *)node);

TRACK_ADD_ERROR:
    
    clGmsMutexUnlock(thisViewDb->trackMutex);

    return rc; 
} 

/* Deletes an entry from the track database.
 */


ClRcT  _clGmsTrackDeleteNode(
                CL_IN   const ClGmsGroupIdT       groupId,
                CL_IN   const ClGmsTrackNodeKeyT  key)
{
    ClRcT       rc = CL_OK;
    ClGmsDbKeyT dbKey = {{0}};
    ClGmsDbT    *thisViewDb=NULL;

    

    rc = _clGmsViewDbFind(groupId, &thisViewDb);
    if (rc != CL_OK)
    {
        return rc;
    }

    if (thisViewDb == NULL)
    {
        return CL_ERR_UNSPECIFIED;
    }

    clGmsMutexLock(thisViewDb->trackMutex);

    dbKey.track = key;

    rc = _clGmsDbDelete(thisViewDb, CL_GMS_TRACK, dbKey);

    clGmsMutexUnlock(thisViewDb->trackMutex);

    return rc;
} 

/* 
 * Find the track node for the given key
 */

ClRcT   _clGmsTrackFindNode(
             CL_IN  const ClGmsGroupIdT        groupId,
             CL_IN  const ClGmsTrackNodeKeyT   key,
             CL_OUT ClGmsTrackNodeT** const    node)
{
    ClRcT       rc = CL_OK;
    ClGmsDbT    *thisViewDb = NULL;
    ClGmsDbKeyT dbKey = {{0}};

    
    CL_ASSERT(node != NULL);

    *node = NULL;

    rc = _clGmsViewDbFind(groupId, &thisViewDb);

    if (rc != CL_OK) return rc;
   
    dbKey.track = key;
 
    rc = _clGmsDbFind(thisViewDb, CL_GMS_TRACK, dbKey, (void **)node);
 
    return rc;
}



/* FIXME:
 */


ClUint32T  _clGmsTrackHashCallback(
                CL_IN   const ClCntKeyHandleT userKey)
{
    /* FIXME: do an exclusive OR hash key
     */
    return ((ClWordT)userKey%CL_GMS_MAX_NUM_OF_BUCKETS);
}

/*   FIXME:
 */


ClInt32T _clGmsTrackKeyCompareCallback(
                CL_IN    const    ClCntKeyHandleT key1,
                CL_IN    const    ClCntKeyHandleT key2)
{
    return ((ClWordT)key1 - (ClWordT)key2);
}



/*  FIXME:
 */


void _clGmsTrackDeleteCallback(
                CL_IN       ClCntKeyHandleT userKey,
                CL_IN   const     ClCntDataHandleT userData)
{

    clHeapFree((void *)userData);

   return;
}


/* FIXME:
 */

void _clGmsTrackDestroyCallback(
                CL_IN       ClCntKeyHandleT userKey,
                CL_IN   const     ClCntDataHandleT userData)
{
    clHeapFree((void *)userData);

    return;
}

/* Initializes the Track functionality.
 */

ClRcT _clGmsTrackIntialize(void)
{
    ClRcT    rc = CL_OK;

    return rc;
}


/* Closes everything related to the track functionality.
 */

ClRcT _clGmsTrackFinalize(void)
{
    ClRcT rc = CL_OK;

    return rc;
}


/* Sends the track notify out to the registered clients */

ClRcT _clGmsTrackSendCBNotification(
           CL_IN   const  ClGmsDbT*          const  thisViewDb, 
           CL_IN   const  ClGmsTrackNotifyT* const  buf,
           CL_IN   const  ClGmsTrackNodeT*   const  trackNode,
           CL_IN   const  ClGmsGroupIdT             groupId)
{
    ClRcT                           rc = CL_OK;
    ClIocAddressT                   iocAddr = {{0}};
    ClGmsGroupTrackCallbackDataT      groupResp = {0};
    ClGmsClusterTrackCallbackDataT    clusterResp = {0};

    CL_ASSERT((trackNode != NULL) && (thisViewDb != NULL) && (buf != NULL));

    iocAddr = trackNode->address;

    if (thisViewDb->viewType == CL_GMS_CLUSTER) 
    {
        clusterResp.rc                   = CL_OK;
        clusterResp.gmsHandle            = trackNode->handle;
        clusterResp.numberOfMembers      = thisViewDb->view.noOfViewMembers;
        clusterResp.buffer.viewNumber    = thisViewDb->view.viewNumber;
        clusterResp.buffer.leader        = thisViewDb->view.leader;
        clusterResp.buffer.deputy        = thisViewDb->view.deputy;
        clusterResp.buffer.leadershipChanged
                                         = thisViewDb->view.leadershipChanged;
        clusterResp.buffer.numberOfItems = buf->entries;
        clusterResp.buffer.notification  = (ClGmsClusterNotificationT*)
                                                    buf->buffer;

        rc = cl_gms_cluster_track_callback(iocAddr, &clusterResp);
    }
    else
    {
        groupResp.rc                   = CL_OK;
        groupResp.gmsHandle            = trackNode->handle;
        groupResp.groupId              = groupId;
        groupResp.numberOfMembers      = thisViewDb->view.noOfViewMembers;
        groupResp.buffer.viewNumber    = thisViewDb->view.viewNumber;
        groupResp.buffer.leader        = thisViewDb->view.leader;
        groupResp.buffer.deputy        = thisViewDb->view.deputy;
        clusterResp.buffer.leadershipChanged 
                                       = thisViewDb->view.leadershipChanged;
        groupResp.buffer.numberOfItems = buf->entries;

        groupResp.buffer.notification  = (ClGmsGroupNotificationT*)
                                                    buf->buffer;

        rc = cl_gms_group_track_callback(iocAddr, &groupResp);
    }
    return rc;
}

