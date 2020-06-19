#include <clDebugApi.h>
#include <clIocDebugCli.h>

ClDebugFuncEntryT iocDebugFuncList[] =
{

    {
        (ClDebugCallbackT) clIocDebugCliTransportMcastPeerListAdd,
        "mcastPeerListAdd",
        "Add a peer address to the multicast list"
    },
    {
            (ClDebugCallbackT) clIocDebugCliTransportMcastPeerListDelete,
            "mcastPeerListDelete",
            "Delete a peer address from the multicast list"
    },
    {
            (ClDebugCallbackT) clIocDebugCliTransportMcastPeerListGet,
            "mcastPeerListGet",
            "Get peer address list from the multicast list"
    }
};
