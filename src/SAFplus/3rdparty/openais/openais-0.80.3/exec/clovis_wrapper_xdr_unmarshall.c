#include "clovis_gms_wrapper.h"

ClRcT   unmarshallClVersionT(ClBufferHandleT bufferHandle, ClVersionT *version)
{
    /* Unmarshall releaseCode */
    CHECK_RETURN(clXdrUnmarshallClUint8T(bufferHandle, &(version->releaseCode)), CL_OK);

    /* Unmarshall majorVersion */
    CHECK_RETURN(clXdrUnmarshallClUint8T(bufferHandle, &(version->majorVersion)), CL_OK);

    /* Unmarshall minorVersion */
    CHECK_RETURN(clXdrUnmarshallClUint8T(bufferHandle, &(version->minorVersion)), CL_OK);

    return CL_OK;
}


ClRcT   unmarshallClGmsClusterMemberT(ClBufferHandleT bufferHandle, ClGmsClusterMemberT *clusterNode)
{
    /* Unmarshall nodeId */
    CHECK_RETURN(clXdrUnmarshallClUint32T(bufferHandle, &(clusterNode->nodeId)), CL_OK);

    /* Unmarshall nodeAddress field */
    CHECK_RETURN(clXdrUnmarshallClUint32T(bufferHandle, &(clusterNode->nodeAddress.iocPhyAddress.nodeAddress)), CL_OK);
    CHECK_RETURN(clXdrUnmarshallClUint32T(bufferHandle, &(clusterNode->nodeAddress.iocPhyAddress.portId)), CL_OK);

    /* Unmarshall nodeIpAddress field */
    CHECK_RETURN(clXdrUnmarshallClUint32T(bufferHandle, &(clusterNode->nodeIpAddress.family)), CL_OK);
    CHECK_RETURN(clXdrUnmarshallClUint16T(bufferHandle, &(clusterNode->nodeIpAddress.length)), CL_OK);
    CHECK_RETURN(clXdrUnmarshallArrayClCharT(bufferHandle, &(clusterNode->nodeIpAddress.value),CL_GMS_MAX_ADDRESS_LENGTH), CL_OK);

    /* Unmarshall nodeName filed */
    CHECK_RETURN(clXdrUnmarshallSaNameT(bufferHandle, &(clusterNode->nodeName)), CL_OK);

    /* Unmarshall memberActive field */
    CHECK_RETURN(clXdrUnmarshallClUint16T(bufferHandle, &(clusterNode->memberActive)), CL_OK);

    /* Unmarshall bootTimeStamp */
    CHECK_RETURN(clXdrUnmarshallClInt64T(bufferHandle, &(clusterNode->bootTimestamp)), CL_OK);

    /* Unmarshall initialViewNumber */
    CHECK_RETURN(clXdrUnmarshallClUint64T(bufferHandle, &(clusterNode->initialViewNumber)), CL_OK);

    /* Unmarshall credential */
    CHECK_RETURN(clXdrUnmarshallClUint32T(bufferHandle, &(clusterNode->credential)), CL_OK);

    /* Unmarshall isCurrentLeader field */
    CHECK_RETURN(clXdrUnmarshallClUint16T(bufferHandle, &(clusterNode->isCurrentLeader)), CL_OK);

    /* Unmarshall isPreferredLeader field */
    CHECK_RETURN(clXdrUnmarshallClUint16T(bufferHandle, &(clusterNode->isPreferredLeader)), CL_OK);

    /* Unmarshall leaderPreferenceSet field */
    CHECK_RETURN(clXdrUnmarshallClUint16T(bufferHandle, &(clusterNode->leaderPreferenceSet)), CL_OK);

    /* Unmarshall gmsVersion field */
    CHECK_RETURN(unmarshallClVersionT(bufferHandle, &(clusterNode->gmsVersion)), CL_OK);

    return CL_OK;
}

ClRcT   unmarshallClGmsGroupMemberT(ClBufferHandleT bufferHandle, ClGmsGroupMemberT *groupMember)
{
    /* Unmarshall memberId field */
    CHECK_RETURN(clXdrUnmarshallClUint64T(bufferHandle, &(groupMember->handle)), CL_OK);

    /* Unmarshall memberId field */
    CHECK_RETURN(clXdrUnmarshallClUint32T(bufferHandle, &(groupMember->memberId)), CL_OK);

    /* Unmarshall memberAddress field */
    CHECK_RETURN(clXdrUnmarshallClUint32T(bufferHandle, &(groupMember->memberAddress.iocPhyAddress.nodeAddress)), CL_OK);
    CHECK_RETURN(clXdrUnmarshallClUint32T(bufferHandle, &(groupMember->memberAddress.iocPhyAddress.portId)), CL_OK);

    /* Unmarshall memberName filed */
    CHECK_RETURN(clXdrUnmarshallSaNameT(bufferHandle, &(groupMember->memberName)), CL_OK);

    /* Unmarshall memberActive field */
    CHECK_RETURN(clXdrUnmarshallClUint16T(bufferHandle, &(groupMember->memberActive)), CL_OK);

    /* Unmarshall joinTimeStamp */
    CHECK_RETURN(clXdrUnmarshallClInt64T(bufferHandle, &(groupMember->joinTimestamp)), CL_OK);

    /* Unmarshall initialViewNumber */
    CHECK_RETURN(clXdrUnmarshallClUint64T(bufferHandle, &(groupMember->initialViewNumber)), CL_OK);

    /* Unmarshall credential */
    CHECK_RETURN(clXdrUnmarshallClUint32T(bufferHandle, &(groupMember->credential)), CL_OK);

    return CL_OK;
}

ClRcT   unmarshallClGmsGroupInfoT(ClBufferHandleT bufferHandle, ClGmsGroupInfoT *groupInfo)
{
    /* Unmarshall groupName filed */
    CHECK_RETURN(clXdrUnmarshallSaNameT(bufferHandle, &(groupInfo->groupName)), CL_OK);

    /* Unmarshall groupId field */
    CHECK_RETURN(clXdrUnmarshallClUint32T(bufferHandle, &(groupInfo->groupId)), CL_OK);

    /* Unmarshall groupParams field */
    CHECK_RETURN(clXdrUnmarshallClUint16T(bufferHandle, &(groupInfo->groupParams.isIocGroup)), CL_OK);
    CHECK_RETURN(clXdrUnmarshallArrayClCharT(bufferHandle, &(groupInfo->groupParams.unused),256), CL_OK);

    /* Unmarshall noOfMembers field */
    CHECK_RETURN(clXdrUnmarshallClUint32T(bufferHandle, &(groupInfo->noOfMembers)), CL_OK);

    /* Unmarshall setForDelete field */
    CHECK_RETURN(clXdrUnmarshallClUint16T(bufferHandle, &(groupInfo->setForDelete)), CL_OK);

    /* Unmarshall iocMulticastAddr field */
    CHECK_RETURN(clXdrUnmarshallClUint64T(bufferHandle, &(groupInfo->iocMulticastAddr)), CL_OK);

    /* Unmarshall iocMulticastAddr field */
    CHECK_RETURN(clXdrUnmarshallClInt64T(bufferHandle, &(groupInfo->creationTimestamp)), CL_OK);

    /* Unmarshall iocMulticastAddr field */
    CHECK_RETURN(clXdrUnmarshallClInt64T(bufferHandle, &(groupInfo->lastChangeTimestamp)), CL_OK);

    return CL_OK;
}

ClRcT   unmarshallSyncMessage(ClBufferHandleT bufferHandle, struct VDECL(req_exec_gms_nodejoin) *req_exec_gms_nodejoin)
{
    ClGmsGroupSyncNotificationT *sync = NULL;
    ClUint32T                    index = 0;

    sync = clHeapAllocate(sizeof(ClGmsGroupSyncNotificationT));
    if (sync == NULL)
    {
        clLogError(OPN,AIS,
                "Failed to allocate memory while processing sync message");
        return CL_ERR_NO_MEMORY;
    }
    
    /* Unmarshall noOfGroups field */
    CHECK_RETURN(clXdrUnmarshallClUint32T(bufferHandle, &(sync->noOfGroups)), CL_OK);

    if (sync->noOfGroups > 0)
    {
        sync->groupInfoList = clHeapAllocate(sizeof(ClGmsGroupInfoT) * sync->noOfGroups);
        if (sync->groupInfoList == NULL)
        {
            clLogError(OPN,AIS,
                    "Failed to allocate memory while processing sync message");
            return CL_ERR_NO_MEMORY;
        }

        for (index = 0; index < sync->noOfGroups; index++)
        {
            CHECK_RETURN(unmarshallClGmsGroupInfoT(bufferHandle, &(sync->groupInfoList[index])), CL_OK);
        }
    }

    /* Unmarshall noOfMembers field */
    CHECK_RETURN(clXdrUnmarshallClUint32T(bufferHandle, &(sync->noOfMembers)), CL_OK);

    if (sync->noOfMembers > 0)
    {
        sync->groupMemberList = clHeapCalloc(1,sizeof(ClGmsViewNodeT) * sync->noOfMembers);
        if (sync->groupMemberList == NULL)
        {
            clLogError(OPN,AIS,
                    "Failed to allocate memory while processing sync message");
            return CL_ERR_NO_MEMORY;
        }

        for (index = 0; index < sync->noOfMembers; index++)
        {
            CHECK_RETURN(unmarshallClGmsGroupMemberT(bufferHandle, &(sync->groupMemberList[index].viewMember.groupMember)), CL_OK);
	    // marshall also group id in group data for sync
            CHECK_RETURN(unmarshallClGmsGroupInfoT(bufferHandle, &(sync->groupMemberList[index].viewMember.groupData)), CL_OK);
        }
    }

    req_exec_gms_nodejoin->dataPtr = (ClPtrT) sync;

    return CL_OK;
}

ClRcT   unmarshallMCastMessage(ClBufferHandleT bufferHandle, struct VDECL(req_exec_gms_nodejoin) *req_exec_gms_nodejoin)
{
    struct mcastMessage *mcastMsg = &(req_exec_gms_nodejoin->specificMessage.mcastMessage);

    CHECK_RETURN(unmarshallClGmsGroupMemberT(bufferHandle, &(mcastMsg->groupInfo.gmsGroupNode)),CL_OK);
    CHECK_RETURN(unmarshallClGmsGroupInfoT(bufferHandle, &(mcastMsg->groupInfo.groupData)),CL_OK);
    CHECK_RETURN(clXdrUnmarshallClUint32T(bufferHandle, &(mcastMsg->userDataSize)),CL_OK);

    req_exec_gms_nodejoin->dataPtr = clHeapAllocate(mcastMsg->userDataSize);
    if (req_exec_gms_nodejoin->dataPtr == NULL)
    {
        clLogError(OPN,AIS,
                "Failed to allocate memory while unmarshalling the multicast message");
        return CL_ERR_NO_MEMORY;
    }

    CHECK_RETURN(clXdrUnmarshallArrayClCharT(bufferHandle, req_exec_gms_nodejoin->dataPtr, mcastMsg->userDataSize),CL_OK);

    return CL_OK;
}

ClRcT   unmarshallHeader(ClBufferHandleT bufferHandle, struct VDECL(req_exec_gms_nodejoin) *req_exec_gms_nodejoin)
{
    /* Unmarshall version field */
    CHECK_RETURN(unmarshallClVersionT(bufferHandle, &(req_exec_gms_nodejoin->version)), CL_OK);

    /* Unmarshall gmsMessageType */
    CHECK_RETURN(clXdrUnmarshallClInt32T(bufferHandle, &(req_exec_gms_nodejoin->gmsMessageType)), CL_OK);

    /* Unmarshall context handle field */
    CHECK_RETURN(clXdrUnmarshallClInt64T(bufferHandle, &(req_exec_gms_nodejoin->contextHandle)), CL_OK);

    /* Unmarshall gmsGroupId field */
    CHECK_RETURN(clXdrUnmarshallClInt32T(bufferHandle, &(req_exec_gms_nodejoin->gmsGroupId)), CL_OK);

    /* Unmarshall Eject Reason field */
    CHECK_RETURN(clXdrUnmarshallClInt32T(bufferHandle, &(req_exec_gms_nodejoin->ejectReason)), CL_OK);

    return CL_OK;
}

ClRcT   unmarshallReqExecGmsNodeJoin(ClBufferHandleT bufferHandle, struct VDECL(req_exec_gms_nodejoin) *req_exec_gms_nodejoin)
{
    /* Unmarshall the header members */
    CHECK_RETURN(unmarshallHeader(bufferHandle, req_exec_gms_nodejoin), CL_OK);

    switch (req_exec_gms_nodejoin->gmsMessageType)
    {
        case CL_GMS_CLUSTER_JOIN_MSG:
        case CL_GMS_CLUSTER_LEAVE_MSG:
        case CL_GMS_CLUSTER_EJECT_MSG:
        case CL_GMS_LEADER_ELECT_MSG:
            CHECK_RETURN(unmarshallClGmsClusterMemberT(bufferHandle, &(req_exec_gms_nodejoin->specificMessage.gmsClusterNode)), CL_OK);
            break;
        case CL_GMS_GROUP_CREATE_MSG:
        case CL_GMS_GROUP_DESTROY_MSG:
        case CL_GMS_GROUP_JOIN_MSG:
        case CL_GMS_GROUP_LEAVE_MSG:
            CHECK_RETURN(unmarshallClGmsGroupMemberT(bufferHandle, &(req_exec_gms_nodejoin->specificMessage.groupMessage.gmsGroupNode)), CL_OK);
            CHECK_RETURN(unmarshallClGmsGroupInfoT(bufferHandle, &(req_exec_gms_nodejoin->specificMessage.groupMessage.groupData)), CL_OK);
            break; 
        case CL_GMS_COMP_DEATH:
            CHECK_RETURN(unmarshallClGmsGroupMemberT(bufferHandle, &(req_exec_gms_nodejoin->specificMessage.groupMessage.gmsGroupNode)), CL_OK);
            break;
        case CL_GMS_GROUP_MCAST_MSG:
            CHECK_RETURN(unmarshallMCastMessage(bufferHandle, req_exec_gms_nodejoin),CL_OK);
            break;
        case CL_GMS_SYNC_MESSAGE:
            CHECK_RETURN(unmarshallSyncMessage(bufferHandle, req_exec_gms_nodejoin),CL_OK);
            break;
    }

    return CL_OK;
}
