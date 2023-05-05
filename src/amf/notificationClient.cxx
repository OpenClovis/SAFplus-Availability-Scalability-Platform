#include <notificationClient.hxx>
#include <clLogIpi.hxx>

#include <boost/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

SAFplus::NotificationCallBack SAFplus::NotificationClient::notificationCallback = NULL;     //initialize static function pointer

SAFplus::NotificationClient::NotificationClient() : SAFplus::EventClient()
{
    this->eventChannelName = NOTIFICATION_CHANNEL_NAME;
    this->channelScope = EventChannelScope::EVENT_GLOBAL_CHANNEL;
    this->finalized = false;
}

SAFplus::NotificationClient::~NotificationClient()
{
    if(!this->finalized)
        this->finalize();
}

ClRcT SAFplus::NotificationClient::initialize(SAFplus::Handle clientHandle, SAFplus::NotificationCallBack notificationCallback)
{
    this->notificationCallback = notificationCallback;

    eventInitialize(clientHandle, this->eventCallback);

    int maxTry = 30; // try in 3s
    int tries = 0;
    while (tries++ < maxTry)
    {
        ClRcT rc = eventChannelSubscribe(this->eventChannelName, this->channelScope);
        if (rc == CL_ERR_TRY_AGAIN)
            boost::this_thread::sleep(boost::posix_time::milliseconds(100));
        else
        {
            if (rc != CL_OK)
                logError("NTF", "INI", "NotificationClient subscribing failed, rc [0x%x]", rc);
            return rc;
        }
    }
    logError("NTF", "INI", "NotificationClient took too long to subscribe to EventServer.");
    return CL_ERR_NOT_EXIST;
}

ClRcT SAFplus::NotificationClient::finalize()
{
    ClRcT rc = eventChannelUnsubscribe(this->eventChannelName, this->channelScope);
    if (rc != CL_OK)
    {
        logError("NTF","FIN", "NotificationClient unsubscribing failed, rc [0x%x]", rc);
        return rc;
    }
    this->finalized = true;
    return rc;
}

void SAFplus::NotificationClient::eventCallback(std::string channelName, EventChannelScope scope, std::string serializedObj, int length)
{
    ClAmfNotificationInfo notificationInfo;

    ClAmfNotificationDescriptor *stateNotification = NULL;
    ClEventPayLoad *compNotification = NULL;
    ClEventNodePayLoad *nodeNotification = NULL;

    //parse data and give it to client
    NotificationPayload parsedPayload = parseFromString<NotificationPayload>(serializedObj);

    switch (parsedPayload.notificationType)
    {
        case CL_AMF_NOTIFICATION_NODE_ARRIVAL:
        case CL_AMF_NOTIFICATION_NODE_DEPARTURE:
        {
            nodeNotification = new ClEventNodePayLoad;
            *nodeNotification = parseFromString<ClEventNodePayLoad>(parsedPayload.serializedData);
            notificationInfo.amfNodeNotification = nodeNotification;
            break;
        }

        case CL_AMF_NOTIFICATION_COMP_ARRIVAL:
        case CL_AMF_NOTIFICATION_COMP_DEPARTURE:
        {
            compNotification = new ClEventPayLoad;
            *compNotification = parseFromString<ClEventPayLoad>(parsedPayload.serializedData);
            notificationInfo.amfCompNotification = compNotification;
            break;
        }

        default:
        {
            stateNotification = new ClAmfNotificationDescriptor;
            *stateNotification = parseFromString<ClAmfNotificationDescriptor>(parsedPayload.serializedData);
            notificationInfo.amfStateNotification = stateNotification;
        }
    }

    notificationInfo.type = parsedPayload.notificationType;
    notificationCallback(notificationInfo);     //calls user's provided notificationCallback

    if (stateNotification)
        delete stateNotification;
    else if (compNotification)
        delete compNotification;
    else if (nodeNotification)
        delete nodeNotification;
}
