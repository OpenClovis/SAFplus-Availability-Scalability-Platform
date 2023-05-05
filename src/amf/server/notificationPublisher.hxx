#include <EventClient.hxx>
#include <notificationData.hxx>
#include <clHandleApi.hxx>
#include <clAmfIpi.hxx>

#include <Component.hxx>
#pragma once

#define STANDARDIZE_O_STATE(clOperState) (                             \
    clOperState ? SaAmfOperationalStateT::SA_AMF_OPERATIONAL_ENABLED : \
    SaAmfOperationalStateT::SA_AMF_OPERATIONAL_DISABLED                \
)

#define STANDARDIZE_A_STATE(clAdminState) (                                                                             \
    (clAdminState == SAFplusAmf::AdministrativeState::off) ? SaAmfAdminStateT::SA_AMF_ADMIN_LOCKED_INSTANTIATION :      \
    (clAdminState == SAFplusAmf::AdministrativeState::idle) ? SaAmfAdminStateT::SA_AMF_ADMIN_LOCKED :                   \
    (clAdminState == SAFplusAmf::AdministrativeState::shuttingDown) ? SaAmfAdminStateT::SA_AMF_ADMIN_SHUTTING_DOWN :    \
    SaAmfAdminStateT::SA_AMF_ADMIN_UNLOCKED                                                                             \
)

namespace SAFplus
{
    class NotificationPublisher: private EventClient
    {
    private:
        std::string eventChannelName;
        EventChannelScope channelScope;

        void handleCompAnnouncement(SAFplusI::CompAnnouncementPayload *payload);
    public:
        NotificationPublisher();

        ClRcT initialize(Handle nodeHandle);
        ClRcT finalize();

        void msgHandler(MsgServer* svr, Message* msgHead, ClPtrT cookie);   //this receives arrival/departure announcements from components

        template<typename SerializableType>
        ClRcT notificationPublish(ClAmfNotificationType notiType, SerializableType notificationData);

        ClRcT adminStateNotifiPublish(std::string entityType,
                                      std::string entityName,
                                      SAFplusAmf::AdministrativeState lastAdminState,
                                      SAFplusAmf::AdministrativeState newAdminState);

        ClRcT compNotifiPublish(std::string compName,
                                std::string nodeName,
                                SAFplus::Handle compHdl,
                                SAFplus::Handle nodeHdl,
                                SAFplus::ClAmfNotificationType compNotiType);

        ClRcT nodeNotifiPublish(std::string nodeName,
                                Handle nodeHdl,
                                SAFplus::ClAmfNotificationType compNotiType);

        ClRcT haStateNotifiPublish(std::string entityType,
                                   std::string entityName,
                                   std::string siName,
                                   SAFplusAmf::HighAvailabilityState lastHAState,
                                   SAFplusAmf::HighAvailabilityState newHAState);

        ClRcT faultNotifiPublish(std::string entityType,
                                 std::string entityName,
                                 std::string faultyCompName,
                                 SAFplusAmf::Recovery recoveryActionTaken);

        ClRcT entityNotifiPublish(std::string entityType,
                                  std::string entityName,
                                  SAFplus::ClAmfNotificationType entityNotiType);

        ClRcT operStateNotifiPublish(std::string entityType,
                                     std::string entityName,
                                     bool lastOperState,
                                     bool newOperState);
        ClRcT assignmentStateNotifiPublish(SAFplusAmf::ServiceInstance* si);
        ClRcT nodeSwitchoverNotifiPublish(SAFplusAmf::Node* node);
        ClRcT nodeFailoverNotifiPublish(Handle nodeHdl);
    };
}
