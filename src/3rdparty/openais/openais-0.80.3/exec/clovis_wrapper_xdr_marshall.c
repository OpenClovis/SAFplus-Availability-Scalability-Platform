#include "clovis_gms_wrapper.h"

ClRcT marshallClVersionT(ClVersionT *version, ClBufferHandleT bufferHandle)
{
    /* Marshall releaseCode */
    CHECK_RETURN(clXdrMarshallClUint8T(&(version->releaseCode),bufferHandle,0), CL_OK);

    /* Marshall majorVersion */
    CHECK_RETURN(clXdrMarshallClUint8T(&(version->majorVersion),bufferHandle,0), CL_OK);

    /* Marshall minorVersion */
    CHECK_RETURN(clXdrMarshallClUint8T(&(version->minorVersion),bufferHandle,0), CL_OK);
    return CL_OK;
}


ClRcT marshallClGmsClusterMemberT(ClGmsClusterMemberT *clusterNode, ClBufferHandleT bufferHandle)
{
    /* Marshall nodeId */
    CHECK_RETURN(clXdrMarshallClUint32T(&(clusterNode->nodeId),bufferHandle,0), CL_OK);

    /* Marshall nodeAddress field */
    CHECK_RETURN(clXdrMarshallClUint32T(&(clusterNode->nodeAddress.iocPhyAddress.nodeAddress),bufferHandle,0), CL_OK);
    CHECK_RETURN(clXdrMarshallClUint32T(&(clusterNode->nodeAddress.iocPhyAddress.portId),bufferHandle,0), CL_OK);

    /* Marshall nodeIpAddress field */
    CHECK_RETURN(clXdrMarshallClUint32T(&(clusterNode->nodeIpAddress.family),bufferHandle,0), CL_OK);
    CHECK_RETURN(clXdrMarshallClUint16T(&(clusterNode->nodeIpAddress.length),bufferHandle,0), CL_OK);
    CHECK_RETURN(clXdrMarshallArrayClCharT(&(clusterNode->nodeIpAddress.value),CL_GMS_MAX_ADDRESS_LENGTH,bufferHandle,0), CL_OK);

    /* Marshall nodeName filed */
    CHECK_RETURN(clXdrMarshallSaNameT(&(clusterNode->nodeName),bufferHandle,0), CL_OK);

    /* Marshall memberActive field */
    CHECK_RETURN(clXdrMarshallClUint16T(&(clusterNode->memberActive),bufferHandle,0), CL_OK);

    /* Marshall bootTimeStamp */
    CHECK_RETURN(clXdrMarshallClInt64T(&(clusterNode->bootTimestamp),bufferHandle,0), CL_OK);

    /* Marshall initialViewNumber */
    CHECK_RETURN(clXdrMarshallClUint64T(&(clusterNode->initialViewNumber),bufferHandle,0), CL_OK);

    /* Marshall credential */
    CHECK_RETURN(clXdrMarshallClUint32T(&(clusterNode->credential),bufferHandle,0), CL_OK);

    /* Marshall isCurrentLeader field */
    CHECK_RETURN(clXdrMarshallClUint16T(&(clusterNode->isCurrentLeader),bufferHandle,0), CL_OK);

    /* Marshall isPreferredLeader field */
    CHECK_RETURN(clXdrMarshallClUint16T(&(clusterNode->isPreferredLeader),bufferHandle,0), CL_OK);

    /* Marshall leaderPreferenceSet field */
    CHECK_RETURN(clXdrMarshallClUint16T(&(clusterNode->leaderPreferenceSet),bufferHandle,0), CL_OK);

    /* Marshall gmsVersion field */
    CHECK_RETURN(marshallClVersionT(&(clusterNode->gmsVersion),bufferHandle), CL_OK);
    return CL_OK;
}

ClRcT   marshallClGmsGroupMemberT(ClGmsGroupMemberT *groupMember, ClBufferHandleT bufferHandle)
{
    /* Marshall memberId field */
    CHECK_RETURN(clXdrMarshallClUint64T(&(groupMember->handle),bufferHandle,0), CL_OK);

    /* Marshall memberId field */
    CHECK_RETURN(clXdrMarshallClUint32T(&(groupMember->memberId),bufferHandle,0), CL_OK);

    /* Marshall memberAddress field */
    CHECK_RETURN(clXdrMarshallClUint32T(&(groupMember->memberAddress.iocPhyAddress.nodeAddress),bufferHandle,0), CL_OK);
    CHECK_RETURN(clXdrMarshallClUint32T(&(groupMember->memberAddress.iocPhyAddress.portId),bufferHandle,0), CL_OK);

    /* Marshall memberName filed */
    CHECK_RETURN(clXdrMarshallSaNameT(&(groupMember->memberName),bufferHandle,0), CL_OK);

    /* Marshall memberActive field */
    CHECK_RETURN(clXdrMarshallClUint16T(&(groupMember->memberActive),bufferHandle,0), CL_OK);

    /* Marshall joinTimeStamp */
    CHECK_RETURN(clXdrMarshallClInt64T(&(groupMember->joinTimestamp),bufferHandle,0), CL_OK);

    /* Marshall initialViewNumber */
    CHECK_RETURN(clXdrMarshallClUint64T(&(groupMember->initialViewNumber),bufferHandle,0), CL_OK);

    /* Marshall credential */
    CHECK_RETURN(clXdrMarshallClUint32T(&(groupMember->credential),bufferHandle,0), CL_OK);
    return CL_OK;
}

ClRcT   marshallClGmsGroupInfoT(ClGmsGroupInfoT *groupInfo, ClBufferHandleT bufferHandle)
{
    /* Marshall groupName filed */
    CHECK_RETURN(clXdrMarshallSaNameT(&(groupInfo->groupName),bufferHandle,0), CL_OK);

    /* Marshall groupId field */
    CHECK_RETURN(clXdrMarshallClUint32T(&(groupInfo->groupId),bufferHandle,0), CL_OK);

    /* Marshall groupParams field */
    CHECK_RETURN(clXdrMarshallClUint16T(&(groupInfo->groupParams.isIocGroup),bufferHandle,0), CL_OK);
    CHECK_RETURN(clXdrMarshallArrayClCharT(&(groupInfo->groupParams.unused),256,bufferHandle,0), CL_OK);

    /* Marshall noOfMembers field */
    CHECK_RETURN(clXdrMarshallClUint32T(&(groupInfo->noOfMembers),bufferHandle,0), CL_OK);

    /* Marshall setForDelete field */
    CHECK_RETURN(clXdrMarshallClUint16T(&(groupInfo->setForDelete),bufferHandle,0), CL_OK);

    /* Marshall iocMulticastAddr field */
    CHECK_RETURN(clXdrMarshallClUint64T(&(groupInfo->iocMulticastAddr),bufferHandle,0), CL_OK);

    /* Marshall iocMulticastAddr field */
    CHECK_RETURN(clXdrMarshallClInt64T(&(groupInfo->creationTimestamp),bufferHandle,0), CL_OK);

    /* Marshall iocMulticastAddr field */
    CHECK_RETURN(clXdrMarshallClInt64T(&(groupInfo->lastChangeTimestamp),bufferHandle,0), CL_OK);
    return CL_OK;
}

ClRcT   marshallMCastMessage(struct mcastMessage *mcastMsg, ClPtrT  data, ClBufferHandleT bufferHandle)
{
    CHECK_RETURN(marshallClGmsGroupMemberT(&(mcastMsg->groupInfo.gmsGroupNode),bufferHandle),CL_OK);
    CHECK_RETURN(marshallClGmsGroupInfoT(&(mcastMsg->groupInfo.groupData),bufferHandle),CL_OK);
    CHECK_RETURN(clXdrMarshallClUint32T(&(mcastMsg->userDataSize),bufferHandle,0),CL_OK);
    CHECK_RETURN(clXdrMarshallArrayClCharT(data, mcastMsg->userDataSize,bufferHandle,0),CL_OK);
    return CL_OK;
}


ClRcT   marshallSyncMessage(struct VDECL(req_exec_gms_nodejoin) *req_exec_gms_nodejoin, ClBufferHandleT bufferHandle)
{
    ClGmsGroupSyncNotificationT *sync = (ClGmsGroupSyncNotificationT *)(req_exec_gms_nodejoin->dataPtr);
    ClUint32T                    index = 0;
    ClGmsGroupInfoT             *groupInfo = NULL;
    ClGmsGroupMemberT           *groupMember = NULL;
    // marshall also group id in group data for sync msg
    ClGmsGroupInfoT     	*groupData = NULL;

    /* Marshall noOfGroups field */
    CHECK_RETURN(clXdrMarshallClInt32T(&(sync->noOfGroups),bufferHandle,0), CL_OK);

    /* Marshall groupInfo field */
    for (index = 0; index < sync->noOfGroups; index++)
    {
        groupInfo = &(sync->groupInfoList[index]);
        CHECK_RETURN(marshallClGmsGroupInfoT(groupInfo, bufferHandle), CL_OK);
    }

    /* Marshall noOfMembers field */
    CHECK_RETURN(clXdrMarshallClInt32T(&(sync->noOfMembers),bufferHandle,0), CL_OK);

    /* Marshall groupInfo field */
    for (index = 0; index < sync->noOfMembers; index++)
    {
        groupMember = &(sync->groupMemberList[index].viewMember.groupMember);
        CHECK_RETURN(marshallClGmsGroupMemberT(groupMember, bufferHandle), CL_OK);
	// marshall also group id
        groupData = &(sync->groupMemberList[index].viewMember.groupData);
	CHECK_RETURN(marshallClGmsGroupInfoT(groupData,bufferHandle),CL_OK);
    }

    return CL_OK;
}

ClRcT   marshallHeader(struct VDECL(req_exec_gms_nodejoin) *req_exec_gms_nodejoin, ClBufferHandleT bufferHandle)
{
    /* Marshall version field */
    CHECK_RETURN(marshallClVersionT(&(req_exec_gms_nodejoin->version),bufferHandle), CL_OK);

    /* Marshall gmsMessageType */
    CHECK_RETURN(clXdrMarshallClInt32T(&(req_exec_gms_nodejoin->gmsMessageType),bufferHandle,0), CL_OK);

    /* Marshall context handle field */
    CHECK_RETURN(clXdrMarshallClInt64T(&(req_exec_gms_nodejoin->contextHandle),bufferHandle,0), CL_OK);

    /* Marshall gmsGroupId Field */
    CHECK_RETURN(clXdrMarshallClInt32T(&(req_exec_gms_nodejoin->gmsGroupId),bufferHandle,0), CL_OK);

    /* Marshall Eject Reason field */
    CHECK_RETURN(clXdrMarshallClInt32T(&(req_exec_gms_nodejoin->ejectReason),bufferHandle,0), CL_OK);
    return CL_OK;
}

ClRcT   marshallReqExecGmsNodeJoin(struct VDECL(req_exec_gms_nodejoin) *req_exec_gms_nodejoin, ClBufferHandleT bufferHandle)
{
    /* Marshall the header members */
    CHECK_RETURN(marshallHeader(req_exec_gms_nodejoin,bufferHandle), CL_OK);

    switch (req_exec_gms_nodejoin->gmsMessageType)
    {
        case CL_GMS_CLUSTER_JOIN_MSG:
        case CL_GMS_CLUSTER_LEAVE_MSG:
        case CL_GMS_CLUSTER_EJECT_MSG:
        case CL_GMS_LEADER_ELECT_MSG:
            CHECK_RETURN(marshallClGmsClusterMemberT(&(req_exec_gms_nodejoin->specificMessage.gmsClusterNode), bufferHandle), CL_OK);
            break;
        case CL_GMS_GROUP_CREATE_MSG:
        case CL_GMS_GROUP_DESTROY_MSG:
        case CL_GMS_GROUP_JOIN_MSG:
        case CL_GMS_GROUP_LEAVE_MSG:
            CHECK_RETURN(marshallClGmsGroupMemberT(&(req_exec_gms_nodejoin->specificMessage.groupMessage.gmsGroupNode), bufferHandle), CL_OK);
            CHECK_RETURN(marshallClGmsGroupInfoT(&(req_exec_gms_nodejoin->specificMessage.groupMessage.groupData), bufferHandle), CL_OK);
            break; 
        case CL_GMS_COMP_DEATH:
            CHECK_RETURN(marshallClGmsGroupMemberT(&(req_exec_gms_nodejoin->specificMessage.groupMessage.gmsGroupNode), bufferHandle), CL_OK);
            break;
        case CL_GMS_GROUP_MCAST_MSG:
            CHECK_RETURN(marshallMCastMessage(&(req_exec_gms_nodejoin->specificMessage.mcastMessage),req_exec_gms_nodejoin->dataPtr,bufferHandle),CL_OK);
            break;
        case CL_GMS_SYNC_MESSAGE:
            CHECK_RETURN(marshallSyncMessage(req_exec_gms_nodejoin, bufferHandle), CL_OK);
            break;
    }
    return CL_OK;
}
