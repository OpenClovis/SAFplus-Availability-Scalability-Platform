#include <EventClient.hxx>
#include <notificationData.hxx>
#include <clHandleApi.hxx>
#include <clAmfIpi.hxx>

namespace SAFplus
{
    typedef void (*NotificationCallBack)(ClAmfNotificationInfo data);

    class NotificationClient: private EventClient
    {
    private:
        bool finalized;
        std::string eventChannelName;
        EventChannelScope channelScope;

        static NotificationCallBack notificationCallback;
        static void eventCallback(std::string channelName, EventChannelScope scope, std::string serializedObj, int length);     //this wraps user's notificationCallback, this will be called by parent class
    public:
        NotificationClient();
        ~NotificationClient();

        ClRcT initialize(Handle clientHandle, NotificationCallBack notificationCallback);
        ClRcT finalize();
    };
}
