#include <notificationPublisher.hxx>
#include <clLogIpi.hxx>
#include <clNameApi.hxx>

SAFplus::NotificationPublisher::NotificationPublisher() : SAFplus::EventClient()
{
    this->eventChannelName = NOTIFICATION_CHANNEL_NAME;
    this->channelScope = EventChannelScope::EVENT_GLOBAL_CHANNEL;
}

ClRcT SAFplus::NotificationPublisher::initialize(SAFplus::Handle nodeHandle)
{
    eventInitialize(nodeHandle, NULL);
    ClRcT rc = eventChannelOpen(this->eventChannelName, this->channelScope);
    if (rc != CL_OK)
    {
        logError("NTF", "INI", "Openning event channel failed for NotificationPublisher, rc [0x%x]", rc);
        return rc;
    }
    rc = eventChannelPublish(this->eventChannelName, this->channelScope);
    if (rc != CL_OK)
        logError("NTF", "INI", "Subscribing NotificationPublisher as event publisher failed, rc [0x%x]", rc);
    return rc;
}

ClRcT SAFplus::NotificationPublisher::finalize()
{
    ClRcT rc = eventChannelUnsubscribe(this->eventChannelName, this->channelScope);
    if (rc != CL_OK)
        logError("NTF","FIN", "NotificationPublisher unsubscribing failed, rc [0x%x]", rc);
    return rc;
}

void SAFplus::NotificationPublisher::msgHandler(MsgServer* svr, Message* msgHead, ClPtrT cookie)
{
    Message* msg = msgHead;
    while (msg)
    {
        assert(msg->firstFragment == msg->lastFragment);  // This code is only written to handle one fragment.
        MsgFragment* frag = msg->firstFragment;
        const void *raw = frag->read(0);
        SAFplusI::CompAnnouncementPayload *payload = (SAFplusI::CompAnnouncementPayload *) raw;
        handleCompAnnouncement(payload);
        msg = msg->nextMsg;
    }
    msgHead->msgPool->free(msgHead);
}

void SAFplus::NotificationPublisher::handleCompAnnouncement(SAFplusI::CompAnnouncementPayload *payload)
{
    ClAmfNotificationType notiType;
    if (payload->type == SAFplusI::CompAnnoucementType::ARRIVAL)
        notiType = ClAmfNotificationType::CL_AMF_NOTIFICATION_COMP_ARRIVAL;
    else
        notiType = ClAmfNotificationType::CL_AMF_NOTIFICATION_COMP_DEPARTURE;

    compNotifiPublish(payload->compName, payload->nodeName, payload->compHdl, payload->nodeHdl, notiType);
}

template<typename SerializableType>
ClRcT SAFplus::NotificationPublisher::notificationPublish(ClAmfNotificationType notiType, SerializableType notificationData)
{
    logDebug("NTF", "PUB", "Publishing notification type [%s]", NTF_TYPE_TO_STR(notiType));

    std::string serializedObj = serializeToString<SerializableType>(notificationData);
    NotificationPayload payload;
    payload.notificationType = notiType;
    payload.serializedData = serializedObj;

    std::string dataToSend = serializeToString<NotificationPayload>(payload);
    int dummySize = 0;
    ClRcT rc = eventPublish(dataToSend, dummySize, this->eventChannelName, this->channelScope);
    if (rc != CL_OK)
        logError("NTF", "PUB", "Publishing notification type [%s] failed, rc [0x%x]", NTF_TYPE_TO_STR(notiType), rc);
    return rc;
}

ClRcT SAFplus::NotificationPublisher::adminStateNotifiPublish(std::string entityType,
                                                            std::string entityName,
                                                            SAFplusAmf::AdministrativeState lastAdminState,
                                                            SAFplusAmf::AdministrativeState newAdminState)
{
    ClAmfNotificationType notiType = ClAmfNotificationType::CL_AMF_NOTIFICATION_ADMIN_STATE_CHANGE;
    ClAmfNotificationDescriptor notiData;
    notiData.entityType = entityType;
    notiData.entityName = entityName;
    notiData.lastAdminState = STANDARDIZE_A_STATE(lastAdminState);
    notiData.newAdminState = STANDARDIZE_A_STATE(newAdminState);

    return notificationPublish<ClAmfNotificationDescriptor>(notiType, notiData);
}

ClRcT SAFplus::NotificationPublisher::compNotifiPublish(std::string compName,
                                                      std::string nodeName,
                                                      SAFplus::Handle compHdl,
                                                      SAFplus::Handle nodeHdl,
                                                      SAFplus::ClAmfNotificationType compNotiType)
{
    ClEventPayLoad notiData;
    notiData.compName = compName;
    notiData.nodeName = nodeName;
    notiData.nodeIocAddress = nodeHdl.getNode();
    notiData.compIocPort = compHdl.getPort();

    return notificationPublish<ClEventPayLoad>(compNotiType, notiData);
}

ClRcT SAFplus::NotificationPublisher::nodeNotifiPublish(std::string nodeName,
                                                      SAFplus::Handle nodeHdl,
                                                      SAFplus::ClAmfNotificationType compNotiType)
{
    ClEventNodePayLoad notiData;
    notiData.nodeName = nodeName;
    notiData.nodeIocAddress = nodeHdl.getNode();

    return notificationPublish<ClEventNodePayLoad>(compNotiType, notiData);
}

ClRcT SAFplus::NotificationPublisher::haStateNotifiPublish(std::string entityType,
                                                         std::string entityName,
                                                         std::string siName,
                                                         SAFplusAmf::HighAvailabilityState lastHAState,
                                                         SAFplusAmf::HighAvailabilityState newHAState)
{
    ClAmfNotificationType notiType;
    ClAmfNotificationDescriptor notiData;

    if (!entityType.compare(ENTITY_TYPE_COMP_STR))
    {
        notiType = ClAmfNotificationType::CL_AMF_NOTIFICATION_COMP_HA_STATE_CHANGE;
        notiData.entityName = entityName;
    }
    else    //SAFplusAmf::ServiceUnit
    {
        notiType = ClAmfNotificationType::CL_AMF_NOTIFICATION_SU_HA_STATE_CHANGE;
        notiData.suName = entityName;
    }

    notiData.entityType = entityType;
    notiData.siName = siName;
    notiData.lastHAState = static_cast<SaAmfHAStateT>(lastHAState);
    notiData.newHAState = static_cast<SaAmfHAStateT>(newHAState);

    return notificationPublish<ClAmfNotificationDescriptor>(notiType, notiData);
}

ClRcT SAFplus::NotificationPublisher::faultNotifiPublish(std::string entityType,
                                                       std::string entityName,
                                                       std::string faultyCompName,
                                                       SAFplusAmf::Recovery recoveryActionTaken)
{
    ClAmfNotificationType notiType;
    ClAmfNotificationDescriptor notiData;
    notiData.repairNecessary = true;

    if (!entityType.compare(ENTITY_TYPE_SU_STR))
    {
        notiType = ClAmfNotificationType::CL_AMF_NOTIFICATION_SU_INSTANTIATION_FAILURE;
    }
    else    //SAFplusAmf::Node || SAFplusAmf::Component
    {
        notiType = ClAmfNotificationType::CL_AMF_NOTIFICATION_FAULT;
        if (recoveryActionTaken == SAFplusAmf::Recovery::CompRestart || recoveryActionTaken == SAFplusAmf::Recovery::SuRestart)
            notiData.repairNecessary = false;
    }

    notiData.entityType = entityType;
    notiData.entityName = entityName;
    notiData.faultyCompName = faultyCompName;
    notiData.recoveryActionTaken = static_cast<SaAmfRecommendedRecoveryT>(recoveryActionTaken);

    return notificationPublish<ClAmfNotificationDescriptor>(notiType, notiData);
}

ClRcT SAFplus::NotificationPublisher::entityNotifiPublish(std::string entityType,
                                                        std::string entityName,
                                                        SAFplus::ClAmfNotificationType entityNotiType)
{
    ClAmfNotificationDescriptor notiData;
    notiData.entityType = entityType;
    notiData.entityName = entityName;

    return notificationPublish<ClAmfNotificationDescriptor>(entityNotiType, notiData);
}

ClRcT SAFplus::NotificationPublisher::operStateNotifiPublish(std::string entityType,
                                                           std::string entityName,
                                                           bool lastOperState,
                                                           bool newOperState)
{
    ClAmfNotificationType notiType = ClAmfNotificationType::CL_AMF_NOTIFICATION_OPER_STATE_CHANGE;
    ClAmfNotificationDescriptor notiData;
    notiData.entityType = entityType;
    notiData.entityName = entityName;
    notiData.lastOperState = STANDARDIZE_O_STATE(lastOperState);
    notiData.newOperState = STANDARDIZE_O_STATE(newOperState);

    return notificationPublish<ClAmfNotificationDescriptor>(notiType, notiData);
}

ClRcT SAFplus::NotificationPublisher::assignmentStateNotifiPublish(SAFplusAmf::ServiceInstance* si)

{
    ClAmfNotificationType notiType;
    ClAmfNotificationDescriptor notiData;
    if (si->assignmentState.value == SAFplusAmf::AssignmentState::fullyAssigned) {
        notiType = ClAmfNotificationType::CL_AMF_NOTIFICATION_SI_FULLY_ASSIGNED;    
    }
    else if (si->assignmentState.value == SAFplusAmf::AssignmentState::partiallyAssigned) {
        notiType = ClAmfNotificationType::CL_AMF_NOTIFICATION_SI_PARTIALLY_ASSIGNED;
    }
    else if (si->assignmentState.value == SAFplusAmf::AssignmentState::unassigned) {
        notiType = ClAmfNotificationType::CL_AMF_NOTIFICATION_SI_UNASSIGNED;
    }
    notiData.entityType = ENTITY_TYPE_SI_STR;
    notiData.entityName = si->name;

    return notificationPublish<ClAmfNotificationDescriptor>(notiType, notiData);
}

ClRcT SAFplus::NotificationPublisher::nodeSwitchoverNotifiPublish(SAFplusAmf::Node* node)
{
    ClAmfNotificationType notiType = ClAmfNotificationType::CL_AMF_NOTIFICATION_NODE_SWITCHOVER;
    ClAmfNotificationDescriptor notiData;

    notiData.entityType = ENTITY_TYPE_NODE_STR;
    notiData.entityName = node->name;

    return notificationPublish<ClAmfNotificationDescriptor>(notiType, notiData);
}

ClRcT SAFplus::NotificationPublisher::nodeFailoverNotifiPublish(Handle nodeHdl)
{
    ClAmfNotificationType notiType = ClAmfNotificationType::CL_AMF_NOTIFICATION_NODE_FAILOVER;
    ClAmfNotificationDescriptor notiData;

    std::string nodeName = name.getName(nodeHdl);

    notiData.entityType = ENTITY_TYPE_NODE_STR;
    notiData.entityName = nodeName;

    return notificationPublish<ClAmfNotificationDescriptor>(notiType, notiData);
}


/*
 *  Template instantiations for types: ClAmfNotificationDescriptor, ClEventPayLoad, ClEventNodePayLoad
 */

template ClRcT SAFplus::NotificationPublisher::notificationPublish<SAFplus::ClAmfNotificationDescriptor>(ClAmfNotificationType, ClAmfNotificationDescriptor);
template ClRcT SAFplus::NotificationPublisher::notificationPublish<SAFplus::ClEventPayLoad>(ClAmfNotificationType, ClEventPayLoad);
template ClRcT SAFplus::NotificationPublisher::notificationPublish<SAFplus::ClEventNodePayLoad>(ClAmfNotificationType, ClEventNodePayLoad);
