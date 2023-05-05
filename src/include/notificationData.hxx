#include <sstream>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/access.hpp>

#include <clHandleApi.hxx>

#include <clCommon6.h>
#include <saAmf.h>

#define NOTIFICATION_CHANNEL_NAME "CL_AMF_NOTIFICATION"

#define ENTITY_TYPE_NODE_STR    "Node"
#define ENTITY_TYPE_SU_STR      "ServiceUnit"
#define ENTITY_TYPE_COMP_STR    "Component"
#define ENTITY_TYPE_SG_STR      "ServiceGroup"
#define ENTITY_TYPE_SI_STR      "ServiceInstance"
#define ENTITY_TYPE_CSI_STR     "ComponentServiceInstance"

#define NTF_TYPE_TO_STR(notificationType)(                                                                                               \
    (notificationType == SAFplus::CL_AMF_NOTIFICATION_FAULT) ? "CL_AMF_NOTIFICATION_FAULT" :                                         \
    (notificationType == SAFplus::CL_AMF_NOTIFICATION_SU_INSTANTIATION_FAILURE) ? "CL_AMF_NOTIFICATION_SU_INSTANTIATION_FAILURE" :   \
    (notificationType == SAFplus::CL_AMF_NOTIFICATION_SU_HA_STATE_CHANGE) ? "CL_AMF_NOTIFICATION_SU_HA_STATE_CHANGE" :               \
    (notificationType == SAFplus::CL_AMF_NOTIFICATION_SI_FULLY_ASSIGNED) ? "CL_AMF_NOTIFICATION_SI_FULLY_ASSIGNED" :                 \
    (notificationType == SAFplus::CL_AMF_NOTIFICATION_SI_PARTIALLY_ASSIGNED) ? "CL_AMF_NOTIFICATION_SI_PARTIALLY_ASSIGNED" :         \
    (notificationType == SAFplus::CL_AMF_NOTIFICATION_SI_UNASSIGNED) ? "CL_AMF_NOTIFICATION_SI_UNASSIGNED" :                         \
    (notificationType == SAFplus::CL_AMF_NOTIFICATION_COMP_ARRIVAL) ? "CL_AMF_NOTIFICATION_COMP_ARRIVAL" :                           \
    (notificationType == SAFplus::CL_AMF_NOTIFICATION_COMP_DEPARTURE) ? "CL_AMF_NOTIFICATION_COMP_DEPARTURE" :                       \
    (notificationType == SAFplus::CL_AMF_NOTIFICATION_NODE_ARRIVAL) ? "CL_AMF_NOTIFICATION_NODE_ARRIVAL" :                           \
    (notificationType == SAFplus::CL_AMF_NOTIFICATION_NODE_DEPARTURE) ? "CL_AMF_NOTIFICATION_NODE_DEPARTURE" :                       \
    (notificationType == SAFplus::CL_AMF_NOTIFICATION_ENTITY_CREATE) ? "CL_AMF_NOTIFICATION_ENTITY_CREATE" :                         \
    (notificationType == SAFplus::CL_AMF_NOTIFICATION_ENTITY_DELETE) ? "CL_AMF_NOTIFICATION_ENTITY_DELETE" :                         \
    (notificationType == SAFplus::CL_AMF_NOTIFICATION_OPER_STATE_CHANGE) ? "CL_AMF_NOTIFICATION_OPER_STATE_CHANGE" :                 \
    (notificationType == SAFplus::CL_AMF_NOTIFICATION_ADMIN_STATE_CHANGE) ? "CL_AMF_NOTIFICATION_ADMIN_STATE_CHANGE" :               \
    (notificationType == SAFplus::CL_AMF_NOTIFICATION_NODE_SWITCHOVER) ? "CL_AMF_NOTIFICATION_NODE_SWITCHOVER" :                     \
    (notificationType == SAFplus::CL_AMF_NOTIFICATION_NODE_FAILOVER) ? "CL_AMF_NOTIFICATION_NODE_FAILOVER" :                         \
    (notificationType == SAFplus::CL_AMF_NOTIFICATION_COMP_HA_STATE_CHANGE) ? "CL_AMF_NOTIFICATION_COMP_HA_STATE_CHANGE" :           \
    "INVALID_NOTIFICATION_TYPE"                                                                                                      \
)

namespace SAFplus
{
    enum ClAmfNotificationType
    {
        CL_AMF_NOTIFICATION_NONE,
        CL_AMF_NOTIFICATION_FAULT,
        CL_AMF_NOTIFICATION_SU_INSTANTIATION_FAILURE,
        CL_AMF_NOTIFICATION_SU_HA_STATE_CHANGE,
        CL_AMF_NOTIFICATION_SI_FULLY_ASSIGNED,
        CL_AMF_NOTIFICATION_SI_PARTIALLY_ASSIGNED,
        CL_AMF_NOTIFICATION_SI_UNASSIGNED,
        CL_AMF_NOTIFICATION_COMP_ARRIVAL,
        CL_AMF_NOTIFICATION_COMP_DEPARTURE,
        CL_AMF_NOTIFICATION_NODE_ARRIVAL,
        CL_AMF_NOTIFICATION_NODE_DEPARTURE,
        CL_AMF_NOTIFICATION_ENTITY_CREATE,
        CL_AMF_NOTIFICATION_ENTITY_DELETE, 
        CL_AMF_NOTIFICATION_OPER_STATE_CHANGE,    
        CL_AMF_NOTIFICATION_ADMIN_STATE_CHANGE,
        CL_AMF_NOTIFICATION_NODE_SWITCHOVER,
        CL_AMF_NOTIFICATION_NODE_FAILOVER,
        CL_AMF_NOTIFICATION_COMP_HA_STATE_CHANGE,
        CL_AMF_NOTIFICATION_MAX,
    };

    class NotificationPayload
    {
    private:
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)    // used internally by boost::serialization
        {
            ar & notificationType;
            ar & serializedData;
        };
    public:
        ClAmfNotificationType notificationType;
        std::string serializedData;
    };

    class ClAmfNotificationDescriptor
    {
    private:
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)    // used internally by boost::serialization
        {
            ar & entityType;
            ar & entityName;
            ar & faultyCompName;
            ar & siName;
            ar & suName;
            ar & lastHAState;
            ar & newHAState;
            ar & recoveryActionTaken;
            ar & repairNecessary;
            ar & lastOperState;
            ar & newOperState;
            ar & lastAdminState;
            ar & newAdminState;
        };
    public:
        std::string entityType;
        std::string entityName;
        std::string faultyCompName;
        std::string siName;
        std::string suName;
        SaAmfHAStateT lastHAState;
        SaAmfHAStateT newHAState;
        SaAmfRecommendedRecoveryT recoveryActionTaken;
        bool repairNecessary;
        SaAmfOperationalStateT lastOperState;
        SaAmfOperationalStateT newOperState;
        SaAmfAdminStateT lastAdminState;
        SaAmfAdminStateT newAdminState;

        ClAmfNotificationDescriptor()
        {
            //avoid uninitialized data exception
            repairNecessary = false;
        };
    };

    class ClEventNodePayLoad
    {
    private:
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)    // used internally by boost::serialization
        {
            ar & nodeName;
            ar & nodeIocAddress;
        }
    public:
        std::string nodeName;
        ClUint16T nodeIocAddress;

        ClEventNodePayLoad()
        {
            nodeIocAddress = 0;
        };
    };

    class ClEventPayLoad
    {
    private:
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)    // used internally by boost::serialization
        {
            ar & compName;
            ar & nodeName;
            ar & nodeIocAddress;
            ar & compIocPort;
        }
    public:
        std::string compName;
        std::string nodeName;
        ClUint16T nodeIocAddress;
        ClUint32T compIocPort;

        ClEventPayLoad()
        {
            nodeIocAddress = 0;
            compIocPort = 0;
        };
    };

    class ClAmfNotificationInfo
    {
    public:
        ClAmfNotificationType type;

        ClAmfNotificationDescriptor *amfStateNotification;
        ClEventPayLoad *amfCompNotification;
        ClEventNodePayLoad *amfNodeNotification;
    };

    /*
     * Serialize functions
     */

    template<typename SerializableType>
    std::string serializeToString(SerializableType obj)
    {
        std::stringstream ss;
        boost::archive::text_oarchive oa(ss);
        oa << obj;    //insert data into archive
        return ss.str();
    };

    template<typename SerializableType>
    SerializableType parseFromString(std::string serializedObj)
    {
        SerializableType result;
        std::stringstream ss(serializedObj);
        boost::archive::text_iarchive ia(ss);
        ia >> result;    //extract data from archive
        return result;
    };
}
