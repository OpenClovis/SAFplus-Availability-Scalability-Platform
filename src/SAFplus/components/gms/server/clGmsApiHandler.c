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
 * File        : clGmsApiHandler.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file contains the implementation of API functions extended by the
 * server to the clients via the GMS client library.  They are all called with
 * unmarshalled data structures obtained from the ILD/RMD layer and they
 * all return with result data structures to that IDL/RMD layer which the
 * latter should marshall before sending to the client side.
 *****************************************************************************/

#include <stdio.h>
#include <string.h>

#include <clCommon.h>
#include <clDebugApi.h>

#include <clClmApi.h>
#include <clTmsApi.h>
#include <clLogApi.h>
#include <clIocIpi.h>
#include <clCntErrors.h>
#include "clGms.h"
#include "clGmsMsg.h"
#include "clGmsRmdServer.h"
#include "clGmsEngine.h"
#include "clGmsErrors.h"
#include "clGmsApiHandler.h"
#include <clEventApi.h>
#include <clCpmExtApi.h>
#include <clGmsCkpt.h>

  
/******************************************************************************
 * Global Variables
 *****************************************************************************/
ClBoolT  ringVersionCheckPassed = CL_TRUE;
static ClTimerTimeOutT opTimeout = { 2 , 0 };
extern ClGmsCkptMetaDataT  gmsCkptMetaData;

static ClInt8T inline 
_clGmsIsReadyToServe(void)
{
    return ((ClInt8T)(gmsGlobalInfo.opState == CL_GMS_STATE_RUNNING));
}


/******************************************************************************
 * API FUNCTION DEFINITIONS
 *****************************************************************************/

/*-----------------------------------------------------------------------------
 * Client Library Intializer.
 *---------------------------------------------------------------------------*/
ClRcT 
clGmsClientLibInitHandler( 
        CL_IN  const ClGmsClientInitRequestT*   const req,
        CL_OUT       ClGmsClientInitResponseT*  const res)
{
    ClRcT rc = CL_OK;
    
    if ((req == (const void*)NULL) || (res == NULL))
    {
        return CL_ERR_NULL_POINTER;
    }

    /* if the server is not in a servicable state then ask the client to retry
     *  again after some time */
    if(0 == (ClInt32T)_clGmsIsReadyToServe())
    {
        rc = CL_GMS_RC(CL_ERR_TRY_AGAIN);
        return rc;
    }
    
    CL_GMS_VERIFY_CLIENT_VERSION( req , res );

    return rc;
}


/*-----------------------------------------------------------------------------
 * Cluster Track API
 *---------------------------------------------------------------------------*/
ClRcT
clGmsClusterTrackHandler(
   CL_IN  const ClGmsClusterTrackRequestT*      const req,
   CL_OUT       ClGmsClusterTrackResponseT*     const res)
{
    ClRcT                           rc            = CL_OK;
    ClGmsClusterTrackCallbackDataT  callback_data = {0};

    if ((req == (const void*)NULL) || (res == NULL))
    {
        return CL_ERR_NULL_POINTER;
    }

    /*
     * CL_GMS_TRACK_CHANGES and CL_GMS_TRACK_CHANGES_ONLY are mutually
     * exclusive; the cleint should have caught this error
     */
    CL_ASSERT (!(((req->trackFlags & CL_GMS_TRACK_CHANGES) != 0) &&
                ((req->trackFlags & CL_GMS_TRACK_CHANGES_ONLY) != 0)));


    CL_GMS_VERIFY_CLIENT_VERSION( req , res );

    /* if the server is not in a servicable state then ask the client to retry
     *  again after some time */
    if(0 == (ClInt32T)_clGmsIsReadyToServe()){
        res->rc = CL_GMS_RC(CL_ERR_TRY_AGAIN);
        return rc;
    }

    if (req->trackFlags & CL_GMS_TRACK_CURRENT)
    {
        ClGmsViewT *thisView = NULL;
        /*
         * We need to trigger an immediate response.  If sync is TRUE,
         * the response must be in the returned res struct; otherwise it
         * must go out in the form of a call to the clients trackCallback().
         */
        
        if (req->sync == CL_TRUE)
        {
            clLog(DBG,CLM,NA,
                    "Received sync TRACK_CURRENT request from [%d:%d]",
                    req->address.iocPhyAddress.nodeAddress,
                    req->address.iocPhyAddress.portId);

            _clGmsViewFindAndLock(0, &thisView);
            
            if(!bootTimeElectionDone)
            {
                _clGmsViewUnlock(0);
                rc = CL_GMS_RC(CL_ERR_TRY_AGAIN);
                goto error_exit;
            }
            
            /* Need to send it right here as our response */
            rc = _clGmsViewGetCurrentViewLocked((ClGmsGroupIdT)0, /* means cluster */
                                                (void*)&(res->buffer));
            _clGmsViewUnlock(0);
            if (rc != CL_OK)
            {
                goto error_exit;
            }
        }
        else
        {
            clLog(DBG,CLM,NA,
                    "Received async TRACK_CURRENT Request from [%d:%d]",
                    req->address.iocPhyAddress.nodeAddress,
                    req->address.iocPhyAddress.portId);
            /* Need to trigger a TrackCallback(); we can send it right now */
            _clGmsViewFindAndLock(0, &thisView);
            if(!bootTimeElectionDone)
            {
                _clGmsViewUnlock(0);
                goto track_changes;
            }
            rc = _clGmsViewGetCurrentViewLocked((ClGmsGroupIdT)0, /* means cluster */
                                                (void*)&(callback_data.buffer));
            _clGmsViewUnlock(0);
            if (rc != CL_OK)
            {
                goto error_exit;
            }
            
            callback_data.gmsHandle = req->gmsHandle;
            callback_data.numberOfMembers = callback_data.buffer.numberOfItems;
            callback_data.rc = rc;

            rc = cl_gms_cluster_track_callback(req->address, &callback_data);
            if (rc != CL_OK)
            {
                if (callback_data.buffer.notification != NULL)
                {
                    clHeapFree((void*)callback_data.buffer.notification);
                }
                goto error_exit;
            }
        }
    }

    track_changes:    
    if (req->trackFlags & (CL_GMS_TRACK_CHANGES | CL_GMS_TRACK_CHANGES_ONLY))
    {
		/*TODO we need a mutex here to protect the ckptMetaData counters */
        /* Need to register with track database */
        ClGmsTrackNodeKeyT  track_key = {0};
        ClGmsTrackNodeT    *subscriber = NULL;
		ClUint32T			dsId = 0;
		ClGmsTrackCkptDataT ckptTrackData = {0};

        clLogMultiline(DBG,CLM,NA,
                "Received track request with %s track flag from [%d:%d]",
                req->trackFlags & CL_GMS_TRACK_CHANGES ? 
                "CL_GMS_TRACK_CHANGES" : "CL_GMS_TRACK_CHANGES_ONLY",
                req->address.iocPhyAddress.nodeAddress,
                req->address.iocPhyAddress.portId);

        track_key.handle = req->gmsHandle;
        track_key.address = req->address;
        
        subscriber = (ClGmsTrackNodeT*)clHeapAllocate(sizeof(ClGmsTrackNodeT));
        if (subscriber == NULL)
        {
            clLog(ERROR,CLM,NA,
                    "Memory allocation failed while adding a track node");
            goto error_exit;
        }

        subscriber->handle     = req->gmsHandle;
        subscriber->address    = req->address;
        subscriber->trackFlags = req->trackFlags;
		dsId = gmsCkptMetaData.perGroupInfo[0];
		subscriber->dsId	   = dsId;

        rc = _clGmsTrackAddNode(0, track_key, subscriber,&dsId);
        if (rc != CL_OK)
        {
            clHeapFree((void*)subscriber);
            goto error_exit;
        }

		/* If this was a new request and the node was not
		 * already in the track DB, then we need to increment the 
		 * dsId counter */
		if (dsId == gmsCkptMetaData.perGroupInfo[0])
		{
			gmsCkptMetaData.perGroupInfo[0]++;
		}

		/* Checkpoint this track information */
		ckptTrackData.groupId = 0;
		ckptTrackData.gmsHandle = req->gmsHandle;
		ckptTrackData.trackFlag = req->trackFlags;
		ckptTrackData.iocAddress = req->address;

		rc = clGmsCheckpointTrackData(&ckptTrackData,dsId);
		if (rc != CL_OK)
		{
			clLog(ERROR,GEN,NA,
					"Failed to checkpoint the track data. rc [0x%x]",rc);
			goto error_exit;
		}
    }

error_exit:    
    clLog(DBG,GEN,NA,
            "Cluster track request completed. Rc 0x%x",rc);
    res->rc = rc;
    
    return rc; /* FIXME: I am not sure we need both res->rc and rc.  Probably
                * the latter can be left as CL_OK.  I suspect the non-zero
                * code may prevent the response reaching the other side.
                */
}

/*-----------------------------------------------------------------------------
 * Cluster Track Stop API
 *---------------------------------------------------------------------------*/
ClRcT
clGmsClusterTrackStopHandler(
    CL_IN  const ClGmsClusterTrackStopRequestT*  const req,
    CL_OUT       ClGmsClusterTrackStopResponseT* const res)
{
    ClRcT		        rc         = CL_OK;
    ClGmsTrackNodeKeyT  track_key  = {0};
    ClGmsTrackNodeT     *foundNode = NULL;
    ClUint32T           oldDsId    = 0;

    if ((req == (const void*)NULL) || (res == NULL))
    {
        rc = CL_ERR_NULL_POINTER;
        goto error_return;
    }

    CL_GMS_VERIFY_CLIENT_VERSION( req , res );

    /* if the server is not in a servicable state then ask the client to retry
     *  again after some time */
    if (0 == (ClInt32T)_clGmsIsReadyToServe())
    {
        rc = CL_GMS_RC(CL_ERR_TRY_AGAIN);
        goto error_return;
    }

    clLog(DBG,CLM,NA,
            "Received a cluster track stop request from [%d:%d]",
            req->address.iocPhyAddress.nodeAddress,
            req->address.iocPhyAddress.portId);

    /* Need to call the track database to cancel tracking for this client */
    track_key.handle = req->gmsHandle;
    track_key.address = req->address;

    /* Check if the node exists */
    rc = _clGmsTrackFindNode(0, track_key, &foundNode);

    if ((rc != CL_CNT_RC(CL_ERR_NOT_EXIST)) && (rc != CL_OK))
    {
        clLog(ERROR,CLM,NA,
                "Failed to find the track node. rc = [0x%x]",rc);
        goto error_return;
    }

    if (rc == CL_CNT_RC(CL_ERR_NOT_EXIST))
    {
        clLogMultiline(ERROR,CLM,NA,
                "clGmsClusterTrackStop requested without a prior "
                "clGmsClusterTrack() request.");
        goto error_return;
    }
    
    /* Retrieve the dsId */
    oldDsId = foundNode->dsId;

    rc = _clGmsTrackDeleteNode((ClGmsGroupIdT)0, track_key);

    if (rc != CL_OK)
    {
        clLog(ERROR,CLM,NA,
                "Failed to delete the track node. rc = [0x%x]",rc);
        goto error_return;
    }

    rc = clGmsCheckpointTrackDataDelete(0,oldDsId);
    if (rc != CL_OK)
    {
        clLog(ERROR,CKP,NA,
                "Failed to delete the track checkpoint dataset. rc = [0x%x]",rc);
        goto error_return;
    }

error_return:
    res->rc = rc;

    return rc; /* FIXME: I am not sure we need both res->rc and rc.  Probably
                * the latter can be left as CL_OK.  I suspect the non-zero
                * code may prevent the response reaching the other side.
                */
}

/*-----------------------------------------------------------------------------
 * Cluster Member Get API
 *---------------------------------------------------------------------------*/
ClRcT
clGmsClusterMemberGetHandler(
    CL_IN  const ClGmsClusterMemberGetRequestT*  const req,
    CL_OUT       ClGmsClusterMemberGetResponseT* const res)
{
    ClRcT		        rc            = CL_OK;
    ClGmsViewNodeT     *node          = NULL;
    ClCharT             strNodeId[20] = "";
    
    if ((req == (const void*)NULL) || (res == NULL))
    {
        return CL_ERR_NULL_POINTER;
    }

    CL_GMS_VERIFY_CLIENT_VERSION( req , res );

    snprintf(strNodeId,20,"%d",req->nodeId);
    clLogMultiline(DBG,CLM,NA,
            "Received a cluster member get request for [nodeid = %s]",
            req->nodeId == CL_GMS_LOCAL_NODE_ID ? strNodeId:"CL_GMS_LOCAL_NODE_ID");

    /* if the server is not in a servicable state then ask the client to retry
     *  again after some time */
    if(0 == (ClInt32T)_clGmsIsReadyToServe())
    {
        res->rc = CL_GMS_RC(CL_ERR_TRY_AGAIN);
        return rc;
    }

    /* Need to call the view database to get the member information */
    rc = _clGmsViewFindNode(0, 
            (req->nodeId == CL_GMS_LOCAL_NODE_ID) ?
            gmsGlobalInfo.config.thisNodeInfo.nodeId : req->nodeId, 
            &node);

    if (rc != CL_OK)
    {
        /* If the requested node is not found, we need to return
         * ERR_INVALID_PARAM as per SAF guideline */
        if ((CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST) ||
                (CL_GET_ERROR_CODE(rc) == CL_ERR_DOESNT_EXIST)) {
            rc = CL_GMS_RC(CL_ERR_INVALID_PARAMETER);
        }
        goto error_exit;
    }
    
    if (node == NULL)
    {
        clLog(ERROR,GEN,NA,
                "node value is NULL even when rc is OK");
        goto error_exit;
    }

    memcpy((void*)&res->member, (void*)&node->viewMember.clusterMember,
           sizeof(ClGmsClusterMemberT));

error_exit:
    res->rc = rc;
    
    return rc; /* FIXME: I am not sure we need both res->rc and rc.  Probably
                * the latter can be left as CL_OK.  I suspect the non-zero
                * code may prevent the response reaching the other side.
                */
}

/*-----------------------------------------------------------------------------
 * Cluster Member Get Async API
 *---------------------------------------------------------------------------*/
ClRcT
clGmsClusterMemberGetAsyncHandler(
    CL_IN  const ClGmsClusterMemberGetAsyncRequestT*  const req,
    CL_OUT       ClGmsClusterMemberGetAsyncResponseT* const res)
{
    ClRcT		                         rc            = CL_OK;
    ClGmsViewNodeT                      *node          = NULL;
    ClGmsClusterMemberGetCallbackDataT   callback_data = {0};
    ClCharT                              strNodeId[20] = "";
  
    if ((req == (const void*)NULL) || (res == NULL))
    {
        return CL_ERR_NULL_POINTER;
    }

    CL_GMS_VERIFY_CLIENT_VERSION( req , res );
    /* if the server is not in a servicable state then ask the client to retry
     *  again after some time */
    if(0 == (ClInt32T)_clGmsIsReadyToServe())
    {
        res->rc = CL_GMS_RC(CL_ERR_TRY_AGAIN);
        return rc;
    }

    snprintf(strNodeId,20,"%d",req->nodeId);
    clLogMultiline(DBG,CLM,NA,
            "Received a async request for cluster member get "
            "from [%d:%d] for [nodeid = %s]",
            req->address.iocPhyAddress.nodeAddress,
            req->address.iocPhyAddress.portId,
            req->nodeId == CL_GMS_LOCAL_NODE_ID ? strNodeId:"CL_GMS_LOCAL_NODE_ID");

    /* Need to call the view database to get the member information */
    rc = _clGmsViewFindNode(0, 
            (req->nodeId == CL_GMS_LOCAL_NODE_ID) ?
            gmsGlobalInfo.config.thisNodeInfo.nodeId : req->nodeId, 
            &node);

    if ((CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST) ||
            (CL_GET_ERROR_CODE(rc) == CL_ERR_DOESNT_EXIST)) 
    {
        rc = CL_GMS_RC(CL_ERR_INVALID_PARAMETER);
    }

    /* if we could not find node, we still need to call callback and say so */
    /* Initiate callback with member info */
    callback_data.rc = rc;          /* indicates if we found the data or not */
    callback_data.gmsHandle = req->gmsHandle;
    callback_data.invocation = req->invocation;
    if (node != NULL)
    {
        memcpy((void*)&callback_data.member, (void*)&node->viewMember.clusterMember,
           sizeof(ClGmsClusterMemberT));
    }
    
    rc = cl_gms_cluster_member_get_callback(req->address, &callback_data);

    res->rc = rc;
    
    return rc; /* FIXME: I am not sure we need both res->rc and rc.  Probably
                * the latter can be left as CL_OK.  I suspect the non-zero
                * code may prevent the response reaching the other side.
                */
}



/*-----------------------------------------------------------------------------
 * Group Track API
 *---------------------------------------------------------------------------*/
ClRcT
clGmsGroupTrackHandler(
    CL_IN   ClGmsGroupTrackRequestT*  req,
    CL_OUT  ClGmsGroupTrackResponseT* res)
{
    ClRcT                           rc            = CL_OK;
    ClGmsGroupTrackCallbackDataT    callback_data = {0};

    if ((req == (const void*)NULL) || (res == NULL))
    {
        return CL_ERR_NULL_POINTER;
    }

    /*
     * CL_GMS_TRACK_CHANGES and CL_GMS_TRACK_CHANGES_ONLY are mutually
     * exclusive; the cleint should have caught this error
     */
    CL_ASSERT (!(((req->trackFlags & CL_GMS_TRACK_CHANGES) != 0) &&
                ((req->trackFlags & CL_GMS_TRACK_CHANGES_ONLY) != 0)));


    CL_GMS_VERIFY_CLIENT_VERSION( req , res );

    /* if the server is not in a servicable state then ask the client to retry
     *  again after some time */
    if (readyToServeGroups != CL_TRUE)
    {
        res->rc = CL_ERR_TRY_AGAIN;
        return CL_ERR_TRY_AGAIN;
    }


    if (req->trackFlags & CL_GMS_TRACK_CURRENT)
    {
        /*
         * We need to trigger an immediate response.  If sync is TRUE,
         * the response must be in the returned res struct; otherwise it
         * must go out in the form of a call to the clients trackCallback().
         */
        
        if (req->sync == CL_TRUE)
        {
            clLogMultiline(DBG,GROUPS,NA,
                    "Received a sync Group Track CURRENT request "
                    "from [%d:%d] for [groupId = %d]",
                    req->address.iocPhyAddress.nodeAddress,
                    req->address.iocPhyAddress.portId,
                    req->groupId);
            /* Need to send it right here as our response */
            rc = _clGmsViewGetCurrentView(req->groupId, /* means cluster */
                                          (void*)&(res->buffer));
            if (rc != CL_OK)
            {
                goto error_exit;
            }
        }
        else
        {
            clLogMultiline(DBG,GROUPS,NA,
                    "Received an Async Group Track CURRENT request "
                    "from [%d:%d] for [groupId = %d]",
                    req->address.iocPhyAddress.nodeAddress,
                    req->address.iocPhyAddress.portId,
                    req->groupId);

            /* Need to trigger a TrackCallback(); we can send it right now */
            rc = _clGmsViewGetCurrentView(req->groupId, /* means cluster */
                                          (void*)&(callback_data.buffer));
            if (rc != CL_OK)
            {
                goto error_exit;
            }
            
            callback_data.gmsHandle = req->gmsHandle;
            callback_data.groupId = req->groupId;
            callback_data.numberOfMembers = callback_data.buffer.numberOfItems;
            callback_data.rc = rc;

            rc = cl_gms_group_track_callback(req->address, &callback_data);
            if (rc != CL_OK)
            {
                if (callback_data.buffer.notification != NULL)
                {
                    clHeapFree((void*)callback_data.buffer.notification);
                }
                goto error_exit;
            }
        }
    }
    
    if (req->trackFlags & (CL_GMS_TRACK_CHANGES | CL_GMS_TRACK_CHANGES_ONLY))
    {
        clLogMultiline(DBG,GROUPS,NA,
                "Received Group Track request with %s track flag "
                "from [%d:%d] for [groupId = %d]",
                req->trackFlags & CL_GMS_TRACK_CHANGES ? 
                "CL_GMS_TRACK_CHANGES" : "CL_GMS_TRACK_CHANGES_ONLY",
                req->address.iocPhyAddress.nodeAddress,
                req->address.iocPhyAddress.portId,
                req->groupId);

		/*TODO we need a mutex here to protect the ckptMetaData counters */
        /* Need to register with track database */
        ClGmsTrackNodeKeyT  track_key = {0};
        ClGmsTrackNodeT    *subscriber = NULL;
		ClUint32T			dsId = 0;
		ClGmsTrackCkptDataT ckptTrackData = {0};

        track_key.handle = req->gmsHandle;
        track_key.address = req->address;
        
        subscriber = (ClGmsTrackNodeT*)clHeapAllocate(sizeof(ClGmsTrackNodeT));
        if (subscriber == NULL)
        {
            goto error_exit;
        }

	if (req->groupId >= gmsGlobalInfo.config.noOfGroups)
	{
		rc = CL_ERR_NOT_EXIST;
		goto error_exit;
	}

        subscriber->handle     = req->gmsHandle;
        subscriber->address    = req->address;
        subscriber->trackFlags = req->trackFlags;
	dsId = gmsCkptMetaData.perGroupInfo[req->groupId];
	subscriber->dsId	   = dsId;

        rc = _clGmsTrackAddNode(req->groupId, track_key, subscriber,&dsId);
        if (rc != CL_OK)
        {
            clHeapFree((void*)subscriber);
            goto error_exit;
        }

		/* If this was a new request and the node was not
		 * already in the track DB, then we need to increment the 
		 * dsId counter */
		if (dsId == gmsCkptMetaData.perGroupInfo[req->groupId])
		{
			gmsCkptMetaData.perGroupInfo[req->groupId]++;
		}

		/* Checkpoint this track information */
		ckptTrackData.groupId = req->groupId;
		ckptTrackData.gmsHandle = req->gmsHandle;
		ckptTrackData.trackFlag = req->trackFlags;
		ckptTrackData.iocAddress = req->address;

		rc = clGmsCheckpointTrackData(&ckptTrackData,dsId);
		if (rc != CL_OK)
		{
			clLog(ERROR,GROUPS,NA,
					"Failed to checkpoint the track data. rc [0x%x]",rc);
			goto error_exit;
		}
    }

error_exit:    
    res->rc = rc;
    
    return rc; /* FIXME: I am not sure we need both res->rc and rc.  Probably
                * the latter can be left as CL_OK.  I suspect the non-zero
                * code may prevent the response reaching the other side.
                */
}


/*-----------------------------------------------------------------------------
 * Group Track Stop API
 *---------------------------------------------------------------------------*/
ClRcT
clGmsGroupTrackStopHandler(
    CL_IN   ClGmsGroupTrackStopRequestT*  req,
    CL_OUT  ClGmsGroupTrackStopResponseT* res)
{
    ClRcT		            rc        = CL_OK;
    ClGmsTrackNodeKeyT      track_key = {0};
    ClGmsTrackNodeT        *foundNode = NULL;
    ClUint32T               oldDsId   = 0;

    if ((req == (const void*)NULL) || (res == NULL))
    {
        rc = CL_ERR_NULL_POINTER;
        goto error_return;
    }

    CL_GMS_VERIFY_CLIENT_VERSION( req , res );

    clLogMultiline(DBG,GROUPS,NA,
            "Received Group Track Stop from [%d:%d] for [groupId = %d]",
            req->address.iocPhyAddress.nodeAddress,
            req->address.iocPhyAddress.portId,
            req->groupId);
    /* if the server is not in a servicable state then ask the client to retry
     *  again after some time */
    if (readyToServeGroups != CL_TRUE)
    {
        res->rc = CL_ERR_TRY_AGAIN;
        return CL_ERR_TRY_AGAIN;
    }

    /* Need to call the track database to cancel tracking for this client */
    track_key.handle = req->gmsHandle;
    track_key.address = req->address;

    /* Check if the node exists */
    rc = _clGmsTrackFindNode(req->groupId, track_key, &foundNode);

    if ((rc != CL_CNT_RC(CL_ERR_NOT_EXIST)) && (rc != CL_OK))
    {
        clLog(ERROR,GROUPS,NA,
                "Failed to find the track node. rc = [0x%x]",rc);
        goto error_return;
    }

    if (rc == CL_CNT_RC(CL_ERR_NOT_EXIST))
    {
        clLogMultiline(ERROR,GROUPS,NA,
                "clGmsGroupTrackStop requested without a prior "
                "clGmsGroupTrack() request.");
        goto error_return;
    }
    
    /* Retrieve the dsId */
    oldDsId = foundNode->dsId;

    rc = _clGmsTrackDeleteNode(req->groupId, track_key);

    if (rc != CL_OK)
    {
        clLog(ERROR,GROUPS,NA,
                "Failed to delete the track node. rc = [0x%x]",rc);
        goto error_return;
    }

    rc = clGmsCheckpointTrackDataDelete(req->groupId,oldDsId);
    if (rc != CL_OK)
    {
        clLog(ERROR,GROUPS,NA,
                "Failed to delete the track checkpoint dataset. rc = [0x%x]",rc);
        goto error_return;
    }

error_return:
    res->rc = rc;

    return rc; /* FIXME: I am not sure we need both res->rc and rc.  Probably
                * the latter can be left as CL_OK.  I suspect the non-zero
                * code may prevent the response reaching the other side.
                */
}


/*-----------------------------------------------------------------------------
 * Group Member Get API
 *---------------------------------------------------------------------------*/
ClRcT
clGmsGroupMemberGetHandler(
    CL_IN   ClGmsGroupMemberGetRequestT    *req,
    CL_OUT  ClGmsGroupMemberGetResponseT   *res)
{
    clLog(WARN,CLM,NA,
            "clGmsGroupMemberGetHandler API is not available");
    return CL_ERR_NOT_IMPLEMENTED;
}


/*-----------------------------------------------------------------------------
 * Group Member Get Async API
 *---------------------------------------------------------------------------*/
ClRcT
clGmsGroupMemberGetAsyncHandler(
    CL_IN   ClGmsGroupMemberGetAsyncRequestT    *req,
    CL_OUT  ClGmsGroupMemberGetAsyncResponseT   *res)
{
    clLog(WARN,CLM,NA,
            "clGmsGroupMemberGetAsyncHandler API is not available");
    return CL_ERR_NOT_IMPLEMENTED;
}


/*-----------------------------------------------------------------------------
 * Cluster Join API
 *---------------------------------------------------------------------------*/
static ClGmsEjectSubscribeT ejectCallback;

ClRcT
clGmsClusterJoinHandler(
    CL_IN  const ClGmsClusterJoinRequestT*  const   req,
    CL_OUT       ClGmsClusterJoinResponseT* const   res)
{
    ClRcT		              rc          = CL_OK;
    ClGmsClusterMemberT       thisNode    = {0};
    ClGmsViewMemberT          viewMember  = {{0}};
    const ClTimerTimeOutT     joinTimeOut = { 2 , 0 };
    ClGmsViewT               *clusterView = NULL;

    if ((req == (const void*)NULL) || (res == NULL))
    {
        return CL_ERR_NULL_POINTER;
    }

    CL_GMS_VERIFY_CLIENT_VERSION( req , res );

    /* if the server is not in a servicable state then ask the client to retry
     *  again after some time */
    if (0 == (ClInt32T)_clGmsIsReadyToServe())
    {
        res->rc = CL_GMS_RC(CL_ERR_TRY_AGAIN);
        return rc;
    }

    clLogMultiline(DBG,CLM,NA,
            "Received Cluster join request from [%d:%d] for [nodeId = %d]",
            req->address.iocPhyAddress.nodeAddress,
            req->address.iocPhyAddress.portId,
            req->nodeId);

    _clGmsSetThisNodeInfo(req->nodeId, &req->nodeName, req->credentials);

    _clGmsGetThisNodeInfo(&thisNode);

    memcpy(&viewMember.clusterMember, &thisNode, sizeof(ClGmsClusterMemberT));

    rc= clGmsSendMsg(&viewMember, req->groupId, CL_GMS_CLUSTER_JOIN_MSG , 0x0, 0, NULL );

    if (rc != CL_OK)
    {
        clLog(ERROR,CLM,NA, 
                "Cluster Join Multicast messaging through openais failed.");
        res->rc = rc;
        goto JOIN_FAILED;
    } 

    ejectCallback.address.iocPhyAddress = req->address.iocPhyAddress;
    ejectCallback.handle                = req->gmsHandle;


    /*
       Enter the critical section and wait for the openais thread to signal
       the critical section until any foreign join message is recieved . 

       if Critical section returns ok 

           if version check passed 
               then return CL_OK to the client 
           else 
               delete the self node from the view database. 
               return CL_ERR_VERSION_MISMATCH to the client. 
       else

           no foriegn join recieved in the wait time period so we are the
           first node in the cluster .
     */



    rc = clGmsCsEnter ( &joinCs , joinTimeOut ) ;
   
    clLog(DBG,CLM,NA,"Unblocking join thread from critical section. rc 0x%x",rc);
    /* copy the current ring version to the join response to inform to
     *  the client */
     res->serverVersion.releaseCode = ringVersion.releaseCode;
     res->serverVersion.minorVersion= ringVersion.minorVersion;
     res->serverVersion.majorVersion= ringVersion.majorVersion;

    if( rc == CL_OK ){
        if( ringVersionCheckPassed ){
            clLog(INFO,CLM,NA,"Returning from cluster join API handler");
            return rc;
        }
        else {
            ringVersionCheckPassed = CL_TRUE;
            if (_clGmsViewFindAndLock ( 0x0 , &clusterView ) != CL_OK)
            {
                clLog(ERROR,CLM,NA,
                        "_clGmsViewFindAndLock failed");
            }

            if (_clGmsViewDeleteNode  ( 0x0 , req->nodeId ) != CL_OK)
            {
                clLog(ERROR,CLM,NA,
                        "_clGmsViewDeleteNode failed");
            }

            if (_clGmsViewUnlock ( 0x0 ) != CL_OK)
            {
                clLog(ERROR,CLM,NA,
                        "_clGmsViewUnlock failed");
            }

            clLog(ERROR,GEN,NA,
                     "Server Version Mismatch Detected");
            res->rc = CL_GMS_RC(CL_GMS_ERR_CLUSTER_VERSION_MISMATCH);

            clLog(DBG,CLM,NA,"Returning from cluster join API handler");
            return (res->rc);
        }
    }
    else if ( CL_GET_ERROR_CODE(rc) == CL_ERR_TIMEOUT ){
        clLog(DBG,CLM,NA,"Returning from cluster join API handler (timeout)");
        return CL_OK;
    }


JOIN_FAILED:
    return rc;
}


/*-----------------------------------------------------------------------------
 * Cluster Leave API
 *---------------------------------------------------------------------------*/
ClRcT
clGmsClusterLeaveHandler(
                         CL_IN  const ClGmsClusterLeaveRequestT*   const req,
                         CL_OUT       ClGmsClusterLeaveResponseT*  const res)
{
    ClRcT		              rc         = CL_OK;
    ClGmsClusterMemberT       thisNode   = {0};
    ClGmsViewMemberT          viewMember = {{0}};
    ClGmsViewNodeT           *node       = NULL;
    
    if ((req == (const void*)NULL) || (res == NULL))
    {
        return CL_ERR_NULL_POINTER;
    }

    CL_GMS_VERIFY_CLIENT_VERSION( req , res );

    /* if the server is not in a servicable state then ask the client to retry
     *  again after some time */
    if (0 == (ClInt32T)_clGmsIsReadyToServe())
    {
        res->rc = CL_GMS_RC(CL_ERR_TRY_AGAIN);
        return rc;
    }

    clLogNotice("CLUSTER", "LEAVE",
                "Received Cluster Leave request for [nodeId = %d], sync flag [%s]",
                req->nodeId, req->sync ? "yes" : "no");

    /* Try to see if the node is part of the cluster view if not then send the
     *  caller an invalid parameter return code . */
    rc = _clGmsViewFindNode ( 0x0 , req->nodeId , &node );
    if ( rc != CL_OK )
    {
        res->rc = CL_GMS_RC(CL_ERR_INVALID_PARAMETER );
        return (res->rc);
    }

    if(req->sync)
    {
        _clGmsGetThisNodeInfo(&thisNode);

        memcpy(&viewMember.clusterMember, &thisNode, sizeof(ClGmsClusterMemberT));

        rc= clGmsSendMsg(&viewMember, req->groupId, CL_GMS_CLUSTER_LEAVE_MSG , 0x0, 0, NULL);

        if (rc != CL_OK)
        {
            clLog(ERROR,CLM,NA,
                  "Cluster Leave multicase messaging through openais failed.");
            res->rc = rc;
            goto LEAVE_FAILED;
        } 
    }
    else
    {
        rc = _clGmsEngineClusterLeave(req->groupId, req->nodeId);
    }

    LEAVE_FAILED:
    return rc;
}

/*-----------------------------------------------------------------------------
 * Cluster Leader Elect API
 *---------------------------------------------------------------------------*/
ClRcT
clGmsClusterLeaderElectHandler(
    CL_IN   ClGmsClusterLeaderElectRequestT*         req,
    CL_OUT  ClGmsClusterLeaderElectResponseT* const  res)
{
    ClRcT		          rc              = CL_OK;
    ClRcT		          _rc              = CL_OK;
    ClGmsViewMemberT      viewMember      = {{0}};
    ClHandleT             contextHandle   = 0;
    ClContextInfoT       *context_info    = NULL;
    ClGmsCsSectionT       contextCondVar  = {0};
    ClGmsViewT           *thisClusterView = NULL;
    ClGmsNodeIdT          leaderNodeId    = CL_GMS_INVALID_NODE_ID;
    ClGmsNodeIdT          deputyNodeId    = CL_GMS_INVALID_NODE_ID;
    ClBoolT               leadershipChanged = CL_FALSE;
    ClGmsDbT             *thisViewDb     = NULL;
    ClGmsViewNodeT       *foundNode      = NULL;
    
    if ((req == (const void*)NULL) || (res == NULL))
    {
        return CL_ERR_NULL_POINTER;
    }

    CL_GMS_VERIFY_CLIENT_VERSION( req , res );

    /* if the server is not in a servicable state then ask the client to retry
     *  again after some time */
    if (0 == (ClInt32T)_clGmsIsReadyToServe())
    {
        res->rc = CL_GMS_RC(CL_ERR_TRY_AGAIN);
        return rc;
    }
    clLogMultiline(DBG,CLM,NA,
            "Received Cluster Leader Elect request");

    /* Find if the given nodeId exists. If not, then return error */
    rc = _clGmsViewDbFind(0,&thisViewDb);
    if (rc != CL_OK)
    {
        return CL_ERR_DOESNT_EXIST;
    }

    clGmsMutexLock(thisViewDb->viewMutex);

    rc = _clGmsViewFindNodePrivate(thisViewDb, req->preferredLeaderNode, 
            CL_GMS_CURRENT_VIEW, &foundNode);

    if (rc != CL_OK)
    {
        clGmsMutexUnlock(thisViewDb->viewMutex);
        clLog(ERROR,CLM,NA,
                "Requested preferred leader node does not exist");
        return CL_ERR_DOESNT_EXIST;
    }

    if (foundNode->viewMember.clusterMember.credential == 0)
    {
        clGmsMutexUnlock(thisViewDb->viewMutex);
        clLog(ERROR,CLM,NA,
                "Requested preferred leader node is not a system controller node.");
        return CL_ERR_DOESNT_EXIST;
    }

    /* Store the old leader value */
    leaderNodeId = thisViewDb->view.leader;

    if (req->preferredLeaderNode == thisViewDb->view.leader)
    {
        clGmsMutexUnlock(thisViewDb->viewMutex);
        clLogMultiline(NOTICE,CLM,NA,
                "Requested preferred leader is already the leader node. "
                "So not invoking the leader election again");
        rc = CL_ERR_ALREADY_EXIST;
        goto done_return; 
    }

    viewMember = foundNode->viewMember;
    clGmsMutexUnlock(thisViewDb->viewMutex);

    /* Create the conditional variable for synchronization */
    rc = clHandleCreate(contextHandleDatabase,
            sizeof(ClContextInfoT),
            &contextHandle);
    if (rc != CL_OK)
    {
        return CL_ERR_NO_RESOURCE;
    }

    rc = clHandleCheckout(contextHandleDatabase,contextHandle,(void*)&context_info);
    CL_ASSERT((rc == CL_OK) && (context_info != NULL));

    clGmsCsCreate(&contextCondVar);

    context_info->condVar.cond = contextCondVar.cond;
    context_info->condVar.mutex = contextCondVar.mutex;
    viewMember.contextHandle = contextHandle;
    rc = clHandleCheckin(contextHandleDatabase,contextHandle);

    /* Aquire the cond mutex before doing mcast */
    clGmsMutexLock ( contextCondVar.mutex );
    rc= clGmsSendMsg(&viewMember, 0 /*This is dummy*/,
                        CL_GMS_LEADER_ELECT_MSG , 0x0, 0, NULL );
    if (rc != CL_OK)
    {
        clGmsMutexUnlock ( contextCondVar.mutex );
        clLog(ERROR,CLM,NA,
                "Leader Elect multicast message through openais failed");
        res->rc = rc;
        return rc;
    }

    rc = clGmsCondWait  ( contextCondVar.cond , contextCondVar.mutex , opTimeout );
    clGmsMutexUnlock ( contextCondVar.mutex );
    if (rc != CL_OK)
    {
        return rc;
    }

    /* Checkout contexHandle and catch the value of RC */
    rc = clHandleCheckout(contextHandleDatabase,contextHandle,(void*)&context_info);
    CL_ASSERT((rc == CL_OK) && (context_info != NULL));
    if (context_info->rc != CL_OK)
    {
        rc = context_info->rc;
        clHandleCheckin(contextHandleDatabase,contextHandle);
        return rc;
    }
    /* Server side operation was successful */
    clHandleCheckin(contextHandleDatabase,contextHandle);

done_return:
    /* Get the latest leader and deputy node from the clusterNode */
    _rc = _clGmsViewFindAndLock(0, &thisClusterView);
    if ((_rc != CL_OK) || (thisClusterView == NULL))
    {
        clLog(ERROR,CLM, NA,
                "Failed to get thisClusterView while running leader election. rc [0x%x]",_rc);
    } else {
        if (leaderNodeId != thisClusterView->leader)
        {
            /* Leadership has changed. So update the leadership changed flag */
            leadershipChanged = CL_TRUE;
        }
        leaderNodeId = thisClusterView->leader;
        deputyNodeId = thisClusterView->deputy;
    }
    if (_clGmsViewUnlock(0x0) != CL_OK)
    {
        clLog(ERROR,LEA,NA,
                ("_clGmsViewUnlock failed in leader election timer callback"));
    }

    if (res != NULL)
    {
        res->rc = rc;
        res->leader = leaderNodeId;
        res->deputy = deputyNodeId;
        res->leadershipChanged = leadershipChanged;
    }

    return rc;
}

/*-----------------------------------------------------------------------------
 * Cluster Member Eject API
 *---------------------------------------------------------------------------*/
ClRcT
clGmsClusterMemberEjectHandler(
    CL_IN   const ClGmsClusterMemberEjectRequestT*  const  req,
    CL_OUT  ClGmsClusterMemberEjectResponseT*       const  res)
{
    ClRcT		        rc         = CL_OK;
    ClGmsViewMemberT    viewMember = {{0}};
    ClGmsClusterMemberT thisNode   = {0} ;
    ClGmsViewNodeT     *node       = NULL;

    if ((req == (const void*)NULL) || (res == NULL))
    {
        return CL_ERR_NULL_POINTER;
    }

    CL_GMS_VERIFY_CLIENT_VERSION( req , res );

    /* if the server is not in a servicable state then ask the client to retry
     *  again after some time */
    if(0 == (ClInt32T)_clGmsIsReadyToServe()){
        res->rc = CL_GMS_RC(CL_ERR_TRY_AGAIN);
        return rc;
    }

    clLogMultiline(DBG,CLM,NA,
            "Received Cluster Member Eject request");
    
    /* Try to see if the node is part of the cluster view if not then send the
     *  caller an invalid parameter return code . */
    rc = _clGmsViewFindNode ( 0x0 , req->nodeId , &node );
    if( rc != CL_OK ){
        return (res->rc = CL_GMS_RC(CL_ERR_INVALID_PARAMETER ));
    }
    

    /* send message to all the server instances */
    thisNode.nodeId = req->nodeId;
    memcpy(&viewMember.clusterMember, &thisNode, sizeof(ClGmsClusterMemberT));
    rc = clGmsSendMsg(&viewMember, 0x0, CL_GMS_CLUSTER_EJECT_MSG ,req->reason, 0, NULL );
    if( rc != CL_OK ){
        clLog(ERROR,GEN,NA,
                 "Member eject multicast messaging through openais failed ");
        res->rc=rc;
        return res->rc;
    }
    
    res->rc = rc;
    
    return rc;
}

/*-----------------------------------------------------------------------------
 * Group Create API
 *---------------------------------------------------------------------------*/
ClRcT
clGmsGroupCreateHandler(
    CL_IN   ClGmsGroupCreateRequestT*     req,
    CL_OUT  ClGmsGroupCreateResponseT*    res)
{
    ClRcT		      rc             = CL_OK;
    ClGmsViewMemberT  viewMember     = {{0}};
    ClGmsGroupIdT     groupId        = 0;
    ClGmsDbT         *thisViewDb     = NULL;
    ClHandleT         contextHandle  = 0;
    ClContextInfoT   *context_info   = NULL;
    ClGmsCsSectionT   contextCondVar = {0};
    ClCharT           name[256]      = "";

    if ((req == (const void*)NULL) || (res == NULL))
    {
        return CL_ERR_NULL_POINTER;
    }
    
    CL_GMS_VERIFY_CLIENT_VERSION( req , res );


    /* if the server is not in a servicable state then ask the 
       client to retry again after some time */
    if (readyToServeGroups != CL_TRUE)
    {
        res->rc = CL_ERR_TRY_AGAIN;
        return CL_ERR_TRY_AGAIN;
    }

    getNameString(&req->groupName, name);

    clLogMultiline(DBG,GROUPS,NA,
            "Received Group Create request for [groupName = %s]",name);

    clGmsMutexLock(gmsGlobalInfo.nameIdMutex);

    rc = _clGmsNameIdDbFind(&gmsGlobalInfo.groupNameIdDb, &req->groupName, &groupId);

    clGmsMutexUnlock(gmsGlobalInfo.nameIdMutex);

    if (rc == CL_OK)
    {
        clLogMultiline(NOTICE,GROUPS,NA,
                "Requested group [%s] already exists",name);
        /* The group is already created. Return the group ID */
        rc = CL_ERR_ALREADY_EXIST;
        res->groupId = groupId;
        return rc;
    }
    
    /* Create the conditional variable for synchronization */
    rc = clHandleCreate(contextHandleDatabase,
                        sizeof(ClContextInfoT),
                        &contextHandle);                    
    if (rc != CL_OK)
    {
        return CL_ERR_NO_RESOURCE;
    }

    rc = clHandleCheckout(contextHandleDatabase,contextHandle,(void*)&context_info);
    CL_ASSERT((rc == CL_OK) && (context_info != NULL));

    clGmsCsCreate(&contextCondVar);

    context_info->condVar.cond = contextCondVar.cond;
    context_info->condVar.mutex = contextCondVar.mutex;
    viewMember.contextHandle = contextHandle;
    rc = clHandleCheckin(contextHandleDatabase,contextHandle);


    /* Get the values from req into viewMember structure. */
    memcpy(&viewMember.groupData.groupName, &req->groupName, 
            sizeof(ClGmsGroupNameT));
    memcpy(&viewMember.groupData.groupParams, &req->groupParams,
            sizeof(ClGmsGroupParamsT));

    /* Aquire the cond mutex before doing mcast */
    clGmsMutexLock ( contextCondVar.mutex );
    
    rc= clGmsSendMsg(&viewMember, 0 /*This is dummy*/, 
                     CL_GMS_GROUP_CREATE_MSG , 0x0, 0, NULL );
    if (rc != CL_OK)
    {
    	clGmsMutexUnlock ( contextCondVar.mutex );
        clLog(ERROR,GEN,NA,
                "Group Create multicast messaging through openais failed");
        res->rc = rc;
        return rc;
    }

    /* Enter critical section and wait for the message to be 
       received by self and the create proc signals on the mutex */
    rc = clGmsCondWait  ( contextCondVar.cond , contextCondVar.mutex , opTimeout );
    clGmsMutexUnlock ( contextCondVar.mutex );
    if (rc != CL_OK)
    {
        return rc;
    }

    /* Checkout contexHandle and catch the value of RC */
    rc = clHandleCheckout(contextHandleDatabase,contextHandle,(void*)&context_info);
    CL_ASSERT((rc == CL_OK) && (context_info != NULL));
    if (context_info->rc != CL_OK)
    {
        rc = context_info->rc;
        clHandleCheckin(contextHandleDatabase,contextHandle);
        return rc;
    }
    /* Server side operation was successful */
    clHandleCheckin(contextHandleDatabase,contextHandle);

    clGmsMutexLock(gmsGlobalInfo.nameIdMutex);

    rc = _clGmsNameIdDbFind(&gmsGlobalInfo.groupNameIdDb, &req->groupName, &groupId);

    clGmsMutexUnlock(gmsGlobalInfo.nameIdMutex);
    
    if (rc != CL_OK)
    {
        return rc;
    }

    rc = _clGmsViewDbFind(groupId,&thisViewDb);
    if (rc != CL_OK)
    {
        return rc;
    }

    clGmsMutexLock(thisViewDb->viewMutex);

    clLog(DBG,GROUPS,NA,
            "GroupId [%d] has been created for group %s", groupId,name);
    res->groupId = groupId;

    memcpy(&res->groupParams, &thisViewDb->groupInfo.groupParams,
            sizeof(ClGmsGroupParamsT));
    res->serverVersion.releaseCode = ringVersion.releaseCode;
    res->serverVersion.minorVersion= ringVersion.minorVersion;
    res->serverVersion.majorVersion= ringVersion.majorVersion;

    clGmsMutexUnlock(thisViewDb->viewMutex);

    res->rc = rc;
    return rc;
}

/*-----------------------------------------------------------------------------
 * Group Destroy API
 *---------------------------------------------------------------------------*/
ClRcT
clGmsGroupDestroyHandler(
    CL_IN   ClGmsGroupDestroyRequestT*    req,
    CL_OUT  ClGmsGroupDestroyResponseT*   res)
{
    ClRcT		      rc             = CL_OK;
    ClGmsViewMemberT  viewMember     = {{0}};
    ClGmsDbT         *thisViewDb     = NULL;
    ClHandleT         contextHandle  = 0;
    ClContextInfoT   *context_info   = NULL;
    ClGmsCsSectionT   contextCondVar = {0};

    if ((req == (const void*)NULL) || (res == NULL))
    {
        return CL_ERR_NULL_POINTER;
    }

    CL_GMS_VERIFY_CLIENT_VERSION( req , res );

    /* if the server is not in a servicable state then ask the client to retry
     *  again after some time */
    if (readyToServeGroups != CL_TRUE)
    {
        res->rc = CL_ERR_TRY_AGAIN;
        return CL_ERR_TRY_AGAIN;
    }
    
    clLogMultiline(DBG,GROUPS,NA,
            "Received Group Destroy request for [groupId = %d]",
            req->groupId);

    /* Check if Db exists. If does not exists, then return error */
    rc = _clGmsViewDbFind(req->groupId,&thisViewDb);
    
    if (rc != CL_OK)
    {
        return rc;
    }
    clGmsMutexLock(thisViewDb->viewMutex);

    thisViewDb->groupInfo.setForDelete = CL_TRUE;
    /* Even if the db exists, check if the number of members is 0.
       if not return error */

    if (thisViewDb->groupInfo.noOfMembers != 0)
    {
        clGmsMutexUnlock(thisViewDb->viewMutex);
        return CL_ERR_INUSE;
    }

    viewMember.groupData.groupId = req->groupId;
    memcpy(&viewMember.groupData.groupName, 
           &thisViewDb->groupInfo.groupName,
           sizeof(ClGmsGroupNameT));

    clGmsMutexUnlock(thisViewDb->viewMutex);

    /* Create the conditional variable for synchronization */
    rc = clHandleCreate(contextHandleDatabase,
                    sizeof(ClContextInfoT),
                    &contextHandle);
    if (rc != CL_OK)
    {
        return CL_ERR_NO_RESOURCE;
    }

    rc = clHandleCheckout(contextHandleDatabase,contextHandle,(void*)&context_info);
    CL_ASSERT((rc == CL_OK) && (context_info != NULL));

    clGmsCsCreate(&contextCondVar);

    context_info->condVar.cond = contextCondVar.cond;
    context_info->condVar.mutex = contextCondVar.mutex;
    viewMember.contextHandle = contextHandle;
    rc = clHandleCheckin(contextHandleDatabase,contextHandle);

    /* Aquire the cond mutex before doing mcast */
    clGmsMutexLock ( contextCondVar.mutex );

    rc= clGmsSendMsg(&viewMember, req->groupId,
            CL_GMS_GROUP_DESTROY_MSG , 0x0, 0, NULL );

    if (rc != CL_OK)
    {
    	clGmsMutexUnlock ( contextCondVar.mutex );
        clLog(ERROR,GROUPS,NA,
                "Group Destroy multicast messagin through openais failed");
        res->rc = rc;
        return rc;
    }

    rc = clGmsCondWait  ( contextCondVar.cond , contextCondVar.mutex , opTimeout );
    clGmsMutexUnlock ( contextCondVar.mutex );
    if (rc != CL_OK)
    {
        return rc;
    }

    /* Checkout contexHandle and catch the value of RC */
    rc = clHandleCheckout(contextHandleDatabase,contextHandle,(void*)&context_info);
    CL_ASSERT((rc == CL_OK) && (context_info != NULL));
    if (context_info->rc != CL_OK)
    {
        rc = context_info->rc;
        clHandleCheckin(contextHandleDatabase,contextHandle);
        return rc;
    }
    /* Server side operation was successful */
    clHandleCheckin(contextHandleDatabase,contextHandle);

    return rc;
}

/*-----------------------------------------------------------------------------
 * Group Join API
 *---------------------------------------------------------------------------*/
ClRcT
clGmsGroupJoinHandler(
    CL_IN   ClGmsGroupJoinRequestT*     req,
    CL_OUT  ClGmsGroupJoinResponseT*    res)
{
    ClRcT             rc             = CL_OK;
    ClGmsViewMemberT  viewMember     = {{0}};
    ClGmsDbT         *thisViewDb     = NULL;
    ClGmsViewNodeT   *foundNode      = NULL;
    ClHandleT         contextHandle  = 0;
    ClContextInfoT   *context_info   = NULL;
    ClGmsCsSectionT   contextCondVar = {0};

    if ((req == (const void*)NULL) || (res == NULL))
    {
        return CL_ERR_NULL_POINTER;
    }

    CL_GMS_VERIFY_CLIENT_VERSION( req , res );

    /* if the server is not in a servicable state then ask the client to retry
     *  again after some time */
    if (readyToServeGroups != CL_TRUE)
    {
        res->rc = CL_ERR_TRY_AGAIN;
        return CL_ERR_TRY_AGAIN;
    }

    clLogMultiline(DBG,GROUPS,NA,
            "Received Group Join request for [groupId = %d], [memberId = %d]",
            req->groupId, req->memberId);

    /* Check if the group exists. If not return error */
    rc = _clGmsViewDbFind(req->groupId,&thisViewDb);
    if (rc != CL_OK)
    {
        return CL_ERR_DOESNT_EXIST;
    }
    clGmsMutexLock(thisViewDb->viewMutex);
    if (thisViewDb->groupInfo.setForDelete == CL_TRUE)
    {
        clGmsMutexUnlock(thisViewDb->viewMutex);
        return CL_ERR_OP_NOT_PERMITTED;
    }

    /* Find if the node already exists, if so, return error */
    rc = _clGmsViewFindNodePrivate(thisViewDb, req->memberId, 
            CL_GMS_CURRENT_VIEW, &foundNode);

    if (rc == CL_OK)
    {
        clGmsMutexUnlock(thisViewDb->viewMutex);
        return CL_ERR_ALREADY_EXIST;
    }

    clGmsMutexUnlock(thisViewDb->viewMutex);

    /* Since node is not found, send the multicast message */
    viewMember.groupMember.handle = req->gmsHandle;
    viewMember.groupMember.memberId = req->memberId;
    memcpy(&viewMember.groupMember.memberAddress, &req->memberAddress,
            sizeof(ClIocAddressT));
    memcpy(&viewMember.groupMember.memberName, &req->memberName,
            sizeof(ClGmsMemberNameT));
    viewMember.groupMember.credential = req->credentials;
    viewMember.groupData.groupId = req->groupId;

    /* Create the conditional variable for synchronization */
    rc = clHandleCreate(contextHandleDatabase,
            sizeof(ClContextInfoT),
            &contextHandle);
    if (rc != CL_OK)
    {
        return CL_ERR_NO_RESOURCE;
    }

    rc = clHandleCheckout(contextHandleDatabase,contextHandle,(void*)&context_info);
    CL_ASSERT((rc == CL_OK) && (context_info != NULL));

    clGmsCsCreate(&contextCondVar);

    context_info->condVar.cond = contextCondVar.cond;
    context_info->condVar.mutex = contextCondVar.mutex;
    viewMember.contextHandle = contextHandle;
    rc = clHandleCheckin(contextHandleDatabase,contextHandle);

    /* Aquire the cond mutex before doing mcast */
    clGmsMutexLock ( contextCondVar.mutex );
    rc= clGmsSendMsg(&viewMember, req->groupId,
                        CL_GMS_GROUP_JOIN_MSG , 0x0, 0, NULL );
    if (rc != CL_OK)
    {
    	clGmsMutexUnlock ( contextCondVar.mutex );
        clLog(ERROR,GROUPS,NA,
                "Group Join Multicast messaging through openais failed");
        res->rc = rc;
        return rc;
    }

    rc = clGmsCondWait  ( contextCondVar.cond , contextCondVar.mutex , opTimeout );
    clGmsMutexUnlock ( contextCondVar.mutex );
    if (rc != CL_OK)
    {
        return rc;
    }

    /* Checkout contexHandle and catch the value of RC */
    rc = clHandleCheckout(contextHandleDatabase,contextHandle,(void*)&context_info);
    CL_ASSERT((rc == CL_OK) && (context_info != NULL));
    if (context_info->rc != CL_OK)
    {
        rc = context_info->rc;
        clHandleCheckin(contextHandleDatabase,contextHandle);
        return rc;
    }
    /* Server side operation was successful */
    clHandleCheckin(contextHandleDatabase,contextHandle);

    return rc;
}

/*-----------------------------------------------------------------------------
 * Group Leave API
 *---------------------------------------------------------------------------*/
ClRcT
clGmsGroupLeaveHandler(
    CL_IN   ClGmsGroupLeaveRequestT*    req,
    CL_OUT  ClGmsGroupLeaveResponseT*   res)
{
    ClRcT              rc             = CL_OK;
    ClGmsDbT          *thisViewDb     = NULL;
    ClGmsViewNodeT    *foundNode      = NULL;
    ClGmsViewMemberT   viewMember     = {{0}};
    ClHandleT          contextHandle  = 0;
    ClContextInfoT    *context_info   = NULL;
    ClGmsCsSectionT    contextCondVar = {0};

    if ((req == (const void*)NULL) || (res == NULL))
    {
        return CL_ERR_NULL_POINTER;
    }

    CL_GMS_VERIFY_CLIENT_VERSION( req , res );

    /* if the server is not in a servicable state then ask the client to retry
     *  again after some time */
    if (readyToServeGroups != CL_TRUE)
    {
        res->rc = CL_ERR_TRY_AGAIN;
        return CL_ERR_TRY_AGAIN;
    }

    clLogMultiline(DBG,GROUPS,NA,
            "Received Group Leave request for [groupId = %d], [memberId = %d]",
            req->groupId, req->memberId);

    /* Check if the group exists. If not return error */
    rc = _clGmsViewDbFind(req->groupId,&thisViewDb);
    if (rc != CL_OK)
    {
        return CL_GMS_ERR_GROUP_DOESNT_EXIST;
    }
    clGmsMutexLock(thisViewDb->viewMutex);

    /* Find if the node exists, if not, return error */
    rc = _clGmsViewFindNodePrivate(thisViewDb, req->memberId,
            CL_GMS_CURRENT_VIEW, &foundNode);

    clGmsMutexUnlock(thisViewDb->viewMutex);

    if (rc != CL_OK)
    {
        return CL_ERR_DOESNT_EXIST;
    }

    viewMember.groupData.groupId = req->groupId;
    viewMember.groupMember.memberId = req->memberId;

    /* Create the conditional variable for synchronization */
    rc = clHandleCreate(contextHandleDatabase,
            sizeof(ClContextInfoT),
            &contextHandle);
    if (rc != CL_OK)
    {
        return CL_ERR_NO_RESOURCE;
    }

    rc = clHandleCheckout(contextHandleDatabase,contextHandle,(void*)&context_info);
    CL_ASSERT((rc == CL_OK) && (context_info != NULL));

    clGmsCsCreate(&contextCondVar);

    context_info->condVar.cond = contextCondVar.cond;
    context_info->condVar.mutex = contextCondVar.mutex;
    viewMember.contextHandle = contextHandle;
    rc = clHandleCheckin(contextHandleDatabase,contextHandle);

    /* Aquire the cond mutex before doing mcast */
    clGmsMutexLock ( contextCondVar.mutex );

    rc= clGmsSendMsg(&viewMember, req->groupId,
            CL_GMS_GROUP_LEAVE_MSG , 0x0, 0, NULL );
    if (rc != CL_OK)
    {
    	clGmsMutexUnlock ( contextCondVar.mutex );
        clLog(ERROR,GROUPS,NA,
              "Group Leave Multicast messaging through openais failed with error [%d]", rc);
        res->rc = rc;
        return rc;
    }

    rc = clGmsCondWait  ( contextCondVar.cond , contextCondVar.mutex , opTimeout );
    clGmsMutexUnlock ( contextCondVar.mutex );
    if (rc != CL_OK)
    {
        return rc;
    }

    /* Checkout contexHandle and catch the value of RC */
    rc = clHandleCheckout(contextHandleDatabase,contextHandle,(void*)&context_info);
    CL_ASSERT((rc == CL_OK) && (context_info != NULL));
    if (context_info->rc != CL_OK)
    {
        rc = context_info->rc;
        clHandleCheckin(contextHandleDatabase,contextHandle);
        return rc;
    }
    /* Server side operation was successful */
    clHandleCheckin(contextHandleDatabase,contextHandle);


    return rc;
}

/*-----------------------------------------------------------------------------
 * Group List Get API
 *---------------------------------------------------------------------------*/
ClRcT 
clGmsGroupInfoListGetHandler(
        CL_IN   ClGmsGroupsInfoListGetRequestT*     req,
        CL_OUT  ClGmsGroupsInfoListGetResponseT*    res) 
{
    ClRcT            rc             = CL_OK;
    ClUint32T        index          = 0;
    ClUint32T        numberOfGroups = 0;
    ClGmsGroupInfoT *groupInfoList  = NULL;

    if ((req == (const void*)NULL) || (res == NULL))
    {
        return CL_ERR_NULL_POINTER;
    }

    CL_GMS_VERIFY_CLIENT_VERSION( req , res );

    /* if the server is not in a servicable state then ask the client to retry
     *  again after some time */
    if (readyToServeGroups != CL_TRUE)
    {
        res->rc = CL_ERR_TRY_AGAIN;
        return CL_ERR_TRY_AGAIN;
    }

    clLogMultiline(DBG,GROUPS,NA,
            "Received GroupInfoListGet request");

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

    res->groupsList.noOfGroups = numberOfGroups;
    res->groupsList.groupInfoList = groupInfoList;
    res->rc = rc;
    
    return rc;
}


/*-----------------------------------------------------------------------------
 * Group Info Get API
 *---------------------------------------------------------------------------*/
ClRcT 
clGmsGroupInfoGetHandler(
        CL_IN   ClGmsGroupInfoGetRequestT*      req,
        CL_OUT  ClGmsGroupInfoGetResponseT*     res) 
{
    ClRcT           rc         = CL_OK;
    ClGmsGroupIdT   groupId    = 0;
    ClGmsDbT       *thisViewDb = NULL;
    ClCharT         name[256]  = "";

    if ((req == (const void*)NULL) || (res == NULL))
    {
        return CL_ERR_NULL_POINTER;
    }

    CL_GMS_VERIFY_CLIENT_VERSION( req , res );

    /* if the server is not in a servicable state then ask the client to retry
     *  again after some time */
    if (readyToServeGroups != CL_TRUE)
    {
        res->rc = CL_ERR_TRY_AGAIN;
        return CL_ERR_TRY_AGAIN;
    }

    getNameString(&req->groupName, name);
    clLogMultiline(DBG,GROUPS,NA,
            "Received GroupInfoGet request for [groupName = %s]",name);

    /* Take the lock on the database */
    clGmsMutexLock(gmsGlobalInfo.nameIdMutex);

    rc = _clGmsNameIdDbFind(&gmsGlobalInfo.groupNameIdDb, &req->groupName, &groupId);

    clGmsMutexUnlock(gmsGlobalInfo.nameIdMutex);

    if (rc != CL_OK)
    {
        return rc;
    }

    rc = _clGmsViewDbFind(groupId,&thisViewDb);
    if (rc != CL_OK)
    {
        return rc;
    }

    clGmsMutexLock(thisViewDb->viewMutex);
    memcpy(&res->groupInfo,&thisViewDb->groupInfo,sizeof(ClGmsGroupInfoT));
    clGmsMutexUnlock(thisViewDb->viewMutex);

    res->rc = rc;
    
    return rc;
}

/*-----------------------------------------------------------------------------
 * Group Mcast Send API
 *---------------------------------------------------------------------------*/
ClRcT
clGmsGroupMcastHandler(
    CL_IN   ClGmsGroupMcastRequestT*     req,
    CL_OUT  ClGmsGroupMcastResponseT*    res)
{
    ClRcT             rc             = CL_OK;
    ClGmsViewMemberT  viewMember     = {{0}};
    ClGmsDbT         *thisViewDb     = NULL;

    clLogDebug("MCAST","MSG","Received a group multicast message request");
    if ((req == (const void*)NULL) || (res == NULL))
    {
        return CL_ERR_NULL_POINTER;
    }

    CL_GMS_VERIFY_CLIENT_VERSION( req , res );

    /* if the server is not in a servicable state then ask the client to retry
     *  again after some time */
    if (readyToServeGroups != CL_TRUE)
    {
        res->rc = CL_ERR_TRY_AGAIN;
        return CL_ERR_TRY_AGAIN;
    }

    /* Check if the group exists. If not return error */
    rc = _clGmsViewDbFind(req->groupId,&thisViewDb);
    if (rc != CL_OK)
    {
        return CL_ERR_DOESNT_EXIST;
    }

    clGmsMutexLock(thisViewDb->viewMutex);

    if (thisViewDb->groupInfo.setForDelete == CL_TRUE)
    {
        clGmsMutexUnlock(thisViewDb->viewMutex);
        return CL_ERR_OP_NOT_PERMITTED;
    }

    clGmsMutexUnlock(thisViewDb->viewMutex);

    /* Since node is not found, send the multicast message */
    viewMember.groupMember.memberId = req->memberId;
    viewMember.groupData.groupId = req->groupId;


    /* Aquire the cond mutex before doing mcast */
    rc= clGmsSendMsg(&viewMember, req->groupId, 
                     CL_GMS_GROUP_MCAST_MSG, 0x0,
                     req->dataSize,
                     req->data);
    if (rc != CL_OK)
    {
        clLog(ERROR,GROUPS,NA,
                "Group Join Multicast messaging through openais failed");
        res->rc = rc;
        return rc;
    }

    return rc;
}
/*-----------------------------------------------------------------------------
 * Notification for a component coming up
 *---------------------------------------------------------------------------*/
ClRcT 
clGmsCompUpNotifyHandler(
        CL_IN   ClGmsCompUpNotifyRequestT*      req,
        CL_OUT  ClGmsCompUpNotifyResponseT*     res) 
{
    ClRcT           rc = CL_OK;

    if ((req == (const void*)NULL) || (res == NULL))
    {
        return CL_ERR_NULL_POINTER;
    }

    CL_GMS_VERIFY_CLIENT_VERSION( req , res );

    clLogMultiline(DBG,GROUPS,NA,
            "Received Even Comp UP Notification from CPM");
    
    clGmsEventInit();
    res->rc = rc;
    
    return rc;
}

/*-----------------------------------------------------------------------------
 * Handler to remove the group member from all the groups during component death
 *---------------------------------------------------------------------------*/
ClRcT   clGroupMemberLeaveOnCompDeath(
        CL_IN   ClGmsMemberIdT      memberId)
{
    ClGmsViewMemberT      viewMember = {{0}};
    ClRcT                 rc         = CL_OK;  

    viewMember.groupMember.memberId = memberId;

    rc= clGmsSendMsg(&viewMember, 0 /*This is dummy*/,
           CL_GMS_COMP_DEATH, 0x0, 0, NULL );

    if (rc != CL_OK)
    {
        clLog(ERROR,GROUPS,NA,
                "Group Leave Multicast messaging through openais failed with error [%d]", rc);
    }

    return rc;
}
/*-----------------------------------------------------------------------------
 * Group Leader Elect API
 *---------------------------------------------------------------------------*/
ClRcT
clGmsGroupLeaderElectHandler(
    CL_IN   ClGmsGroupLeaderElectRequestT*      req,
    CL_OUT  ClGmsGroupLeaderElectResponseT*     res)
{
    clLogMultiline(DBG,GROUPS,NA,
            "Group Leader Election is not supported");
    return CL_ERR_NOT_IMPLEMENTED;
}

/*-----------------------------------------------------------------------------
 * Group Member Eject API
 *---------------------------------------------------------------------------*/
ClRcT
clGmsGroupMemberEjectHandler(
    CL_IN   ClGmsGroupMemberEjectRequestT*      req,
    CL_OUT  ClGmsGroupMemberEjectResponseT*     res)
{
    clLogMultiline(DBG,GROUPS,NA,
            "Group member eject is not supported");
    return CL_ERR_NOT_IMPLEMENTED;
}


/* Called from the CLM code to invoke the eject callback of the member the
 * callback location information is registered when the join happens we call
 * in to the member */
ClRcT 
_clGmsCallClusterMemberEjectCallBack(
        const ClGmsMemberEjectReasonT reason)
{
    ClGmsClusterMemberEjectCallbackDataT  callbackData = {0};

    /*Initializing the callbackData*/
    callbackData.gmsHandle = ejectCallback.handle;
    callbackData.reason = reason;

    return cl_gms_cluster_member_eject_callback(
            ejectCallback.address,
            &callbackData 
            );
}

void clEvtSubsTestDeliverCallback(ClEventSubscriptionIdT subscriptionId,
                                  ClEventHandleT         eventHandle,
                                  ClSizeT                eventDataSize )
{
    ClRcT                   rc = CL_OK;
    ClCpmEventPayLoadT      payLoad;

    switch(subscriptionId)
    {
        case 1:
            {
                rc = clCpmEventPayLoadExtract(eventHandle, eventDataSize, CL_CPM_COMP_EVENT, (void *)&payLoad);
                if (rc != CL_OK)
                {
                    clLog(ERROR,GEN,NA, "Component failure event received but Event payload extract failed, rc=[%#x]", rc);
                    goto out_free;
                }

                clLog(DBG,GEN,NA, "Component event received, Node [%.*s, %d] Comp [%.*s, id: 0x%x, eo: 0x%llx, port: 0x%x] Event [%s, %d]", payLoad.nodeName.length, payLoad.nodeName.value, payLoad.nodeIocAddress ,payLoad.compName.length, payLoad.compName.value, payLoad.compId, payLoad.eoId, payLoad.eoIocPort,ClCpmCompEventT2Str(payLoad.operation),payLoad.operation);
                
                if(payLoad.operation != CL_CPM_COMP_DEATH)
                    goto out_free;
                /* Invoke group leave for this component */
                clGroupMemberLeaveOnCompDeath(payLoad.compId);
                break;
            }
        default:
            {
                clLog(DBG,GEN,NA,
                        "Event with subscription ID %d has been received",subscriptionId);
            }
    }
    
    out_free:
    clEventFree(eventHandle);
}

void clGmsEventInit(void)
{
    ClRcT                    rc = CL_OK;
    ClEventInitHandleT       evtInitHandle = 0;
    ClVersionT               version = {'B', 0x01, 0x01};
    ClEventChannelHandleT    evtChannelHandleGlobal = 0;
    ClEventChannelHandleT    nodeEvtChannelHandleGlobal = 0;
    ClNameT                  cpmChannelName = {0};
    const ClEventCallbacksT  evtCallbacks = {
                                NULL,
                                clEvtSubsTestDeliverCallback
                             };
    ClUint32T                 deathPattern   = htonl(CL_CPM_COMP_DEATH_PATTERN);
    ClUint32T                 nodeDeparturePattern = htonl(CL_CPM_NODE_DEATH_PATTERN);
    ClEventFilterT            compDeathFilter[]  = {{CL_EVENT_EXACT_FILTER, 
                                                {0, (ClSizeT)sizeof(deathPattern), (ClUint8T*)&deathPattern}}
    };
    ClEventFilterArrayT      compDeathFilterArray = {sizeof(compDeathFilter)/sizeof(compDeathFilter[0]), 
                                                     compDeathFilter
    };
    ClEventFilterT            nodeDepartureFilter[]         = { {CL_EVENT_EXACT_FILTER,
                                                                {0, (ClSizeT)sizeof(nodeDeparturePattern),
                                                                (ClUint8T*)&nodeDeparturePattern}}
    };
    ClEventFilterArrayT       nodeDepartureFilterArray = {sizeof(nodeDepartureFilter)/sizeof(nodeDepartureFilter[0]),
                                                          nodeDepartureFilter 
    };

    cpmChannelName.length = strlen(CL_CPM_EVENT_CHANNEL_NAME);
    strncpy(cpmChannelName.value, CL_CPM_EVENT_CHANNEL_NAME, cpmChannelName.length);

    rc = clEventInitialize(&evtInitHandle, &evtCallbacks, &version);
    if (rc != CL_OK)
    {
        clLog(CRITICAL,GEN,NA,
                "Event initialization failed, rc [%#x]", rc);
    }
    rc = clEventChannelOpen(evtInitHandle,
            &cpmChannelName,
            CL_EVENT_CHANNEL_SUBSCRIBER |
            CL_EVENT_GLOBAL_CHANNEL,
            (ClTimeT)-1,
            &evtChannelHandleGlobal);
    if (rc != CL_OK)
    {
        clLog(CRITICAL,GEN,NA,
                "Event Channel [%.*s] open failed, rc [%#x]",
                cpmChannelName.length,
                cpmChannelName.value,
                rc);
    }

    rc = clEventSubscribe(evtChannelHandleGlobal,
                          &compDeathFilterArray,
                          0x1,
                          NULL);
    if (rc != CL_OK)
    {
        clLog(CRITICAL,GEN,NA,
                "Event Channel [%.*s] subscribe failed, rc [%#x]",
                cpmChannelName.length,
                cpmChannelName.value,
                rc);
    }

    cpmChannelName.length = strlen(CL_CPM_NODE_EVENT_CHANNEL_NAME);
    strncpy(cpmChannelName.value, CL_CPM_NODE_EVENT_CHANNEL_NAME, cpmChannelName.length);

    rc = clEventChannelOpen(evtInitHandle,
            &cpmChannelName,
            CL_EVENT_CHANNEL_SUBSCRIBER |
            CL_EVENT_GLOBAL_CHANNEL,
            (ClTimeT)-1,
            &nodeEvtChannelHandleGlobal);
    if (rc != CL_OK)
    {
        clLog(CRITICAL,GEN,NA,
                "Event Channel [%.*s] open failed, rc [%#x]",
                cpmChannelName.length,
                cpmChannelName.value,
                rc);
    }

    rc = clEventSubscribe(nodeEvtChannelHandleGlobal,
                          &nodeDepartureFilterArray,
                          0x2,
                          NULL);
    if (rc != CL_OK)
    {
        clLog(CRITICAL,GEN,NA,
                "Event Channel [%.*s] subscribe failed, rc [%#x]",
                cpmChannelName.length,
                cpmChannelName.value,
                rc);
    }
    return;
}
