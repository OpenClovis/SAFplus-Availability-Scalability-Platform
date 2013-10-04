#include <clTmsApi.h>
#include <clCommon.h>
#include <clCommonErrors.h>
#include <clDebugApi.h>
#include <clIocIpi.h>

#include <clClmTmsCommon.h>
#include <clClmApi.h>
#include <clGmsErrors.h>
#include <clGmsCommon.h>
#include <saClm.h>

#include "clHandleApi.h"
#include "clVersionApi.h"
#include "clGmsRmdClient.h"
#include "clGmsApiClient.h"
/******************************************************************************
 * TOTAL ORDERING MESSAGE SERVICE APIs
 *****************************************************************************/
/**
 * Callback function that the user can register to receive the
 * sync multicast messages on a given process group.
 */

/*-----------------------------------------------------------------------------
 * Group Create API
 *---------------------------------------------------------------------------*/
ClRcT clTmsGroupCreate(
    CL_IN    ClTmsHandleT                        gmsHandle,
    CL_IN    ClTmsGroupNameT                    *groupName,
    CL_INOUT ClTmsGroupParamsT                  *groupParams,
    CL_OUT   ClTmsGroupIdT                      *groupId)
{
    ClRcT                                rc = CL_OK;
    struct gms_instance                 *gms_instance_ptr = NULL;
    ClGmsGroupCreateRequestT               req = {0};
    ClGmsGroupCreateResponseT             *res = NULL;

    CL_GMS_SET_CLIENT_VERSION( req );
    /*FIXME: Currently groupManageCallbacks and groupParams are
     * allowed to be NULL */
    if ((groupId == NULL) || (groupName == NULL))
    {
        return CL_GMS_RC(CL_ERR_NULL_POINTER);
    }
    
    rc = clHandleCheckout(handle_database, gmsHandle, (void*)&gms_instance_ptr);
    if (rc != CL_OK)
    {
        return rc;
    }
    CL_ASSERT(gms_instance_ptr != NULL);
    
    req.gmsHandle = gmsHandle;
    memcpy((void*)&req.groupName, (void*)groupName, sizeof(SaNameT));
    if (groupParams != NULL)
    {
        memcpy((void*)&req.groupParams, (void*)groupParams,
                sizeof(ClGmsGroupParamsT));
    }
    req.groupParams.isIocGroup = CL_TRUE;
    
    clGmsMutexLock(gms_instance_ptr->response_mutex);

    rc = cl_gms_group_create_rmd(&req, 0 /* use def. timeout */, &res);
    if (rc != CL_OK) /* If there was an error, res isn't allocated */
    {
        goto error_unlock_checkin;
    }
    CL_ASSERT(res != NULL);
    
    rc = res->rc;

    *groupId = res->groupId;
    if (groupParams)
    {
        memcpy((void*)groupParams, (void*)&(res->groupParams),
                sizeof(ClGmsGroupParamsT));
    }
                                
    clHeapFree((void*)res);
    
error_unlock_checkin:
    clGmsMutexUnlock(gms_instance_ptr->response_mutex);
    
    clHandleCheckin(handle_database, gmsHandle);
    return CL_GMS_RC(rc);
}


/*-----------------------------------------------------------------------------
 * Group Destroy API
 *---------------------------------------------------------------------------*/
ClRcT clTmsGroupDestroy(
    CL_IN   ClTmsHandleT                    gmsHandle,
    CL_IN   ClTmsGroupIdT                   groupId)
{
    ClRcT                                rc = CL_OK;
    struct gms_instance                 *gms_instance_ptr = NULL;
    ClGmsGroupDestroyRequestT              req = {0};
    ClGmsGroupDestroyResponseT            *res = NULL;

    CL_GMS_SET_CLIENT_VERSION( req );
    rc = clHandleCheckout(handle_database, gmsHandle, (void*)&gms_instance_ptr);
    if (rc != CL_OK)
    {
        return rc;
    }
    CL_ASSERT(gms_instance_ptr != NULL);
    
    req.gmsHandle   = gmsHandle;
    req.groupId     = groupId;
    
    clGmsMutexLock(gms_instance_ptr->response_mutex);

    rc = cl_gms_group_destroy_rmd(&req, 0 /* use def. timeout */, &res);
    if (rc != CL_OK) /* If there was an error, res isn't allocated */
    {
        goto error_unlock_checkin;
    }
    CL_ASSERT(res != NULL);
    
    rc = res->rc;
    
    clHeapFree((void*)res);
    
error_unlock_checkin:
    clGmsMutexUnlock(gms_instance_ptr->response_mutex);
    
    clHandleCheckin(handle_database, gmsHandle);
    return CL_GMS_RC(rc);
}


/*-----------------------------------------------------------------------------
 * Group Join API
 *---------------------------------------------------------------------------*/
/*FIXME: MemberID should not be 32bit integer */
ClRcT clTmsGroupJoin(
    CL_IN ClTmsHandleT                      gmsHandle,
    CL_IN ClTmsGroupIdT                     groupId,
    CL_IN ClTmsMemberIdT                    memberId,
    CL_IN ClTmsMemberNameT                 *memberName,
    CL_IN ClTmsLeadershipCredentialsT       credentials,
    CL_IN ClTmsGroupMessageDeliveryCallbackT msgDeliveryCallback,
    CL_IN ClTimeT                           timeout)
{
    ClRcT                                rc = CL_OK;
    struct gms_instance                 *gms_instance_ptr = NULL;
    ClGmsGroupJoinRequestT               req = {0};
    ClGmsGroupJoinResponseT             *res = NULL;
    ClIocMcastUserInfoT                  multicastInfo = {0};
    ClIocPhysicalAddressT                selfAddress = {0};

    CL_GMS_SET_CLIENT_VERSION( req );
    
    rc = clHandleCheckout(handle_database, gmsHandle, (void*)&gms_instance_ptr);
    if (rc != CL_OK)
    {
        return rc;
    }
    CL_ASSERT(gms_instance_ptr != NULL);

    /* Get self IOC address details */
    selfAddress.nodeAddress = clIocLocalAddressGet();
    clEoMyEoIocPortGet(&selfAddress.portId);
    
    req.gmsHandle   = gmsHandle;
    req.groupId     = groupId;
    req.memberId    = memberId;
    if (memberName != NULL)
    {
        memcpy(&req.memberName, memberName, sizeof(SaNameT));
    }

    req.credentials = credentials;
    req.sync        = CL_TRUE;
    req.memberAddress.iocPhyAddress.nodeAddress = selfAddress.nodeAddress;
    req.memberAddress.iocPhyAddress.portId = selfAddress.portId;
    
    clGmsMutexLock(gms_instance_ptr->response_mutex);
    gms_instance_ptr->mcastMessageDeliveryCallback = msgDeliveryCallback;

    /* Check if this client has already joined the group */
    multicastInfo.mcastAddr = CL_IOC_MULTICAST_ADDRESS_FORM(0, groupId);
    multicastInfo.physicalAddr.nodeAddress = 
        req.memberAddress.iocPhyAddress.nodeAddress;
    multicastInfo.physicalAddr.portId =
        req.memberAddress.iocPhyAddress.portId;
    rc = clIocMcastIsRegistered(&multicastInfo);
    if (CL_GET_ERROR_CODE(rc) == CL_ERR_ALREADY_EXIST)
    {
        rc = CL_ERR_ALREADY_EXIST;
        goto error_unlock_checkin;
    }
    
    rc = cl_gms_group_join_rmd(&req, (ClUint32T)(timeout/NS_IN_MS), &res);
    if (rc != CL_OK) /* If there was an error, res isn't allocated */
    {
        goto error_unlock_checkin;
    }
    CL_ASSERT(res != NULL);
    
    rc = res->rc;
    if (rc == CL_OK)
    {
        /*FIXME: This needs to be done only for IOC
         * groups in future. */
        rc = clIocMulticastRegister(&multicastInfo);
        if (rc != CL_OK)
        {
            clLogError(GROUPS,NA,
                       "IOC mutlicast address register failed with rc %d\n",rc);
            rc = CL_GMS_ERR_IOC_REGISTRATION;
        }
    }

    clHeapFree((void*)res);
    
error_unlock_checkin:
    clGmsMutexUnlock(gms_instance_ptr->response_mutex);
    
    clHandleCheckin(handle_database, gmsHandle);
    return CL_GMS_RC(rc);
}

/*-----------------------------------------------------------------------------
 * Group Leave API
 *---------------------------------------------------------------------------*/
ClRcT clTmsGroupLeave(
    CL_IN ClTmsHandleT                      gmsHandle,
    CL_IN ClTmsGroupIdT                     groupId,
    CL_IN ClTmsMemberIdT                    memberId,
    CL_IN ClTimeT                           timeout)
{
    ClRcT                                rc = CL_OK;
    struct gms_instance                 *gms_instance_ptr = NULL;
    ClGmsGroupLeaveRequestT              req = {0};
    ClGmsGroupLeaveResponseT            *res = NULL;
    ClIocMcastUserInfoT                  multicastInfo = {0};
    ClIocPhysicalAddressT                selfAddress = {0};

    CL_GMS_SET_CLIENT_VERSION( req );

    rc = clHandleCheckout(handle_database, gmsHandle, (void*)&gms_instance_ptr);
    if (rc != CL_OK)
    {
        return rc;
    }
    CL_ASSERT(gms_instance_ptr != NULL);

    /*Get self IOC address details */
    selfAddress.nodeAddress = clIocLocalAddressGet();
    clEoMyEoIocPortGet(&selfAddress.portId);
    
    req.gmsHandle   = gmsHandle;
    req.groupId     = groupId;
    req.memberId    = memberId;
    req.sync        = CL_TRUE;
    
    clGmsMutexLock(gms_instance_ptr->response_mutex);

    rc = cl_gms_group_leave_rmd(&req, (ClUint32T)(timeout/NS_IN_MS), &res);
    if (rc != CL_OK) /* If there was an error, res isn't allocated */
    {
        goto error_unlock_checkin;
    }
    CL_ASSERT(res != NULL);
    
    rc = res->rc;
    if (rc == CL_OK)
    {
        /* Deregister this application from the IOC
         * multicast registry */
        /* Extract multicast address information to deregister with IOC */
        multicastInfo.mcastAddr = CL_IOC_MULTICAST_ADDRESS_FORM(0, groupId);
        multicastInfo.physicalAddr.nodeAddress = selfAddress.nodeAddress;
        multicastInfo.physicalAddr.portId = selfAddress.portId;

        rc = clIocMulticastDeregister(&multicastInfo);
        if (rc != CL_OK)
        {
            clLogError(GROUPS,NA,
                       "IOC Multicast Deregister failed with rc = 0x%x",rc);
            rc = CL_GMS_ERR_IOC_DEREGISTRATION;
        }
    }
    
    clHeapFree((void*)res);
    
error_unlock_checkin:
    clGmsMutexUnlock(gms_instance_ptr->response_mutex);
    
    clHandleCheckin(handle_database, gmsHandle);
    return CL_GMS_RC(rc);
}

/*-----------------------------------------------------------------------------
 * Group Track API
 *---------------------------------------------------------------------------*/
ClRcT clTmsGroupTrack(
    CL_IN    ClTmsHandleT                   gmsHandle,
    CL_IN    ClTmsGroupIdT                  groupId,
    CL_IN    ClUint8T                       trackFlags,
    CL_INOUT ClTmsGroupNotificationBufferT *notificationBuffer)
{    
    ClRcT                       rc = CL_OK;
    struct gms_instance        *gms_instance_ptr = NULL;
    ClGmsGroupTrackRequestT     req = {0};
    ClGmsGroupTrackResponseT   *res = NULL;
    const ClUint8T validFlag = CL_GMS_TRACK_CURRENT | CL_GMS_TRACK_CHANGES |
                               CL_GMS_TRACK_CHANGES_ONLY;
    ClBoolT shouldFreeNotification = CL_TRUE;

    if (((trackFlags | validFlag) ^ validFlag) != 0) {
        return CL_GMS_RC(CL_ERR_BAD_FLAG);
    }

    CL_GMS_SET_CLIENT_VERSION( req );
    
    if (((trackFlags & CL_GMS_TRACK_CURRENT) == CL_GMS_TRACK_CURRENT) && /* If current view is requested */
        (notificationBuffer != NULL) &&        /* Buffer is provided */
        (notificationBuffer->notification != NULL) && /* Caller provides array */
        (notificationBuffer->numberOfItems == 0)) /* then size must be given */
    {
        return CL_GMS_RC(CL_ERR_INVALID_PARAMETER);
    }
    
    if (trackFlags == 0) /* at least one flag should be specified */
    {
        return CL_GMS_RC(CL_ERR_INVALID_PARAMETER);
    }
    
    if (((trackFlags & CL_GMS_TRACK_CHANGES) == CL_GMS_TRACK_CHANGES) &&
        ((trackFlags & CL_GMS_TRACK_CHANGES_ONLY) == CL_GMS_TRACK_CHANGES_ONLY)) /* mutually exclusive flags */
    {
        return CL_GMS_RC(CL_ERR_INVALID_PARAMETER);
    }
    
    rc = clHandleCheckout(handle_database, gmsHandle, (void*)&gms_instance_ptr);
    if (rc != CL_OK)
    {
        return CL_GMS_RC(CL_ERR_INVALID_HANDLE);
    }
    CL_ASSERT(gms_instance_ptr != NULL);

    /* If not a sync call, then clGmsClusterTrackCallbackHandler must be given */
    if (((trackFlags & (CL_GMS_TRACK_CHANGES|CL_GMS_TRACK_CHANGES_ONLY)) != 0) ||
        (((trackFlags & CL_GMS_TRACK_CURRENT) == CL_GMS_TRACK_CURRENT) &&
         (notificationBuffer == NULL)))
    {
        if (gms_instance_ptr->callbacks.clGmsGroupTrackCallback == NULL)
        {
            rc = CL_GMS_RC(CL_ERR_NO_CALLBACK);
            goto error_checkin;
        }
    }
    
    req.gmsHandle  = gmsHandle;
    req.trackFlags = trackFlags;
    req.sync       = CL_FALSE;
    req.groupId    = groupId;
    req.address.iocPhyAddress.nodeAddress = clIocLocalAddressGet();
    rc = clEoMyEoIocPortGet(&(req.address.iocPhyAddress.portId));
    
    CL_ASSERT(rc == CL_OK); /* Should really never happen */
    
    clGmsMutexLock(gms_instance_ptr->response_mutex);
    
    if (((trackFlags & CL_GMS_TRACK_CURRENT) == CL_GMS_TRACK_CURRENT) &&
        (notificationBuffer != NULL)) /* Sync response requested */
    {
        /*
         * We need to call the extended track() request which returns with
         * a notification buffer allocated by the XDR layer.
         */
        req.sync = CL_TRUE;
        rc = cl_gms_group_track_rmd(&req, 0 /* use def. timeout */, &res);
        if (rc != CL_OK)
        {
            goto error_unlock_checkin;
        }
        CL_ASSERT(res != NULL);

        if (res->rc != CL_OK) /* If other side indicated error, we need
                               * to free the buffer.*/
        {
            rc = res->rc;
            goto error_exit;
        }

        /* All fine, need to copy buffer */
        if (notificationBuffer->notification == NULL) /* we provide array */
        {
            memcpy(notificationBuffer, &res->buffer,
                   sizeof(*notificationBuffer)); /* This takes care of array */
            shouldFreeNotification = CL_FALSE;
        }
        else
        { /* caller provided array with fixed given size; we need to copy if
           * there is enough space.
           */
            if (notificationBuffer->numberOfItems >=
                res->buffer.numberOfItems)
            {
                /* Copy array, as much as we can */
                memcpy((void*)notificationBuffer->notification,
                       (void*)res->buffer.notification,
                       res->buffer.numberOfItems *
                          sizeof(ClGmsGroupNotificationT));
            } 
            /*
             * Instead of copying the rest of the fields in buffer one-by-one,
             * we do a trick: relink the above array and than copy the entire
             * struck over.  This will keep working even if the buffer struct
             * grows in the future; without change here.
             */
            clHeapFree((void*)res->buffer.notification);
            res->buffer.notification = notificationBuffer->notification;
            memcpy((void*)notificationBuffer, (void*)&res->buffer,
                   sizeof(*notificationBuffer));
            shouldFreeNotification = CL_FALSE;
        }
    }
    else
    {
        /* No sync response requested, so we call the simple rmd call */
        rc = cl_gms_group_track_rmd(&req, 0 /* use def. timeout */, &res);
        if (rc != CL_OK)
        {
            goto error_unlock_checkin;
        }
        CL_ASSERT(res != NULL);

        rc = res->rc;
    }     

error_exit:
    if((shouldFreeNotification == CL_TRUE) && 
            (res->buffer.notification != NULL))
    {
        clHeapFree((void*)res->buffer.notification);
    }
    clHeapFree((void*)res);
    
error_unlock_checkin:
    clGmsMutexUnlock(gms_instance_ptr->response_mutex);
    
error_checkin:
    if (clHandleCheckin(handle_database, gmsHandle) != CL_OK)
    {
        clLogError(CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,
                   "\nclHandleCheckin Failed");
    }
    
    return rc;
}

/*-----------------------------------------------------------------------------
 * Group Track Stop API
 *---------------------------------------------------------------------------*/
ClRcT clTmsGroupTrackStop(
    CL_IN ClTmsHandleT     gmsHandle,
    CL_IN ClTmsGroupIdT    groupId)
{
    ClRcT                           rc = CL_OK;
    struct gms_instance            *gms_instance_ptr = NULL;
    ClGmsGroupTrackStopRequestT     req = {0};
    ClGmsGroupTrackStopResponseT   *res = NULL;

    CL_GMS_SET_CLIENT_VERSION( req );
    rc = clHandleCheckout(handle_database, gmsHandle, (void*)&gms_instance_ptr);
    if (rc != CL_OK)
    {
        return rc;
    }
    
    req.gmsHandle = gmsHandle;
    req.groupId   = groupId;
    req.address.iocPhyAddress.nodeAddress = clIocLocalAddressGet();
    rc = clEoMyEoIocPortGet(&(req.address.iocPhyAddress.portId));

    CL_ASSERT(rc == CL_OK); /* Should really never happen */
    
    clGmsMutexLock(gms_instance_ptr->response_mutex);
    
    rc = cl_gms_group_track_stop_rmd(&req, 0 /* use def. timeout */, &res);
    if (rc != CL_OK) /* If there was an error, res isn't allocated */
    {
        goto error_exit;
    }
    
    rc = res->rc;
    
    clHeapFree((void*)res);

error_exit:
    clGmsMutexUnlock(gms_instance_ptr->response_mutex);
    
    clHandleCheckin(handle_database, gmsHandle);
    return rc;
}

/*-----------------------------------------------------------------------------
 * Get the list of Group Info API
 *---------------------------------------------------------------------------*/
ClRcT clTmsListGroups(
        CL_IN     ClTmsHandleT             gmsHandle,
        CL_IN     ClTimeT                  timeout,
        CL_INOUT  ClTmsGroupInfoListT      *groups)
{
    ClRcT   rc = CL_OK;
    ClGmsGroupsInfoListGetRequestT      req = {0};
    ClGmsGroupsInfoListGetResponseT     *res = NULL;
    struct gms_instance                 *gms_instance_ptr = NULL;

    CL_GMS_SET_CLIENT_VERSION( req );

    rc = clHandleCheckout(handle_database, gmsHandle, (void*)&gms_instance_ptr);
    if (rc != CL_OK)
    {
        return rc;
    }
    CL_ASSERT(gms_instance_ptr != NULL);

    req.gmsHandle   = gmsHandle;

    clGmsMutexLock(gms_instance_ptr->response_mutex);

    rc = cl_gms_group_list_get_rmd(&req, (ClUint32T)(timeout/NS_IN_MS), &res);
    if (rc != CL_OK) /* If there was an error, res isn't allocated */
    {
        goto error_unlock_checkin;
    }
    CL_ASSERT(res != NULL);

    groups->noOfGroups = res->groupsList.noOfGroups;
    groups->groupInfoList = res->groupsList.groupInfoList;
    rc = res->rc;

    clHeapFree((void*)res);

error_unlock_checkin:
    clGmsMutexUnlock(gms_instance_ptr->response_mutex);

    clHandleCheckin(handle_database, gmsHandle);
    return CL_GMS_RC(rc);
}

/*-----------------------------------------------------------------------------
 * Group Info Get API
 *---------------------------------------------------------------------------*/

ClRcT clTmsGetGroupInfo(
        CL_IN     ClTmsHandleT             gmsHandle,
        CL_IN     ClTmsGroupNameT         *groupName,
        CL_IN     ClTimeT                  timeout,
        CL_INOUT  ClTmsGroupInfoT         *groupInfo)
{
    ClRcT   rc = CL_OK;
    ClGmsGroupInfoGetRequestT      req = {0};
    ClGmsGroupInfoGetResponseT    *res = NULL;
    struct gms_instance           *gms_instance_ptr = NULL;

    if ((groupName == NULL) || (groupInfo == NULL))
    {
        return CL_GMS_RC(CL_ERR_NULL_POINTER);
    }

    CL_GMS_SET_CLIENT_VERSION( req );

    rc = clHandleCheckout(handle_database, gmsHandle, (void*)&gms_instance_ptr);
    if (rc != CL_OK)
    {
        return rc;
    }
    CL_ASSERT(gms_instance_ptr != NULL);

    req.gmsHandle   = gmsHandle;
    req.groupName   = *groupName;

    clGmsMutexLock(gms_instance_ptr->response_mutex);

    rc = cl_gms_group_info_get_rmd(&req, (ClUint32T)(timeout/NS_IN_MS), &res);
    if (rc != CL_OK) /* If there was an error, res isn't allocated */
    {
        goto error_unlock_checkin;
    }
    CL_ASSERT(res != NULL);

    memcpy(groupInfo,&(res->groupInfo),sizeof(ClGmsGroupInfoT));
    rc = res->rc;

    clHeapFree((void*)res);

error_unlock_checkin:
    clGmsMutexUnlock(gms_instance_ptr->response_mutex);

    clHandleCheckin(handle_database, gmsHandle);
    return CL_GMS_RC(rc);
}


/*-----------------------------------------------------------------------------
 * API to send a multicast message to a group
 *---------------------------------------------------------------------------*/
ClRcT clTmsSendAll(CL_IN   ClTmsHandleT        gmsHandle,
                   CL_IN   ClTmsGroupIdT       groupId,
                   CL_IN   ClTmsMemberIdT      memberId,
                   CL_IN   ClTimeT             timeout,
                   CL_IN   ClUint32T           dataSize,
                   CL_IN   ClPtrT              data)
{
    ClRcT                        rc = CL_OK;
    ClGmsGroupMcastRequestT      req = {0};
    ClGmsGroupMcastResponseT    *res = NULL;
    struct gms_instance         *gms_instance_ptr = NULL;

    if (data == NULL)
    {
        return CL_GMS_RC(CL_ERR_NULL_POINTER);
    }

    CL_GMS_SET_CLIENT_VERSION(req);

    rc = clHandleCheckout(handle_database, gmsHandle, (void*)&gms_instance_ptr);
    if (rc != CL_OK)
    {
        return rc;
    }
    CL_ASSERT(gms_instance_ptr != NULL);

    req.groupId     = groupId;
    req.memberId    = memberId;
    req.sync        = CL_TRUE;
    req.dataSize    = dataSize;
    req.data        = data;

    clGmsMutexLock(gms_instance_ptr->response_mutex);

    clLogTrace(GROUPS,NA,"Sending the group mcast RMD with data size %d",req.dataSize);
    rc = cl_gms_group_mcast_send_rmd(&req, (ClUint32T)(timeout/NS_IN_MS), &res);
    if (rc != CL_OK) /* If there was an error, res isn't allocated */
    {
        clLogError(GROUPS,NA,"Group mcast RMD failed with rc 0x%x",rc);
        goto error_unlock_checkin;
    }
    CL_ASSERT(res != NULL);

    rc = res->rc;

    clHeapFree((void*)res);

error_unlock_checkin:
    clGmsMutexUnlock(gms_instance_ptr->response_mutex);

    clHandleCheckin(handle_database, gmsHandle);
    return CL_GMS_RC(rc);
}

/*----------------------------------------------------------------------------
 *  Group Track Callback Handler
 *---------------------------------------------------------------------------*/
ClRcT clTmsGroupTrackCallbackHandler(
    CL_IN   ClGmsGroupTrackCallbackDataT *res)
{
    ClRcT rc = CL_OK;
    struct gms_instance *gms_instance_ptr = NULL;
    ClGmsHandleT gmsHandle = CL_GMS_INVALID_HANDLE;

    CL_ASSERT(res != NULL);
    
    gmsHandle = res->gmsHandle;
    rc = clHandleCheckout(handle_database, gmsHandle, (void*)&gms_instance_ptr);
    if (rc != CL_OK)
    {
        goto error_free_res;
    }
    CL_ASSERT(gms_instance_ptr != NULL);
    
    if (gms_instance_ptr->callbacks.clGmsGroupTrackCallback == NULL)
    {
        rc = CL_GMS_RC(CL_ERR_NO_CALLBACK);
        goto error_checkin_free_res;
    }

    /*
     * Calling the user's callback function with the data.  The user cannot
     * free the data we provide.  If it needs to reatin it, it has to copy
     * it out from what we provide here.
     */
    (*gms_instance_ptr->callbacks.clGmsGroupTrackCallback)
                  (res->groupId, &res->buffer, res->numberOfMembers, res->rc);

error_checkin_free_res:
    clHandleCheckin(handle_database, gmsHandle);

error_free_res:
    /* Need to free data (res) if are not able to call the actual callback */
    if (res->buffer.notification != NULL)
    {
        clHeapFree((void*)res->buffer.notification);
    }
    clHeapFree((void*)res);
    return rc;
}

/*----------------------------------------------------------------------------
 *  Group multicast callback handler
 *---------------------------------------------------------------------------*/
ClRcT clTmsGroupMcastCallbackHandler(
        CL_IN ClGmsGroupMcastCallbackDataT *res)
{
    ClRcT rc = CL_OK;
    struct gms_instance *gms_instance_ptr = NULL;
    ClGmsHandleT gmsHandle = CL_GMS_INVALID_HANDLE;

    CL_ASSERT(res != NULL);
    
    gmsHandle = res->gmsHandle;
    rc = clHandleCheckout(handle_database, gmsHandle, (void*)&gms_instance_ptr);
    if (rc != CL_OK)
    {
        goto error_free_res;
    }
    CL_ASSERT(gms_instance_ptr != NULL);
    
    if (gms_instance_ptr->mcastMessageDeliveryCallback == NULL)
    {
        rc = CL_GMS_RC(CL_ERR_NO_CALLBACK);
        goto error_checkin_free_res;
    }

    /*
     * Calling the user's callback function with the data.  Its users 
     * responsibility to free the data provided here
     */
    (*gms_instance_ptr->mcastMessageDeliveryCallback)
                  (res->groupId, res->memberId, res->dataSize, res->data);

error_checkin_free_res:
    clHandleCheckin(handle_database, gmsHandle);

error_free_res:
    /* Need to free data (res) if are not able to call the actual callback */
    clHeapFree((void*)res);
    return rc;
}

